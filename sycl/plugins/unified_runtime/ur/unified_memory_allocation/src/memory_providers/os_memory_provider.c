/*
    Copyright (c) 2022 Intel Corporation
    Licensed under the Apache License, Version 2.0 (the "License");
    you may not use this file except in compliance with the License.
    You may obtain a copy of the License at
        http://www.apache.org/licenses/LICENSE-2.0
    Unless required by applicable law or agreed to in writing, software
    distributed under the License is distributed on an "AS IS" BASIS,
    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
    See the License for the specific language governing permissions and
    limitations under the License.
*/

#include <uma/memory_provider_base.h>
#include <uma/memory_providers/os_memory_provider.h>

#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <unistd.h>

struct uma_os_memory_provider_config_t {
  int protection;
  enum uma_mem_visibility visibility;

  // NUMA config
  unsigned long *nodemask;
  unsigned long maxnode;
  int numa_mode;
  int numa_flags;
};

struct uma_memory_provider_t {
  struct uma_memory_provider_ops_t ops;
  uma_os_memory_provider_config_handle_t config;
};

uma_os_memory_provider_config_handle_t umaOsMemoryProviderConfigCreate() {
  struct uma_os_memory_provider_config_t *config =
      malloc(sizeof(struct uma_os_memory_provider_config_t));

  config->protection = ProtectionRead | ProtectionWrite;
  config->visibility = VisibilityPrivate;

  config->nodemask = NULL;
  config->maxnode = 0;
  config->numa_mode = NumaModeDefault;
  config->numa_flags = 0;

  return (uma_os_memory_provider_config_handle_t)config;
}

void umaOsMemoryProviderConfigDestroy(
    uma_os_memory_provider_config_handle_t hConfig) {
  free(((struct uma_os_memory_provider_config_t *)hConfig)->nodemask);
  free(hConfig);
}

void umaOsMemoryProviderConfigSetMemoryProtection(
    uma_os_memory_provider_config_handle_t hConfig, int memoryProtection) {
  hConfig->protection = memoryProtection;
}

void umaOsMemoryProviderConfigSetMemoryVisibility(
    uma_os_memory_provider_config_handle_t hConfig,
    enum uma_mem_visibility memoryVisibility) {
  hConfig->visibility = memoryVisibility;
}

static enum uma_result_t copyNodeMask(const unsigned long *nodemask,
                                      unsigned long maxnode,
                                      unsigned long **outNodemask) {
  if (maxnode == 0 || nodemask == NULL) {
    *outNodemask = NULL;
    return UMA_RESULT_SUCCESS;
  }

  size_t nodemask_bytes = maxnode / 8;

  // round to the next multiple of sizeof(unsigned long)
  if (nodemask_bytes % sizeof(unsigned long) != 0 || nodemask_bytes == 0)
    nodemask_bytes +=
        sizeof(unsigned long) - nodemask_bytes % sizeof(unsigned long);

  *outNodemask = (unsigned long *)calloc(1, nodemask_bytes);
  if (!*outNodemask) {
    return UMA_RESULT_RUNTIME_ERROR;
  }

  memcpy(*outNodemask, nodemask, nodemask_bytes);

  return UMA_RESULT_SUCCESS;
}

enum uma_result_t umaOsMemoryProviderConfigSetNumaMemBind(
    uma_os_memory_provider_config_handle_t handle,
    const unsigned long *nodemask, unsigned long maxnode, int mode, int flags) {

  enum uma_result_t ret = copyNodeMask(nodemask, maxnode, &handle->nodemask);
  if (ret != UMA_RESULT_SUCCESS) {
    return ret;
  }

  handle->maxnode = maxnode;
  handle->numa_mode = mode;
  handle->numa_flags = flags;

  return UMA_RESULT_SUCCESS;
}

static enum uma_result_t
memoryProviderConfigCreateCopy(uma_os_memory_provider_config_handle_t in,
                               uma_os_memory_provider_config_handle_t *out) {
  *out = umaOsMemoryProviderConfigCreate();
  if (!*out) {
    return UMA_RESULT_RUNTIME_ERROR;
  }

  **out = *in;
  return copyNodeMask(in->nodemask, in->maxnode, &(*out)->nodemask);
}

static enum uma_result_t os_alloc(uma_memory_provider_handle_t h, size_t size,
                                  size_t alignment, void **resultPtr) {
  if (alignment > sysconf(_SC_PAGE_SIZE)) {
    return UMA_RESULT_RUNTIME_ERROR;
  }

  int flags = MAP_ANONYMOUS | h->config->visibility;
  int protection = h->config->protection;

  if (h->config->visibility == VisibilityShared &&
      h->config->numa_mode != NumaModeDefault) {
    // TODO: add support for that
    fprintf(stderr, "numa binding not supported for VisibilityShared");
    return UMA_RESULT_RUNTIME_ERROR;
  }

  void *result = mmap(NULL, size, protection, flags, -1, 0);
  if (result == MAP_FAILED) {
    fprintf(stderr, "syscall mmap() returned: %p", result);
    return UMA_RESULT_RUNTIME_ERROR;
  }

  int ret = mbind(result, size, h->config->numa_mode, h->config->nodemask,
                  h->config->maxnode, h->config->numa_flags);
  if (ret) {
    return UMA_RESULT_RUNTIME_ERROR;
  }

  *resultPtr = result;

  return UMA_RESULT_SUCCESS;
}

static enum uma_result_t os_free(uma_memory_provider_handle_t h, void *ptr,
                                 size_t bytes) {
  // TODO: round size to page?

  int ret = munmap(ptr, bytes);

  if (ret) {
    // TODO: map error
    return UMA_RESULT_RUNTIME_ERROR;
  }

  return UMA_RESULT_SUCCESS;
}

enum uma_result_t
umaOsMemoryProviderCreate(uma_os_memory_provider_config_handle_t hConfig,
                          uma_memory_provider_handle_t *out) {
  struct uma_memory_provider_t *provider =
      malloc(sizeof(struct uma_memory_provider_t));
  struct uma_memory_provider_ops_t ops = {.alloc = os_alloc,
                                          .free = os_free};

  provider->ops = ops;

  enum uma_result_t ret =
      memoryProviderConfigCreateCopy(hConfig, &provider->config);
  if (ret != UMA_RESULT_SUCCESS) {
    return ret;
  }

  *out = provider;
  return UMA_RESULT_SUCCESS;
}

void umaOSMemoryProviderDestroy(uma_memory_provider_handle_t hProvider) {
  umaOsMemoryProviderConfigDestroy(hProvider->config);
  free(hProvider);
}
