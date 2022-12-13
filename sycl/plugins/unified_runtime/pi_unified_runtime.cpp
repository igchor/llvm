//===--- pi_unified_runtime.cpp - Unified Runtime PI Plugin  -----------==//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===------------------------------------------------------------------===//

#include <pi2ur.hpp>

extern "C" {
__SYCL_EXPORT pi_result piPlatformsGet(pi_uint32 num_entries,
                                       pi_platform *platforms,
                                       pi_uint32 *num_platforms) {
  return pi2ur::piPlatformsGet(num_entries, platforms, num_platforms);
}

pi_result piPluginInit(pi_plugin *PluginInit) {
  PI_ASSERT(PluginInit, PI_ERROR_INVALID_VALUE);

  // Check that the major version matches in PiVersion and SupportedVersion
  // TODO

  HANDLE_ERRORS(urInit(0, 0));

  (PluginInit->PiFunctionTable).piPlatformsGet = &piPlatformsGet;

  return PI_SUCCESS;
}

pi_result piTearDown(void *PluginParameter) {
  urTearDown(nullptr);
  return PI_SUCCESS;
}

} // extern "C
