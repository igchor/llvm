//===- usm_memory_resource.hpp --------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#pragma once

#include <memory_resource>

#include <sycl/context.hpp>
#include <sycl/device.hpp>
#include <sycl/exception.hpp>
#include <sycl/queue.hpp>
#include <sycl/usm.hpp>

namespace sycl {
inline namespace _V1 {
namespace ext::intel::experimental {

template <usm::alloc AllocKind>
class usm_memory_resource : public std::pmr::memory_resource {
public:
  usm_memory_resource() = delete;

  usm_memory_resource(const queue &syclQueue,
                      const property_list &propList = {})
      : MContext(Q.get_context()), MDevice(Q.get_device()),
        MPropList(PropList) {}
  usm_memory_resource(const context &syclContext, const device &syclDevice,
                      const property_list &propList = {})
      : MContext(Ctxt), MDevice(Dev), MPropList(PropList) {}

  usm_memory_resource(const usm_memory_resource &) = default;
  usm_memory_resource(usm_memory_resource &&) noexcept = default;
  usm_memory_resource &operator=(const usm_memory_resource &Other) = default;
  usm_memory_resource &operator=(usm_memory_resource &&Other) = default;

  void *do_allocate(std::size_t bytes, std::size_t alignment) {
    auto Result = aligned_alloc(alignment, bytes, MDevice, MContext, AllocKind,
                                MPropList);
    if (!Result) {
      throw memory_allocation_error();
    }
    return Result;
  }

  void *do_deallocate(void *p, std::size_t, std::size_t) {
    if (p) {
      free(p, MContext);
    }
  }

  bool do_is_equal(const std::pmr::memory_resource &other) const noexcept {
    return this == &other;
  }

private:
  context MContext;
  device MDevice;
  property_list MPropList;
};

} // namespace ext::intel::experimental
} // namespace _V1
} // namespace sycl
