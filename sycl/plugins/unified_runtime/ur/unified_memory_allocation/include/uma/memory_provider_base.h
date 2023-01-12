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

#pragma once
#ifdef __cplusplus
extern "C" {
#endif

#include <uma/base.h>

typedef struct uma_memory_provider_t *uma_memory_provider_handle_t;

struct uma_memory_provider_ops_t {
  enum uma_result_t (*alloc)(uma_memory_provider_handle_t, size_t, size_t,
                             void **);
  enum uma_result_t (*free)(uma_memory_provider_handle_t, void *, size_t);
};

enum uma_result_t umaMemoryProviderAlloc(uma_memory_provider_handle_t, size_t,
                                         size_t, void **);
enum uma_result_t umaMemoryProviderFree(uma_memory_provider_handle_t, void *,
                                        size_t);

#ifdef __cplusplus
}
#endif
