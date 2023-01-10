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

struct uma_memory_provider_ops_t {
  uint64_t version_major;
  uint64_t version_minor;

  enum uma_result_t (*initialize)(void *params, void **provider);
  void (*finalize)(void *pool);
  enum uma_result_t (*alloc)(void *provider, size_t size, size_t alignment,
                             void **ptr);
  enum uma_result_t (*free)(void *provider, void *ptr, size_t size);
};

#ifdef __cplusplus
}
#endif
