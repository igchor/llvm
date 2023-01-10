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

#include "usm_allocator.hpp"

#include <uma/memory_pool_base.h>
#include <uma/memory_pools/disjoint_pool.h>

class MemoryProviderWrapper : public SystemMemory {
public:
  MemoryProviderWrapper(usm_memory_prrovider_handle_t hProvider): hProvider(hProvider) {
  }

  void *allocate(size_t size) override {
    return allocate(size, 0);
  }

  void *allocate(size_t size, size_t aligned) override {
    void* ptr;
    
    if (usmMemoryProviderAlloc(hProvider, size, aligned, &ptr) != UMA_RESULT_SUCCESS) {
      throw std::runtime_error("TODO");
    }

    return ptr;
  }

  void deallocate(void *ptr) override {
    if (usmMemoryProviderFree(hProvider, ptr, 0) != UMA_RESULT_SUCCESS) {
      throw std::runtime_error("TODO");
    }
  }

private:
  usm_memory_prrovider_handle_t hProvider;
};

extern "C" {

struct disjoint_pool_t {
    disjoint_pool_t(USMAllocatorParameters params): ctx(params) {
      pool->ops.alloc = [](uma_memory_pool_handle_t hPool, size_t size) {
        reinterpret_cast<disjoint_pool_t*>(hPool)->ctx.allocate(size);
      };

      pool->ops.aligned_malloc = [](uma_memory_pool_handle_t hPool, size_t size, size_t align) {
        reinterpret_cast<disjoint_pool_t*>(hPool)->ctx.allocate(size, align);
      };

      pool->ops.free = [](uma_memory_pool_handle_t hPool, void *ptr) {
        reinterpret_cast<disjoint_pool_t*>(hPool)->ctx.free(ptr);
      };

      pool->ops.calloc = nullptr;
      pool->ops.realloc = nullptr;
      pool->ops.malloc_usable_size = nullptr;
    }

    uma_memory_pool_ops_t ops;
    USMAllocContext ctx;
};

enum uma_result_t umaDisjointPoolCreate(uma_memory_provider_handle_t hProvider,
                                        USMAllocatorParameters *params,
                                        uma_memory_pool_handle_t *out) {
  auto memHandle = std::unique_ptr<SystemMemory>(new MemoryProviderWrapper(hProvider));
  if (!memHandle) {
    return UMA_RESULT_OPERATION_FAILED;
  }

  auto hPool = new disjoint_pool_t(*params);
  if (!pool) {
    return UMA_RESULT_OPERATION_FAILED;
  }

  *out = reinterpret_cast<uma_memory_pool_handle_t>(hPool);
  return UMA_RESULT_SUCCESS;
}

void umaUSMPoolDestroy(uma_memory_pool_handle_t hPool) {
  delete reinterpret_cast<disjoint_pool_t *>(hPool);
}
}
