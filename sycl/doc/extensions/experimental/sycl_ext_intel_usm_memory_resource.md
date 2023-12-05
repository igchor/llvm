# sycl_ext_intel_usm_memory_resource

## Notice

This document describes an **experimental** API that applications can use to try
out a new feature. Future versions of this API may change in ways that are
incompatible with this experimental version.

## Introduction

This extension adds a new class that implements std::pmr::memory_resource for allocating Unified Shared Memory: `usm_memory_resource`. It can be used as an upstream provider in pool memory resources (e.g. `std::pmr::monotonic_buffer_resource`). `usm_memory_resource` can also be used with `std::pmr::polymorphic_allocator` to make container instances interoperable.

## Feature test macro

As encouraged by the SYCL specification, a feature-test macro,
`SYCL_EXT_INTEL_USM_MEMORY_RESOURCE`, is provided to determine
whether this extension is implemented.

## Synopsis

```c++
template <usm::alloc AllocKind>
class usm_memory_resource : public std::pmr::memory_resource {
public:
    usm_memory_resource() = delete;
    usm_memory_resource(const queue& syclQueue,
                        const property_list& propList = {});
    usm_memory_resource(const context& syclContext,
                        const device& syclDevice,
                        const property_list& propList = {});

    usm_memory_resource(const usm_memory_resource &);
    usm_memory_resource(usm_memory_resource &&) noexcept;
    usm_memory_resource &operator=(const usm_memory_resource &Other);
    usm_memory_resource &operator=(usm_memory_resource &&Other);

    void* do_allocate(std::size_t bytes, std::size_t alignment);
    void* do_deallocate(void* p, std::size_t, std::size_t);
    bool do_is_equal(const std::pmr::memory_resource& other) const noexcept;
};
```

## Examples

```c++
sycl::queue q = sycl::queue{sycl::gpu_selector_v};
sycl::ext::intel::experimental::usm_memory_resource<usm::alloc::host> usm_resource(q);

// usm_memory_resource can be used directly
std::pmr::vector<int> data1(&usm_resource);
auto *ptr1 = usm_resource.allocate(1024);

// usm_memory_resource can also be used as an upstream resource for pool resources
std::pmr::monotonic_buffer_resource buffer_resource(&usm_resource);
std::pmr::vector<int> data2(&buffer_resource);
auto *ptr2 = buffer_resource.allocate(1024);
```
