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

#include <uma/memory_provider_base.h>
#include <ze_api.h>

typedef struct l0_memory_provider_config_t
    *l0_memory_provider_config_handle_t;

enum l0_memory_type {
  TypeDevice, TypeHost, TypeShared
};

enum l0_protection_t {
  Read, Write, None
};

l0_memory_provider_config_handle_t l0MemoryProviderConfigCreate();
void l0MemoryProviderConfigDestroy(l0_memory_provider_config_handle_t);

// TODO: we should accept handles to zeContext, zeDevice instead. This is just temporary implementation
// to limit number of changes inside the implementation
struct _pi_context;
using pi_context = struct _pi_context*;
struct _pi_device;
using pi_device = struct _pi_device*;
void l0MemoryProviderConfigSetContext(l0_memory_provider_config_handle_t, pi_context);
void l0MemoryProviderConfigSetDevice(l0_memory_provider_config_handle_t, pi_device);

void l0MemoryProviderConfigSetMemoryType(l0_memory_provider_config_handle_t, enum l0_memory_type);
void l0MemoryProviderConfigSetMemoryProtection(l0_memory_provider_config_handle_t, int);
// TODO: mem_properties?

enum uma_result_t
l0MemoryProviderCreate(l0_memory_provider_config_handle_t,
                          uma_memory_provider_handle_t *);
void l0MemoryProviderDestroy(uma_memory_provider_handle_t);

#ifdef __cplusplus
}
#endif
