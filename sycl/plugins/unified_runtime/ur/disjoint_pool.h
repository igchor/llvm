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

#pragma once
#ifdef __cplusplus
extern "C" {
#endif

#include <uma/memory_pool_base.h>
#include <uma/memory_provider_base.h>

struct USMAllocatorParameters;

enum uma_result_t disjointPoolCreate(uma_memory_provider_handle_t hProvider,
                                        USMAllocatorParameters *params,
                                        uma_memory_pool_handle_t *out);

void disjointPoolDestroy(uma_memory_pool_handle_t hPool);

#ifdef __cplusplus
}
#endif
