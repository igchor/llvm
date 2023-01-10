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

#include <stddef.h>
#include <stdint.h>

#define UMA_VERSION_MAJOR 0
#define UMA_VERSION_MINOR 9

/// \brief Operation statuses
enum uma_result_t {
  /**
   * @brief Success.
   */
  UMA_RESULT_SUCCESS = 0,

  /**
   * @brief Error: Operation failed.
   */
  UMA_RESULT_OPERATION_FAILED = -1,

  /**
   * @brief Error: pointer argument may not be nullptr
   */
  UMA_RESULT_ERROR_INVALID_NULL_POINTER = -2,

  /**
   * @brief Error: handle argument is not valid
   */
  UMA_RESULT_ERROR_INVALID_NULL_HANDLE = -3,

  UMA_RESULT_INVALID_VALUE = -4,

  UMA_RESULT_ERROR_UNINITIALIZED = -5,

  /**
   * @brief Error: Unspecified run-time error.
   */
  UMA_RESULT_RUNTIME_ERROR = -255
};

#ifdef __cplusplus
}
#endif