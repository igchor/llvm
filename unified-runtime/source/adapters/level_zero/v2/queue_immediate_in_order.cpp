//===--------- queue_immediate_in_order.cpp - Level Zero Adapter ---------===//
//
// Copyright (C) 2024 Intel Corporation
//
// Part of the Unified-Runtime Project, under the Apache License v2.0 with LLVM
// Exceptions. See LICENSE.TXT
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "queue_immediate_in_order.hpp"
#include "command_buffer.hpp"
#include "kernel.hpp"
#include "memory.hpp"
#include "ur.hpp"

#include "../common/latency_tracker.hpp"
#include "../helpers/kernel_helpers.hpp"
#include "../image_common.hpp"

#include "../program.hpp"
#include "../ur_interface_loader.hpp"

namespace v2 {

ur_queue_immediate_in_order_t::ur_queue_immediate_in_order_t(
    ur_context_handle_t hContext, ur_device_handle_t hDevice,
    uint32_t ordinal, ze_command_queue_priority_t priority, std::optional<int32_t> index, event_flags_t eventFlags, ur_queue_flags_t flags)
    : ur_command_list_manager(
          hContext, hDevice,
          std::make_unique<single_command_list_provider>(
              hContext->getCommandListCache().getImmediateCommandList(
                  hDevice->ZeDevice,
                  {true, ordinal,
                   true /* always enable copy offload */},
                  ZE_COMMAND_QUEUE_MODE_ASYNCHRONOUS,
                  priority,
                  index)),
                  eventFlags),
      flags(flags) {}

ur_queue_immediate_in_order_t::ur_queue_immediate_in_order_t(
    ur_context_handle_t hContext, ur_device_handle_t hDevice,
    ur_native_handle_t hNativeHandle, event_flags_t eventFlags, ur_queue_flags_t flags, bool ownZeQueue)
    : ur_command_list_manager(
          hContext, hDevice,
          std::make_unique<single_command_list_provider>(
              raii::command_list_unique_handle(
                  reinterpret_cast<ze_command_list_handle_t>(hNativeHandle),
                  [ownZeQueue](ze_command_list_handle_t hZeCommandList) {
                    if (ownZeQueue) {
                      if (checkL0LoaderTeardown()) {
                        ZE_CALL_NOCHECK(zeCommandListDestroy, (hZeCommandList));
                      }
                    }
                  })),
          eventFlags),
      flags(flags) {}

ur_result_t
ur_queue_immediate_in_order_t::queueGetInfo(ur_queue_info_t propName,
                                            size_t propSize, void *pPropValue,
                                            size_t *pPropSizeRet) {
  UrReturnHelper ReturnValue(propSize, pPropValue, pPropSizeRet);
  // TODO: consider support for queue properties and size
  switch ((uint32_t)propName) { // cast to avoid warnings on EXT enum values
  case UR_QUEUE_INFO_CONTEXT:
    return ReturnValue(hContext);
  case UR_QUEUE_INFO_DEVICE:
    return ReturnValue(hDevice);
  case UR_QUEUE_INFO_REFERENCE_COUNT:
    return ReturnValue(uint32_t{RefCount.load()});
  case UR_QUEUE_INFO_FLAGS:
    return ReturnValue(flags);
  case UR_QUEUE_INFO_SIZE:
  case UR_QUEUE_INFO_DEVICE_DEFAULT:
    return UR_RESULT_ERROR_UNSUPPORTED_ENUMERATION;
  case UR_QUEUE_INFO_EMPTY: {
    auto status =
        ZE_CALL_NOCHECK(zeCommandListHostSynchronize, (getZeCommandList(), 0));
    if (status == ZE_RESULT_SUCCESS) {
      return ReturnValue(true);
    } else if (status == ZE_RESULT_NOT_READY) {
      return ReturnValue(false);
    } else {
      return ze2urResult(status);
    }
  }
  default:
    UR_LOG(ERR,
           "Unsupported ParamName in urQueueGetInfo: "
           "ParamName=ParamName={}(0x{})",
           propName, logger::toHex(propName));
    return UR_RESULT_ERROR_INVALID_VALUE;
  }

  return UR_RESULT_SUCCESS;
}

ur_result_t ur_queue_immediate_in_order_t::queueGetNativeHandle(
    ur_queue_native_desc_t * /*pDesc*/, ur_native_handle_t *phNativeQueue) {
  *phNativeQueue = reinterpret_cast<ur_native_handle_t>(getZeCommandList());
  return UR_RESULT_SUCCESS;
}

ur_result_t ur_queue_immediate_in_order_t::queueFinish() {
  TRACK_SCOPE_LATENCY("ur_queue_immediate_in_order_t::queueFinish");

  ZE2UR_CALL(zeCommandListHostSynchronize, (getZeCommandList(), UINT64_MAX));

  UR_CALL(releaseSubmittedKernels());

  return UR_RESULT_SUCCESS;
}

ur_result_t ur_queue_immediate_in_order_t::queueFlush() {
  return UR_RESULT_SUCCESS;
}

ur_queue_immediate_in_order_t::~ur_queue_immediate_in_order_t() {
  try {
    UR_CALL_THROWS(queueFinish());
  } catch (...) {
    // Ignore errors during destruction
  }
}

ur_result_t ur_queue_immediate_in_order_t::enqueueEventsWait(
    uint32_t numEventsInWaitList, const ur_event_handle_t *phEventWaitList,
    ur_event_handle_t *phEvent) {
  TRACK_SCOPE_LATENCY("ur_queue_immediate_in_order_t::enqueueEventsWait");

  if (!numEventsInWaitList && !phEvent) {
    // nop
    return UR_RESULT_SUCCESS;
  }

  auto zeSignalEvent = getSignalEvent(phEvent, UR_COMMAND_EVENTS_WAIT);
  auto [pWaitEvents, numWaitEvents] =
      getWaitListView(phEventWaitList, numEventsInWaitList);
  auto zeCommandList = getZeCommandList();

  if (numWaitEvents > 0) {
    ZE2UR_CALL(zeCommandListAppendWaitOnEvents,
               (zeCommandList, numWaitEvents, pWaitEvents));
  }

  if (zeSignalEvent) {
    ZE2UR_CALL(zeCommandListAppendSignalEvent, (zeCommandList, zeSignalEvent));
  }
  return UR_RESULT_SUCCESS;
}

ur_result_t ur_queue_immediate_in_order_t::enqueueEventsWaitWithBarrierImpl(
    uint32_t numEventsInWaitList, const ur_event_handle_t *phEventWaitList,
    ur_event_handle_t *phEvent) {
  TRACK_SCOPE_LATENCY(
      "ur_queue_immediate_in_order_t::enqueueEventsWaitWithBarrier");

  if (!numEventsInWaitList && !phEvent) {
    // nop
    return UR_RESULT_SUCCESS;
  }

  auto zeSignalEvent =
      getSignalEvent(phEvent, UR_COMMAND_EVENTS_WAIT_WITH_BARRIER);
  auto [pWaitEvents, numWaitEvents] =
      getWaitListView(phEventWaitList, numEventsInWaitList);
  auto zeCommandList = getZeCommandList();

  ZE2UR_CALL(zeCommandListAppendBarrier,
             (zeCommandList, zeSignalEvent, numWaitEvents, pWaitEvents));

  return UR_RESULT_SUCCESS;
}

ur_result_t ur_queue_immediate_in_order_t::enqueueEventsWaitWithBarrier(
    uint32_t numEventsInWaitList, const ur_event_handle_t *phEventWaitList,
    ur_event_handle_t *phEvent) {
  // For in-order queue we don't need a real barrier, just wait for
  // requested events in potentially different queues and add a "barrier"
  // event signal because it is already guaranteed that previous commands
  // in this queue are completed when the signal is started. However, we do
  // need to use barrier if profiling is enabled: see
  // zeCommandListAppendWaitOnEvents
  if ((flags & UR_QUEUE_FLAG_PROFILING_ENABLE) != 0) {
    return enqueueEventsWaitWithBarrierImpl(numEventsInWaitList,
                                            phEventWaitList, phEvent);
  } else {
    return enqueueEventsWait(numEventsInWaitList, phEventWaitList, phEvent);
  }
}

ur_result_t ur_queue_immediate_in_order_t::enqueueEventsWaitWithBarrierExt(
    const ur_exp_enqueue_ext_properties_t *, uint32_t numEventsInWaitList,
    const ur_event_handle_t *phEventWaitList, ur_event_handle_t *phEvent) {
  return enqueueEventsWaitWithBarrier(numEventsInWaitList, phEventWaitList,
                                      phEvent);
}

} // namespace v2
