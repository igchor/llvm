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

#include <uma/base.h>
#include <uma/memory_pool_ops.h>

typedef struct uma_memory_pool_t *uma_memory_pool_handle_t;

///
/// \brief Creates new memory pool
/// \param ops instance of uma_memory_pool_ops_t
/// \param params pointer to pool-specific parameters
/// \return UMA_RESULT_SUCCESS on success or appropriate error code on failure
///
// TODO: perhaps pass pointer to `void (*init_ops)(struct
// uma_memory_pool_ops_t*)` so that user can call
// umaPoolCreate(&my_pool_init_ops, params, &pool) ?
enum uma_result_t umaPoolCreate(struct uma_memory_pool_ops_t *ops, void *params,
                                uma_memory_pool_handle_t *hPool);

///
/// \brief Destroys uma pool
/// \param hPool handle to the pool
///
void umaPoolDestroy(uma_memory_pool_handle_t hPool);

///
/// \brief Allocates size bytes of uninitialized storage of the specified hPool
/// \param hPool specified memory hPool
/// \param size number of bytes to allocate
/// \return Pointer to the allocated memory
///
void *umaPoolMalloc(uma_memory_pool_handle_t hPool, size_t size);

///
/// \brief Allocates size bytes of uninitialized storage of the specified hPool
/// with
///        specified alignment
/// \param hPool specified memory hPool
/// \param size number of bytes to allocate
/// \param alignment alignment of the allocation
/// \return Pointer to the allocated memory
///
void *umaPoolAlignedMalloc(uma_memory_pool_handle_t hPool, size_t size,
                           size_t alignment);

///
/// \brief Allocates memory of the specified hPool for an array of num elements
///        of size bytes each and initializes all bytes in the allocated storage
///        to zero
/// \param hPool specified memory hPool
/// \param num number of objects
/// \param size specified size of each element
/// \return Pointer to the allocated memory
///
void *umaPoolCalloc(uma_memory_pool_handle_t hPool, size_t num, size_t size);

///
/// \brief Reallocates memory of the specified hPool
/// \param hPool specified memory hPool
/// \param ptr pointer to the memory block to be reallocated
/// \param size new size for the memory block in bytes
/// \return Pointer to the allocated memory
///
void *umaPoolRealloc(uma_memory_pool_handle_t hPool, void *ptr, size_t size);

///
/// \brief obtain size of block of memory allocated from the pool
/// \param hPool specified memory hPool
/// \param ptr pointer to the allocated memory
/// \return Number of bytes
///
size_t umaPoolMallocUsabeSize(uma_memory_pool_handle_t hPool, void *ptr);

///
/// \brief Free the memory space of the specified hPool pointed by ptr
/// \param hPool specified memory hPool
/// \param ptr pointer to the allocated memory
///
void umaPoolFree(uma_memory_pool_handle_t hPool, void *ptr);

#ifdef __cplusplus
}
#endif
