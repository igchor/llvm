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

#include <uma/memory_provider.h>

#include <stdlib.h>

struct uma_memory_provider_t {
  struct uma_memory_provider_ops_t ops;
  void *provider_priv;
};

enum uma_result_t
umaMemoryProviderCreate(struct uma_memory_provider_ops_t *ops, void *params,
                        uma_memory_provider_handle_t *hProvider) {
  uma_memory_provider_handle_t provider =
      malloc(sizeof(struct uma_memory_provider_t));
  if (!provider) {
    return UMA_RESULT_RUNTIME_ERROR;
  }

  // TODO: handle any version mismatches
  provider->ops = *ops;

  void *provider_priv;
  enum uma_result_t ret = ops->initialize(params, &provider_priv);
  if (ret != UMA_RESULT_SUCCESS) {
    return ret;
  }

  provider->provider_priv = provider_priv;

  *hProvider = provider;

  return UMA_RESULT_SUCCESS;
}

void umaMemoryProviderDestroy(uma_memory_provider_handle_t hProvider) {
  hProvider->ops.finalize(hProvider->provider_priv);
  free(hProvider);
}

enum uma_result_t umaMemoryProviderAlloc(uma_memory_provider_handle_t hProvider,
                                         size_t size, size_t alignment,
                                         void **ptr) {
  return hProvider->ops.alloc(hProvider->provider_priv, size, alignment, ptr);
}

enum uma_result_t umaMemoryProviderFree(uma_memory_provider_handle_t hProvider,
                                        void *ptr, size_t size) {
  return hProvider->ops.free(hProvider->provider_priv, ptr, size);
}
