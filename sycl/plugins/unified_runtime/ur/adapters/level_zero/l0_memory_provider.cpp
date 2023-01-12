/*
    Copyright (c) 2023 Intel Corporation
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

#include "l0_memory_provider.h"
#include "ur_level_zero.hpp"

// TODO: remove this (this is for defintions of pi_context/device)
#include "pi_level_zero.hpp"
static enum uma_result_t mapError(ze_result_t result) {
    if (result == ZE_RESULT_SUCCESS) {
        return UMA_RESULT_SUCCESS;
    }
    return UMA_RESULT_RUNTIME_ERROR;
}
#define ZE_CALL(ZeName, ZeArgs)                                                \
  {                                                                            \
    ze_result_t ZeResult = ZeName ZeArgs;                                      \
    if (auto Result = ZeCall().doCall(ZeResult, #ZeName, #ZeArgs, true))       \
      return mapError(Result);                                                 \
  }

#ifdef __cplusplus
extern "C" {
#endif

#define ASSERT(condition, error)                                            \
  if (!(condition))                                                            \
    return error;

struct l0_memory_provider_config_t {
    enum l0_memory_type type;
    int protection;

    pi_context hContext;
    pi_device hDevice;
};

l0_memory_provider_config_handle_t l0MemoryProviderConfigCreate() {
    return (l0_memory_provider_config_handle_t) malloc(sizeof(struct l0_memory_provider_config_t));
}

void l0MemoryProviderConfigDestroy(l0_memory_provider_config_handle_t hConfig) {
    free(hConfig);
}

void l0MemoryProviderConfigSetContext(l0_memory_provider_config_handle_t hConfig, pi_context hContext) {
    hConfig->hContext = hContext;
}

void l0MemoryProviderConfigSetDevice(l0_memory_provider_config_handle_t hConfig, pi_device hDevice) {
    hConfig->hDevice = hDevice;
}

void l0MemoryProviderConfigSetMemoryType(l0_memory_provider_config_handle_t hConfig, enum l0_memory_type type) {
    hConfig->type = type;
}

void l0MemoryProviderConfigSetMemoryProtection(l0_memory_provider_config_handle_t hConfig, int protection) {
    hConfig->protection = protection;
}

struct l0_memory_provider_t {
    struct uma_memory_provider_ops_t ops;
    struct l0_memory_provider_config_t config;
};

static enum uma_result_t l0DeviceAlloc(uma_memory_provider_handle_t hProvider,
                                    size_t size, size_t alignment, void **ptr) {
  struct l0_memory_provider_config_t *hConfig = &((struct l0_memory_provider_t*)hProvider)->config;
  ASSERT(hConfig->hContext, UMA_RESULT_INVALID_VALUE);
  ASSERT(hConfig->hDevice, UMA_RESULT_INVALID_VALUE);

  // Check that incorrect bits are not set in the properties.
  // TODO: should we have flags pre-populated inside the provider config?
  //   ASSERT(!Properties || *Properties == 0 ||
  //                 (*Properties == PI_MEM_ALLOC_FLAGS && *(Properties + 2) == 0),
  //             PI_ERROR_INVALID_VALUE);
  // TODO: translate PI properties to Level Zero flags

  ZeStruct<ze_device_mem_alloc_desc_t> zeDesc;
  zeDesc.flags = 0;
  zeDesc.ordinal = 0;

  ZeStruct<ze_relaxed_allocation_limits_exp_desc_t> RelaxedDesc;
  if (size > hConfig->hDevice->ZeDeviceProperties->maxMemAllocSize) {
    // Tell Level-Zero to accept Size > maxMemAllocSize
    RelaxedDesc.flags = ZE_RELAXED_ALLOCATION_LIMITS_EXP_FLAG_MAX_SIZE;
    zeDesc.pNext = &RelaxedDesc;
  }

  ZE_CALL(zeMemAllocDevice, (hConfig->hContext->ZeContext, &zeDesc, size, alignment,
                             hConfig->hDevice->ZeDevice, ptr));

  ASSERT(alignment == 0 ||
                reinterpret_cast<std::uintptr_t>(*ptr) % alignment == 0,
            UMA_RESULT_INVALID_VALUE);

  return UMA_RESULT_SUCCESS;
}

static enum uma_result_t l0SharedAlloc(uma_memory_provider_handle_t hProvider,
                                    size_t size, size_t alignment, void **ptr) {
  struct l0_memory_provider_config_t *hConfig = &((struct l0_memory_provider_t*)hProvider)->config;
  ASSERT(hConfig->hContext, UMA_RESULT_INVALID_VALUE);
  ASSERT(hConfig->hDevice, UMA_RESULT_INVALID_VALUE);

  // TODO: translate properties to Level Zero flags
  ZeStruct<ze_host_mem_alloc_desc_t> zeHostDesc;
  zeHostDesc.flags = 0;
  ZeStruct<ze_device_mem_alloc_desc_t> zeDevDesc;
  zeDevDesc.flags = 0;
  zeDevDesc.ordinal = 0;

  ZeStruct<ze_relaxed_allocation_limits_exp_desc_t> RelaxedDesc;
  if (size > hConfig->hDevice->ZeDeviceProperties->maxMemAllocSize) {
    // Tell Level-Zero to accept Size > maxMemAllocSize
    RelaxedDesc.flags = ZE_RELAXED_ALLOCATION_LIMITS_EXP_FLAG_MAX_SIZE;
    zeDevDesc.pNext = &RelaxedDesc;
  }

  ZE_CALL(zeMemAllocShared, (hConfig->hContext->ZeContext, &zeDevDesc, &zeHostDesc, size,
                             alignment, hConfig->hDevice->ZeDevice, ptr));

  ASSERT(alignment == 0 ||
                reinterpret_cast<std::uintptr_t>(*ptr) % alignment == 0,
            UMA_RESULT_INVALID_VALUE);

  // TODO: Handle PI_MEM_ALLOC_DEVICE_READ_ONLY.
  return UMA_RESULT_SUCCESS;
}

static enum uma_result_t l0HostAlloc(uma_memory_provider_handle_t hProvider,
                                    size_t size, size_t alignment, void **ptr) {
  struct l0_memory_provider_config_t *hConfig = &((struct l0_memory_provider_t*)hProvider)->config;
  ASSERT(hConfig->hContext, UMA_RESULT_INVALID_VALUE);

  // Check that incorrect bits are not set in the properties.
  // TODO
  //   ASSERT(!Properties || *Properties == 0 ||
  //                 (*Properties == PI_MEM_ALLOC_FLAGS && *(Properties + 2) == 0),
  //             PI_ERROR_INVALID_VALUE);

  // TODO: translate PI properties to Level Zero flags
  ZeStruct<ze_host_mem_alloc_desc_t> zeHostDesc;
  zeHostDesc.flags = 0;
  ZE_CALL(zeMemAllocHost,
          (hConfig->hContext->ZeContext, &zeHostDesc, size, alignment, ptr));

  ASSERT(alignment == 0 ||
                reinterpret_cast<std::uintptr_t>(*ptr) % alignment == 0,
            UMA_RESULT_INVALID_VALUE);

  return UMA_RESULT_SUCCESS;
}

static enum uma_result_t l0Alloc(uma_memory_provider_handle_t hProvider,
                                    size_t size, size_t alignment, void **ptr) {
    struct l0_memory_provider_config_t *hConfig = &((struct l0_memory_provider_t*)hProvider)->config;
    
    // TODO: handle protection

    if(hConfig->type == TypeDevice) {
        return l0DeviceAlloc(hProvider, size, alignment, ptr);
    } else if(hConfig->type == TypeHost) {
        return l0HostAlloc(hProvider, size, alignment, ptr);
    } else {
        return l0SharedAlloc(hProvider, size, alignment, ptr);
    }
}

static enum uma_result_t l0Free(uma_memory_provider_handle_t hProvider, void *ptr, size_t size) {
  (void) size;

  struct l0_memory_provider_config_t *hConfig = &((struct l0_memory_provider_t*)hProvider)->config;
  ZE_CALL(zeMemFree, (hConfig->hContext->ZeContext, ptr));

  return UMA_RESULT_SUCCESS;
}

enum uma_result_t
l0MemoryProviderCreate(l0_memory_provider_config_handle_t hConfig,
                          uma_memory_provider_handle_t *hOut) {
    struct l0_memory_provider_t *hProvider = (struct l0_memory_provider_t *) malloc(sizeof(struct l0_memory_provider_t));
    if (!hProvider) {
        return UMA_RESULT_RUNTIME_ERROR;
    }

    hProvider->config = *hConfig;
    hProvider->ops.alloc = l0Alloc;
    hProvider->ops.free = l0Free;

    *hOut = (uma_memory_provider_handle_t)hProvider;
    return UMA_RESULT_SUCCESS;
}

void l0MemoryProviderDestroy(uma_memory_provider_handle_t hProvider) {
    free(hProvider);
}

#ifdef __cplusplus
}
#endif
