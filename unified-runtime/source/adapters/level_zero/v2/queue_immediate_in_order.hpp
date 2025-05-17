//===--------- queue_immediate_in_order.hpp - Level Zero Adapter ---------===//
//
// Copyright (C) 2024 Intel Corporation
//
// Part of the Unified-Runtime Project, under the Apache License v2.0 with LLVM
// Exceptions. See LICENSE.TXT
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
#pragma once

#include "../common.hpp"
#include "../device.hpp"

#include "context.hpp"
#include "event.hpp"
#include "event_pool_cache.hpp"
#include "memory.hpp"
#include "queue_api.hpp"

#include "ur/ur.hpp"

#include "command_list_manager.hpp"
#include "lockable.hpp"

namespace v2 {

using queue_group_type = ur_device_handle_t_::queue_group_info_t::type;

struct ur_queue_immediate_in_order_t : ur_command_list_manager {
private:
  ur_queue_flags_t flags;

  ur_result_t
  enqueueEventsWaitWithBarrierImpl(uint32_t numEventsInWaitList,
                                   const ur_event_handle_t *phEventWaitList,
                                   ur_event_handle_t *phEvent);

public:
  ur_queue_immediate_in_order_t(ur_context_handle_t, ur_device_handle_t,
                                uint32_t ordinal, ze_command_queue_priority_t priority, std::optional<int32_t> index, event_flags_t eventFlags, ur_queue_flags_t flags);
  ur_queue_immediate_in_order_t(ur_context_handle_t, ur_device_handle_t,
                                ur_native_handle_t, event_flags_t, ur_queue_flags_t,
                                bool ownZeQueue);

  ~ur_queue_immediate_in_order_t();

  ur_result_t queueGetInfo(ur_queue_info_t propName, size_t propSize,
                           void *pPropValue, size_t *pPropSizeRet) override;
  ur_result_t queueGetNativeHandle(ur_queue_native_desc_t *pDesc,
                                   ur_native_handle_t *phNativeQueue) override;
  ur_result_t queueFinish() override;
  ur_result_t queueFlush() override;

  ur_result_t enqueueEventsWait(uint32_t numEventsInWaitList,
                                const ur_event_handle_t *phEventWaitList,
                                ur_event_handle_t *phEvent) override;
  ur_result_t
  enqueueEventsWaitWithBarrier(uint32_t numEventsInWaitList,
                               const ur_event_handle_t *phEventWaitList,
                               ur_event_handle_t *phEvent) override;
  ur_result_t enqueueEventsWaitWithBarrierExt(
      const ur_exp_enqueue_ext_properties_t *pProperties,
      uint32_t numEventsInWaitList, const ur_event_handle_t *phEventWaitList,
      ur_event_handle_t *phEvent) override;
};

} // namespace v2
