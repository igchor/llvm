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

#include <uma/memory_pool_base.h>

static struct uma_memory_pool_ops_t *ops(uma_memory_pool_handle_t hPool) {
  return (struct uma_memory_pool_ops_t *)hPool;
}

void *umaPoolMalloc(uma_memory_pool_handle_t hPool, size_t size) {
  return ops(hPool)->malloc(hPool, size);
}

void *umaPoolAlignedMalloc(uma_memory_pool_handle_t hPool, size_t size,
                           size_t alignment) {
  return ops(hPool)->aligned_malloc(hPool, size, alignment);
}

void *umaPoolCalloc(uma_memory_pool_handle_t hPool, size_t num, size_t size) {
  return ops(hPool)->calloc(hPool, num, size);
}

void *umaPoolRealloc(uma_memory_pool_handle_t hPool, void *ptr, size_t size) {
  return ops(hPool)->realloc(hPool, ptr, size);
}

size_t umaPoolMallocUsabeSize(uma_memory_pool_handle_t hPool, void *ptr) {
  return ops(hPool)->malloc_usable_size(hPool, ptr);
}

void umaPoolFree(uma_memory_pool_handle_t hPool, void *ptr) {
  return ops(hPool)->free(hPool, ptr);
}
