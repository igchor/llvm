//==---- DefaultValues.cpp --- Spec constants default values unit test -----==//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include <detail/kernel_bundle_impl.hpp>
#include <sycl/sycl.hpp>

#include <helpers/PiImage.hpp>
#include <helpers/PiMock.hpp>

#include <gtest/gtest.h>

class TestKernel;
class TestKernelExeOnly;

namespace sycl {
__SYCL_INLINE_VER_NAMESPACE(_V1) {
namespace detail {
template <> struct KernelInfo<TestKernel> {
  static constexpr unsigned getNumParams() { return 0; }
  static const kernel_param_desc_t &getParamDesc(int) {
    static kernel_param_desc_t Dummy;
    return Dummy;
  }
  static constexpr const char *getName() { return "TestKernel"; }
  static constexpr bool isESIMD() { return false; }
  static constexpr bool callsThisItem() { return false; }
  static constexpr bool callsAnyThisFreeFunction() { return false; }
  static constexpr int64_t getKernelSize() { return 1; }
};

template <> struct KernelInfo<TestKernelExeOnly> {
  static constexpr unsigned getNumParams() { return 0; }
  static const kernel_param_desc_t &getParamDesc(int) {
    static kernel_param_desc_t Dummy;
    return Dummy;
  }
  static constexpr const char *getName() { return "TestKernelExeOnly"; }
  static constexpr bool isESIMD() { return false; }
  static constexpr bool callsThisItem() { return false; }
  static constexpr bool callsAnyThisFreeFunction() { return false; }
  static constexpr int64_t getKernelSize() { return 1; }
};

} // namespace detail
} // __SYCL_INLINE_VER_NAMESPACE(_V1)
} // namespace sycl

static sycl::unittest::PiImage
generateDefaultImage(std::initializer_list<std::string> KernelNames,
                     pi_device_binary_type BinaryType,
                     const char *DeviceTargetSpec) {
  using namespace sycl::unittest;

  PiPropertySet PropSet;

  std::vector<unsigned char> Bin{0, 1, 2, 3, 4, 5}; // Random data

  PiArray<PiOffloadEntry> Entries = makeEmptyKernels(KernelNames);

  PiImage Img{BinaryType, // Format
              DeviceTargetSpec,
              "", // Compile options
              "", // Link options
              std::move(Bin),
              std::move(Entries),
              std::move(PropSet)};

  return Img;
}

static sycl::unittest::PiImage Imgs[3] = {
    generateDefaultImage({"TestKernel"}, PI_DEVICE_BINARY_TYPE_SPIRV,
                         __SYCL_PI_DEVICE_BINARY_TARGET_SPIRV64),
    generateDefaultImage({"TestKernelExeOnly"}, PI_DEVICE_BINARY_TYPE_NATIVE,
                         __SYCL_PI_DEVICE_BINARY_TARGET_SPIRV64_X86_64),
    // A device image without entires
    generateDefaultImage({},
                         PI_DEVICE_BINARY_TYPE_NATIVE,
                         __SYCL_PI_DEVICE_BINARY_TARGET_SPIRV64_X86_64)};
static sycl::unittest::PiImageArray<3> ImgArray{Imgs};

TEST(KernelBundle, GetKernelBundleFromKernel) {
  sycl::unittest::PiMock Mock;

  const sycl::device Dev = Mock.getPlatform().get_devices()[0];
  sycl::context Ctx{Dev};
  sycl::queue Queue{Ctx, Dev};

  sycl::kernel_bundle<sycl::bundle_state::executable> KernelBundle =
      sycl::get_kernel_bundle<sycl::bundle_state::executable>(Ctx, {Dev});

  sycl::kernel Kernel =
      KernelBundle.get_kernel(sycl::get_kernel_id<TestKernel>());

  sycl::kernel_bundle<sycl::bundle_state::executable> RetKernelBundle =
      Kernel.get_kernel_bundle();

  EXPECT_EQ(KernelBundle, RetKernelBundle);
}

TEST(KernelBundle, KernelBundleAndItsDevImageStateConsistency) {
  sycl::unittest::PiMock Mock;

  const sycl::device Dev = Mock.getPlatform().get_devices()[0];

  sycl::queue Queue{Dev};

  const sycl::context Ctx = Queue.get_context();

  sycl::kernel_bundle KernelBundle =
      sycl::get_kernel_bundle<TestKernel, sycl::bundle_state::input>(Ctx,
                                                                     {Dev});

  auto ObjBundle = sycl::compile(KernelBundle, KernelBundle.get_devices());
  EXPECT_FALSE(ObjBundle.empty()) << "Expect non-empty obj kernel bundle";

  auto ObjBundleImpl = sycl::detail::getSyclObjImpl(ObjBundle);
  EXPECT_EQ(ObjBundleImpl->get_bundle_state(), sycl::bundle_state::object)
      << "Expect object device image in bundle";

  auto LinkBundle = sycl::link(ObjBundle, ObjBundle.get_devices());
  EXPECT_FALSE(LinkBundle.empty()) << "Expect non-empty exec kernel bundle";

  auto LinkBundleImpl = sycl::detail::getSyclObjImpl(LinkBundle);
  EXPECT_EQ(LinkBundleImpl->get_bundle_state(), sycl::bundle_state::executable)
      << "Expect executable device image in bundle";
}

TEST(KernelBundle, EmptyKernelBundle) {
  sycl::unittest::PiMock Mock;

  const sycl::device Dev = Mock.getPlatform().get_devices()[0];
  sycl::queue Queue{Dev};
  const sycl::context Ctx = Queue.get_context();

  auto EmptyKernelBundle =
      sycl::get_kernel_bundle<sycl::bundle_state::executable>(Ctx, {Dev}, {});

  EXPECT_TRUE(EmptyKernelBundle.empty());
  EXPECT_EQ(std::distance(EmptyKernelBundle.begin(), EmptyKernelBundle.end()),
            0u);
}

TEST(KernelBundle, EmptyKernelBundleKernelLaunchException) {
  sycl::unittest::PiMock Mock;

  const sycl::device Dev = Mock.getPlatform().get_devices()[0];
  sycl::queue Queue{Dev};
  const sycl::context Ctx = Queue.get_context();

  auto EmptyKernelBundle =
      sycl::get_kernel_bundle<sycl::bundle_state::executable>(Ctx, {Dev}, {});

  class UnqiueException {};

  try {
    Queue.submit([&](sycl::handler &CGH) {
      CGH.use_kernel_bundle(EmptyKernelBundle);

      try {
        CGH.single_task<TestKernel>([]() {});
        FAIL() << "No exception was thrown.";
      } catch (const sycl::exception &e) {
        ASSERT_EQ(e.code().value(),
                  static_cast<int>(sycl::errc::kernel_not_supported))
            << "sycl::exception code was not the expected "
               "sycl::errc::kernel_not_supported.";
        // Throw uniquely identifiable exception to distinguish between that
        // the sycl::exception originates from the correct level.
        throw UnqiueException{};
      } catch (...) {
        FAIL()
            << "Unexpected exception was thrown in kernel invocation function.";
      }
    });
  } catch (const UnqiueException &) {
    // Expected path
  } catch (const sycl::exception &) {
    FAIL() << "sycl::exception thrown at the wrong level.";
  } catch (...) {
    FAIL() << "Unexpected exception was thrown in submit.";
  }
}

TEST(KernelBundle, HasKernelBundle) {
  sycl::unittest::PiMock Mock;

  const sycl::device Dev = Mock.getPlatform().get_devices()[0];

  sycl::queue Queue{Dev};

  const sycl::context Ctx = Queue.get_context();

  bool HasKernelBundle =
      sycl::has_kernel_bundle<sycl::bundle_state::input>(Ctx, {Dev});
  EXPECT_TRUE(HasKernelBundle);
  HasKernelBundle =
      sycl::has_kernel_bundle<sycl::bundle_state::object>(Ctx, {Dev});
  EXPECT_TRUE(HasKernelBundle);
  HasKernelBundle =
      sycl::has_kernel_bundle<sycl::bundle_state::executable>(Ctx, {Dev});
  EXPECT_TRUE(HasKernelBundle);

  HasKernelBundle =
      sycl::has_kernel_bundle<TestKernel, sycl::bundle_state::input>(Ctx,
                                                                     {Dev});
  EXPECT_TRUE(HasKernelBundle);
  HasKernelBundle =
      sycl::has_kernel_bundle<TestKernel, sycl::bundle_state::object>(Ctx,
                                                                      {Dev});
  EXPECT_TRUE(HasKernelBundle);
  HasKernelBundle =
      sycl::has_kernel_bundle<TestKernel, sycl::bundle_state::executable>(
          Ctx, {Dev});
  EXPECT_TRUE(HasKernelBundle);

  HasKernelBundle =
      sycl::has_kernel_bundle<TestKernelExeOnly, sycl::bundle_state::input>(
          Ctx, {Dev});
  EXPECT_FALSE(HasKernelBundle);
  HasKernelBundle =
      sycl::has_kernel_bundle<TestKernelExeOnly, sycl::bundle_state::object>(
          Ctx, {Dev});
  EXPECT_FALSE(HasKernelBundle);
  HasKernelBundle =
      sycl::has_kernel_bundle<TestKernelExeOnly,
                              sycl::bundle_state::executable>(Ctx, {Dev});
  EXPECT_TRUE(HasKernelBundle);
}

TEST(KernelBundle, UseKernelBundleWrongContextPrimaryQueueOnly) {
  sycl::unittest::PiMock Mock;

  const sycl::device Dev = Mock.getPlatform().get_devices()[0];
  sycl::queue Queue{Dev};
  const sycl::context QueueCtx = Queue.get_context();
  const sycl::context OtherCtx{Dev};

  ASSERT_NE(QueueCtx, OtherCtx);

  auto KernelBundle =
      sycl::get_kernel_bundle<sycl::bundle_state::executable>(OtherCtx, {Dev});

  class UnqiueException {};

  try {
    Queue.submit([&](sycl::handler &CGH) {
      try {
        CGH.use_kernel_bundle(KernelBundle);
        FAIL() << "No exception was thrown.";
        CGH.single_task<TestKernel>([]() {});
      } catch (const sycl::exception &e) {
        ASSERT_EQ(e.code().value(), static_cast<int>(sycl::errc::invalid))
            << "sycl::exception code was not the expected sycl::errc::invalid.";
        // Throw uniquely identifiable exception to distinguish between that
        // the sycl::exception originates from the correct level.
        throw UnqiueException{};
      } catch (...) {
        FAIL()
            << "Unexpected exception was thrown in kernel invocation function.";
      }
    });
  } catch (const UnqiueException &) {
    // Expected path
  } catch (const sycl::exception &) {
    FAIL() << "sycl::exception thrown at the wrong level.";
  } catch (...) {
    FAIL() << "Unexpected exception was thrown in submit.";
  }
}

TEST(KernelBundle, UseKernelBundleWrongContextPrimaryQueueValidSecondaryQueue) {
  sycl::unittest::PiMock Mock;

  const sycl::device Dev = Mock.getPlatform().get_devices()[0];
  const sycl::context PrimaryCtx{Dev};
  const sycl::context SecondaryCtx{Dev};

  ASSERT_NE(PrimaryCtx, SecondaryCtx);

  auto KernelBundle = sycl::get_kernel_bundle<sycl::bundle_state::executable>(
      SecondaryCtx, {Dev});

  sycl::queue PrimaryQueue{PrimaryCtx, Dev};
  sycl::queue SecondaryQueue{SecondaryCtx, Dev};

  class UnqiueException {};

  try {
    PrimaryQueue.submit(
        [&](sycl::handler &CGH) {
          try {
            CGH.use_kernel_bundle(KernelBundle);
            FAIL() << "No exception was thrown.";
            CGH.single_task<TestKernel>([]() {});
          } catch (const sycl::exception &e) {
            ASSERT_EQ(e.code().value(), static_cast<int>(sycl::errc::invalid))
                << "sycl::exception code was not the expected "
                   "sycl::errc::invalid.";
            // Throw uniquely identifiable exception to distinguish between that
            // the sycl::exception originates from the correct level.
            throw UnqiueException{};
          } catch (...) {
            FAIL() << "Unexpected exception was thrown in kernel invocation "
                      "function.";
          }
        },
        SecondaryQueue);
  } catch (const UnqiueException &) {
    // Expected path
  } catch (const sycl::exception &) {
    FAIL() << "sycl::exception thrown at the wrong level.";
  } catch (...) {
    FAIL() << "Unexpected exception was thrown in submit.";
  }
}

TEST(KernelBundle, UseKernelBundleValidPrimaryQueueWrongContextSecondaryQueue) {
  sycl::unittest::PiMock Mock;

  const sycl::device Dev = Mock.getPlatform().get_devices()[0];
  const sycl::context PrimaryCtx{Dev};
  const sycl::context SecondaryCtx{Dev};

  ASSERT_NE(PrimaryCtx, SecondaryCtx);

  auto KernelBundle = sycl::get_kernel_bundle<sycl::bundle_state::executable>(
      PrimaryCtx, {Dev});

  sycl::queue PrimaryQueue{PrimaryCtx, Dev};
  sycl::queue SecondaryQueue{SecondaryCtx, Dev};

  class UnqiueException {};

  try {
    PrimaryQueue.submit(
        [&](sycl::handler &CGH) {
          try {
            CGH.use_kernel_bundle(KernelBundle);
            FAIL() << "No exception was thrown.";
            CGH.single_task<TestKernel>([]() {});
          } catch (const sycl::exception &e) {
            ASSERT_EQ(e.code().value(), static_cast<int>(sycl::errc::invalid))
                << "sycl::exception code was not the expected "
                   "sycl::errc::invalid.";
            // Throw uniquely identifiable exception to distinguish between that
            // the sycl::exception originates from the correct level.
            throw UnqiueException{};
          } catch (...) {
            FAIL() << "Unexpected exception was thrown in kernel invocation "
                      "function.";
          }
        },
        SecondaryQueue);
  } catch (const UnqiueException &) {
    // Expected path
  } catch (const sycl::exception &) {
    FAIL() << "sycl::exception thrown at the wrong level.";
  } catch (...) {
    FAIL() << "Unexpected exception was thrown in submit.";
  }
}

TEST(KernelBundle, UseKernelBundleWrongContextPrimaryQueueAndSecondaryQueue) {
  sycl::unittest::PiMock Mock;

  const sycl::device Dev = Mock.getPlatform().get_devices()[0];
  const sycl::context PrimaryCtx{Dev};
  const sycl::context SecondaryCtx{Dev};
  const sycl::context OtherCtx{Dev};

  ASSERT_NE(PrimaryCtx, SecondaryCtx);
  ASSERT_NE(PrimaryCtx, OtherCtx);
  ASSERT_NE(SecondaryCtx, OtherCtx);

  auto KernelBundle =
      sycl::get_kernel_bundle<sycl::bundle_state::executable>(OtherCtx, {Dev});

  sycl::queue PrimaryQueue{PrimaryCtx, Dev};
  sycl::queue SecondaryQueue{SecondaryCtx, Dev};

  class UnqiueException {};

  try {
    PrimaryQueue.submit(
        [&](sycl::handler &CGH) {
          try {
            CGH.use_kernel_bundle(KernelBundle);
            FAIL() << "No exception was thrown.";
            CGH.single_task<TestKernel>([]() {});
          } catch (const sycl::exception &e) {
            ASSERT_EQ(e.code().value(), static_cast<int>(sycl::errc::invalid))
                << "sycl::exception code was not the expected "
                   "sycl::errc::invalid.";
            // Throw uniquely identifiable exception to distinguish between that
            // the sycl::exception originates from the correct level.
            throw UnqiueException{};
          } catch (...) {
            FAIL() << "Unexpected exception was thrown in kernel invocation "
                      "function.";
          }
        },
        SecondaryQueue);
  } catch (const UnqiueException &) {
    // Expected path
  } catch (const sycl::exception &) {
    FAIL() << "sycl::exception thrown at the wrong level.";
  } catch (...) {
    FAIL() << "Unexpected exception was thrown in submit.";
  }
}

TEST(KernelBundle, EmptyDevicesKernelBundleLinkException) {
  sycl::unittest::PiMock Mock;

  const sycl::device Dev = Mock.getPlatform().get_devices()[0];
  sycl::queue Queue{Dev};
  const sycl::context Ctx = Queue.get_context();

  auto EmptyKernelBundle =
      sycl::get_kernel_bundle<sycl::bundle_state::object>(Ctx, {Dev}, {});

  std::vector<sycl::device> EmptyDevices{};

  // Case without an object bundle.
  try {
    auto LinkBundle = sycl::link(EmptyKernelBundle, EmptyDevices);
    FAIL() << "No exception was thrown.";
  } catch (const sycl::exception &e) {
    ASSERT_EQ(e.code().value(), static_cast<int>(sycl::errc::invalid))
        << "sycl::exception code was not the expected "
           "sycl::errc::invalid.";
    // Expected path
  } catch (...) {
    FAIL() << "Unexpected exception was thrown in sycl::link.";
  }

  sycl::kernel_bundle KernelBundle =
      sycl::get_kernel_bundle<TestKernel, sycl::bundle_state::input>(Ctx,
                                                                     {Dev});

  auto ObjBundle = sycl::compile(KernelBundle, KernelBundle.get_devices());
  EXPECT_FALSE(ObjBundle.empty()) << "Expect non-empty obj kernel bundle";

  try {
    auto LinkBundle = sycl::link(ObjBundle, EmptyDevices);
    FAIL() << "No exception was thrown.";
  } catch (const sycl::exception &e) {
    ASSERT_EQ(e.code().value(), static_cast<int>(sycl::errc::invalid))
        << "sycl::exception code was not the expected "
           "sycl::errc::invalid.";
    // Expected path
  } catch (...) {
    FAIL() << "Unexpected exception was thrown in sycl::link.";
  }
}
