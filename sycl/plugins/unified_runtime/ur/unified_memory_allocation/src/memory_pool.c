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

#include <uma/memory_pool.h>

#include <stdlib.h>

struct uma_memory_pool_t {
  struct uma_memory_pool_ops_t ops;
  void *pool_priv;
};

enum uma_result_t umaPoolCreate(struct uma_memory_pool_ops_t *ops, void *params,
                                uma_memory_pool_handle_t *hPool) {
  uma_memory_pool_handle_t pool = malloc(sizeof(struct uma_memory_pool_t));
  if (!pool) {
    return UMA_RESULT_RUNTIME_ERROR;
  }

  // TODO: handle any version mismatches
  pool->ops = *ops;

  void *pool_priv;
  enum uma_result_t ret = ops->initialize(params, &pool_priv);
  if (ret != UMA_RESULT_SUCCESS) {
    return ret;
  }

  pool->pool_priv = pool_priv;

  *hPool = pool;

  return UMA_RESULT_SUCCESS;
}

void umaPoolDestroy(uma_memory_pool_handle_t hPool) {
  hPool->ops.finalize(hPool->pool_priv);
  free(hPool);
}

void *umaPoolMalloc(uma_memory_pool_handle_t hPool, size_t size) {
  return hPool->ops.malloc(hPool->pool_priv, size);
}

void *umaPoolAlignedMalloc(uma_memory_pool_handle_t hPool, size_t size,
                           size_t alignment) {
  return hPool->ops.aligned_malloc(hPool->pool_priv, size, alignment);
}

void *umaPoolCalloc(uma_memory_pool_handle_t hPool, size_t num, size_t size) {
  return hPool->ops.calloc(hPool->pool_priv, num, size);
}

void *umaPoolRealloc(uma_memory_pool_handle_t hPool, void *ptr, size_t size) {
  return hPool->ops.realloc(hPool->pool_priv, ptr, size);
}

size_t umaPoolMallocUsabeSize(uma_memory_pool_handle_t hPool, void *ptr) {
  return hPool->ops.malloc_usable_size(hPool->pool_priv, ptr);
}

void umaPoolFree(uma_memory_pool_handle_t hPool, void *ptr) {
  hPool->ops.free(hPool->pool_priv, ptr);
}
