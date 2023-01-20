//===--------- uma_helpers.hpp - uma helpers for UR -------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===-----------------------------------------------------------------===//
#pragma once

#include <uma/memory_pool.h>
#include <uma/memory_pool_ops.h>
#include <uma/memory_provider.h>
#include <uma/memory_provider_ops.h>

#include <stdexcept>

using ur_memory_provider_handle_unique =
    std::unique_ptr<uma_memory_provider_t, decltype(&umaMemoryProviderDestroy)>;
using ur_memory_pool_handle_unique =
    std::unique_ptr<uma_memory_pool_t, decltype(&umaPoolDestroy)>;

template <typename MemProvider, typename... Args>
auto memoryProviderMakeUnique(Args &&...args) {
  struct uma_memory_provider_ops_t ops;
  ops.initialize = [](void *params, void **provider) {
    try {
      auto *tuple = reinterpret_cast<std::tuple<Args...> *>(params);
      *provider = std::apply(
          [](Args &&...args) {
            return new MemProvider(std::forward<Args>(args)...);
          },
          std::move(*tuple));
      return UMA_RESULT_SUCCESS;
    } catch (...) {
      return UMA_RESULT_RUNTIME_ERROR;
    }
  };
  ops.finalize = [](void *provider) {
    delete reinterpret_cast<MemProvider *>(provider);
  };
  ops.alloc = [](void *provider, size_t size, size_t alignment, void **ptr) {
    try {
      return reinterpret_cast<MemProvider *>(provider)->alloc(size, alignment,
                                                              ptr);
    } catch (...) {
      return UMA_RESULT_RUNTIME_ERROR;
    }
  };
  ops.free = [](void *provider, void *ptr, size_t size) {
    try {
      return reinterpret_cast<MemProvider *>(provider)->free(ptr, size);
    } catch (...) {
      return UMA_RESULT_RUNTIME_ERROR;
    }
  };

  auto tuple = std::make_tuple(std::forward<Args>(args)...);
  uma_memory_provider_handle_t hProvider;
  auto ret = umaMemoryProviderCreate(&ops, reinterpret_cast<void *>(&tuple),
                                     &hProvider);

  if (ret != UMA_RESULT_SUCCESS) {
    throw std::runtime_error("UMA error");
  }

  return ur_memory_provider_handle_unique(hProvider, &umaMemoryProviderDestroy);
}

template <typename MemPool, typename... Args>
auto poolMakeUnique(Args &&...args) {
  struct uma_memory_pool_ops_t ops;
  ops.initialize = [](void *params, void **pool) {
    try {
      auto *tuple = reinterpret_cast<std::tuple<Args...> *>(params);
      *pool = std::apply(
          [](Args &&...args) {
            return new MemPool(std::forward<Args>(args)...);
          },
          std::move(*tuple));
      return UMA_RESULT_SUCCESS;
    } catch (...) {
      return UMA_RESULT_RUNTIME_ERROR;
    }
  };
  ops.finalize = [](void *pool) { delete reinterpret_cast<MemPool *>(pool); };
  ops.malloc = [](void *pool, size_t size) {
    return reinterpret_cast<MemPool *>(pool)->malloc(size);
  };
  ops.calloc = [](void *pool, size_t num, size_t size) {
    return reinterpret_cast<MemPool *>(pool)->calloc(num, size);
  };
  ops.realloc = [](void *pool, void *ptr, size_t size) {
    return reinterpret_cast<MemPool *>(pool)->realloc(ptr, size);
  };
  ops.aligned_malloc = [](void *pool, size_t size, size_t alignment) {
    return reinterpret_cast<MemPool *>(pool)->aligned_malloc(size, alignment);
  };
  ops.malloc_usable_size = [](void *pool, void *ptr) {
    return reinterpret_cast<MemPool *>(pool)->malloc_usable_size(ptr);
  };
  ops.free = [](void *pool, void *ptr) {
    return reinterpret_cast<MemPool *>(pool)->free(ptr);
  };

  auto tuple = std::make_tuple(std::forward<Args>(args)...);
  uma_memory_pool_handle_t hPool;
  auto ret = umaPoolCreate(&ops, reinterpret_cast<void *>(&tuple), &hPool);

  if (ret != UMA_RESULT_SUCCESS) {
    throw std::runtime_error("UMA error");
  }

  return ur_memory_pool_handle_unique(hPool, &umaPoolDestroy);
}
