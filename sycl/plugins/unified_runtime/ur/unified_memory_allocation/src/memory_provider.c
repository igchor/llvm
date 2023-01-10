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

static struct uma_memory_provider_ops_t *
ops(uma_memory_provider_handle_t hProvider) {
  return (struct uma_memory_provider_ops_t *)hProvider;
}

 enum uma_result_t
umaMemoryProviderAlloc(uma_memory_provider_handle_t hProvider, size_t size,
                       size_t alignment, void **ptr) {
  return ops(hProvider)->alloc(hProvider, size, alignment, ptr);
}

 enum uma_result_t
umaMemoryProviderFree(uma_memory_provider_handle_t hProvider, void *ptr,
                      size_t size) {
  return ops(hProvider)->free(hProvider, ptr, size);
}
