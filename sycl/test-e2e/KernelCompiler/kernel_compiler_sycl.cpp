//==- kernel_compiler_sycl.cpp --- kernel_compiler extension tests   -------==//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

// REQUIRES: (opencl || level_zero)
// UNSUPPORTED: accelerator

// Flaky timeout on CPU. Enable when fixed.
// Depends on SPIR-V Backend & run-time drivers version.
// UNSUPPORTED: spirv-backend && cpu
// UNSUPPORTED-TRACKER: CMPLRLLVM-64705

// -- Test the kernel_compiler with SYCL source.
// RUN: %{build} -o %t.out

// Note: this 'invoking clang++' version for SYCL language support is temporary,
// and will be replaced by the SYCL_JIT version soon.

// RUN: %{run} %t.out
// RUN: %{l0_leak_check} %{run} %t.out

// -- Test again, with caching.
// 'reading-from-cache' is just a string we pass to differentiate between the
// two runs.

// DEFINE: %{cache_vars} = %{l0_leak_check} env SYCL_CACHE_PERSISTENT=1 SYCL_CACHE_TRACE=5 SYCL_CACHE_DIR=%t/cache_dir
// RUN: rm -rf %t/cache_dir
// RUN:  %{cache_vars} %t.out 2>&1 |  FileCheck %s --check-prefixes=CHECK-WRITTEN-TO-CACHE
// RUN:  %{cache_vars} %t.out reading-from-cache 2>&1 |  FileCheck %s --check-prefixes=CHECK-READ-FROM-CACHE

// -- Add leak check.
// RUN: rm -rf %t/cache_dir
// RUN: %{l0_leak_check} %{cache_vars} %t.out 2>&1 |  FileCheck %s --check-prefixes=CHECK-WRITTEN-TO-CACHE
// RUN: %{l0_leak_check} %{cache_vars} %t.out reading-from-cache 2>&1 |  FileCheck %s --check-prefixes=CHECK-READ-FROM-CACHE

// CHECK-WRITTEN-TO-CACHE: [Persistent Cache]: enabled
// CHECK-WRITTEN-TO-CACHE-NOT: [kernel_compiler Persistent Cache]: using cached binary
// CHECK-WRITTEN-TO-CACHE: [kernel_compiler Persistent Cache]: binary has been cached

// CHECK-READ-FROM-CACHE: [Persistent Cache]: enabled
// CHECK-READ-FROM-CACHE-NOT: [kernel_compiler Persistent Cache]: binary has been cached
// CHECK-READ-FROM-CACHE: [kernel_compiler Persistent Cache]: using cached binary

#include <sycl/detail/core.hpp>
#include <sycl/kernel_bundle.hpp>
#include <sycl/usm.hpp>

auto constexpr AddEmH = R"===(
  int AddEm(int a, int b){
    return a + b + 5;
  }
)===";

auto constexpr PlusEmH = R"===(
  int PlusEm(int a, int b){
    return a + b + 6;
  }
)===";

// TODO: remove SYCL_EXTERNAL once it is no longer needed.
auto constexpr SYCLSource = R"===(
#include <sycl/sycl.hpp>
#include "intermediate/AddEm.h"
#include "intermediate/PlusEm.h"

// use extern "C" to avoid name mangling
extern "C" SYCL_EXTERNAL SYCL_EXT_ONEAPI_FUNCTION_PROPERTY((sycl::ext::oneapi::experimental::nd_range_kernel<1>))
void ff_cp(int *ptr) {

  // intentionally using deprecated routine, as opposed to this_work_item::get_nd_item<1>()
  sycl::nd_item<1> Item = sycl::ext::oneapi::experimental::this_nd_item<1>();

  sycl::id<1> GId = Item.get_global_id();
  ptr[GId.get(0)] = AddEm(GId.get(0), 37);
}

// this name will be mangled
template <typename T>
SYCL_EXTERNAL SYCL_EXT_ONEAPI_FUNCTION_PROPERTY((sycl::ext::oneapi::experimental::nd_range_kernel<1>))
void ff_templated(T *ptr) {

  sycl::nd_item<1> Item = sycl::ext::oneapi::this_work_item::get_nd_item<1>();

  sycl::id<1> GId = Item.get_global_id();
  ptr[GId.get(0)] = PlusEm(GId.get(0), 38);
}
)===";

auto constexpr BadSource = R"===(
#include <sycl/sycl.hpp>

extern "C" SYCL_EXTERNAL SYCL_EXT_ONEAPI_FUNCTION_PROPERTY((sycl::ext::oneapi::experimental::nd_range_kernel<1>))
void ff_cp(int *ptr) {

  sycl::nd_item<1> Item = sycl::ext::oneapi::this_work_item::get_nd_item<1>();

  sycl::id<1> GId = Item.get_global_id() + no semi colon !!
  ptr[GId.get(0)] = GId.get(0) + 41;
}
)===";

auto constexpr ESIMDSource = R"===(
#include <sycl/sycl.hpp>
#include <sycl/ext/intel/esimd.hpp>

using namespace sycl::ext::intel::esimd;

constexpr int VL = 16;

extern "C" SYCL_EXTERNAL SYCL_ESIMD_KERNEL SYCL_EXT_ONEAPI_FUNCTION_PROPERTY((sycl::ext::oneapi::experimental::nd_range_kernel<1>))
void vector_add_esimd(float *A, float *B, float *C) {
    sycl::nd_item<1> item = sycl::ext::oneapi::this_work_item::get_nd_item<1>();
    unsigned int i = item.get_global_id(0);
    unsigned int offset = i * VL ;

    simd<float, VL> va(A + offset);
    simd<float, VL> vb(B + offset);
    simd<float, VL> vc = va + vb;
    vc.copy_to(C + offset);
}
)===";

void test_1(sycl::queue &Queue, sycl::kernel &Kernel, int seed) {
  constexpr int Range = 10;
  int *usmPtr = sycl::malloc_shared<int>(Range, Queue);
  int start = 3;

  sycl::nd_range<1> R1{{Range}, {1}};

  bool Passa = true;

  memset(usmPtr, 0, Range * sizeof(int));
  Queue.submit([&](sycl::handler &Handler) {
    Handler.set_arg(0, usmPtr);
    Handler.parallel_for(R1, Kernel);
  });
  Queue.wait();

  for (int i = 0; i < Range; i++) {
    std::cout << usmPtr[i] << "=" << (i + seed) << " ";
    assert(usmPtr[i] == i + seed);
  }
  std::cout << std::endl;

  sycl::free(usmPtr, Queue);
}

void test_build_and_run(bool readingFromCache) {
  namespace syclex = sycl::ext::oneapi::experimental;
  using source_kb = sycl::kernel_bundle<sycl::bundle_state::ext_oneapi_source>;
  using exe_kb = sycl::kernel_bundle<sycl::bundle_state::executable>;

  sycl::queue q;
  sycl::context ctx = q.get_context();

  bool ok =
      q.get_device().ext_oneapi_can_compile(syclex::source_language::sycl);
  if (!ok) {
    std::cout << "Apparently this device does not support SYCL source "
                 "kernel bundle extension: "
              << q.get_device().get_info<sycl::info::device::name>()
              << std::endl;
    return;
  }

  // Create from source.
  syclex::include_files incFiles{"intermediate/AddEm.h", AddEmH};
  incFiles.add("intermediate/PlusEm.h", PlusEmH);
  source_kb kbSrc = syclex::create_kernel_bundle_from_source(
      ctx, syclex::source_language::sycl, SYCLSource,
      syclex::properties{incFiles});

  // Double check kernel_bundle.get_source() / get_backend().
  sycl::context ctxRes = kbSrc.get_context();
  assert(ctxRes == ctx);
  sycl::backend beRes = kbSrc.get_backend();
  assert(beRes == ctx.get_backend());

  // Compilation of empty prop list, no devices.
  exe_kb kbExe1 = syclex::build(kbSrc);

  // Compilation with props and devices
  std::string log;
  std::vector<std::string> flags{"-g", "-fno-fast-math"};
  std::vector<sycl::device> devs = kbSrc.get_devices();
  exe_kb kbExe2 = syclex::build(
      kbSrc, devs,
      syclex::properties{syclex::build_options{flags}, syclex::save_log{&log},
                         syclex::registered_kernel_names{"ff_templated<int>"}});

  // If the kernel was restored from cache, there will not have been
  // any warning issued by the compilation of the kernel.
  if (!readingFromCache) {
    assert(log.find("warning: 'this_nd_item<1>' is deprecated") !=
           std::string::npos);
  }

  // clang-format off

  // extern "C" was used, so the name "ff_cp" is not mangled and can be used directly.
  sycl::kernel k = kbExe2.ext_oneapi_get_kernel("ff_cp");

  // The templated function name will have been mangled. Mapping from original
  // name to mangled is not yet supported. So we cannot yet do this:
  // sycl::kernel k2 = kbExe2.ext_oneapi_get_kernel("ff_templated<int>");

  // Instead, we can TEMPORARILY use the mangled name. Once demangling is supported
  // this might no longer work.
  sycl::kernel k2 = kbExe2.ext_oneapi_get_kernel("_Z26__sycl_kernel_ff_templatedIiEvPT_");

  // clang-format on

  // Test the kernels.
  test_1(q, k, 37 + 5);  // ff_cp seeds 37. AddEm will add 5 more.
  test_1(q, k2, 38 + 6); // ff_templated seeds 38. PlusEm adds 6 more.
}

void test_error() {
  namespace syclex = sycl::ext::oneapi::experimental;
  using source_kb = sycl::kernel_bundle<sycl::bundle_state::ext_oneapi_source>;
  using exe_kb = sycl::kernel_bundle<sycl::bundle_state::executable>;

  sycl::queue q;
  sycl::context ctx = q.get_context();

  bool ok =
      q.get_device().ext_oneapi_can_compile(syclex::source_language::sycl);
  if (!ok) {
    return;
  }

  source_kb kbSrc = syclex::create_kernel_bundle_from_source(
      ctx, syclex::source_language::sycl, BadSource);
  try {
    exe_kb kbExe = syclex::build(kbSrc);
    assert(false && "we should not be here");
  } catch (sycl::exception &e) {
    // yas!
    assert(e.code() == sycl::errc::build);
    assert(std::string(e.what()).find(
               "error: expected ';' at end of declaration") !=
           std::string::npos);
  }
}

void test_esimd() {
  namespace syclex = sycl::ext::oneapi::experimental;
  using source_kb = sycl::kernel_bundle<sycl::bundle_state::ext_oneapi_source>;
  using exe_kb = sycl::kernel_bundle<sycl::bundle_state::executable>;

  sycl::queue q;
  sycl::context ctx = q.get_context();

  if (!q.get_device().has(sycl::aspect::ext_intel_esimd)) {
    std::cout << "Device does not support ESIMD" << std::endl;
    return;
  }

  bool ok =
      q.get_device().ext_oneapi_can_compile(syclex::source_language::sycl);
  if (!ok) {
    return;
  }

  std::string log;

  source_kb kbSrc = syclex::create_kernel_bundle_from_source(
      ctx, syclex::source_language::sycl, ESIMDSource);
  exe_kb kbExe =
      syclex::build(kbSrc, syclex::properties{syclex::save_log{&log}});

  // extern "C" was used, so the name "vector_add_esimd" is not mangled and can
  // be used directly.
  sycl::kernel k = kbExe.ext_oneapi_get_kernel("vector_add_esimd");

  // Now test it.
  constexpr int VL = 16; // this constant also in ESIMDSource string.
  constexpr int size = VL * 16;

  float *A = sycl::malloc_shared<float>(size, q);
  float *B = sycl::malloc_shared<float>(size, q);
  float *C = sycl::malloc_shared<float>(size, q);
  for (size_t i = 0; i < size; i++) {
    A[i] = float(1);
    B[i] = float(2);
    C[i] = 0.0f;
  }
  sycl::range<1> GlobalRange{size / VL};
  sycl::range<1> LocalRange{1};
  sycl::nd_range<1> NDRange{GlobalRange, LocalRange};

  q.submit([&](sycl::handler &h) {
     h.set_arg(0, A);
     h.set_arg(1, B);
     h.set_arg(2, C);
     h.parallel_for(NDRange, k);
   }).wait();

  // Check.
  for (size_t i = 0; i < size; i++) {
    assert(C[i] == 3.0f);
  }

  sycl::free(A, q);
  sycl::free(B, q);
  sycl::free(C, q);
}

int main(int argc, char *argv[]) {
  namespace syclex = sycl::ext::oneapi::experimental;
  bool readingFromCache = false;

  // Check if the argument is present
  if (argc > 1) {
    std::string argument(argv[1]);
    if (argument == "reading-from-cache") {
      readingFromCache = true;
    } else if (argument == "available") {
      sycl::device d;
      bool avail = d.ext_oneapi_can_compile(syclex::source_language::sycl);
      return avail;
    }
  }

#ifdef SYCL_EXT_ONEAPI_KERNEL_COMPILER
  test_build_and_run(readingFromCache);
  test_error();
  test_esimd();
#else
  static_assert(false, "Kernel Compiler feature test macro undefined");
#endif
  return 0;
}
