; NOTE: Assertions have been autogenerated by utils/update_test_checks.py UTC_ARGS: --check-globals all --version 5
; RUN: opt -bugpoint-enable-legacy-pm -localaccessortosharedmemory %s -S -o - | FileCheck %s

target datalayout = "e-p:64:64-p1:64:64-p2:32:32-p3:32:32-p4:64:64-p5:32:32-p6:32:32-i64:64-v16:16-v24:32-v32:32-v48:64-v96:128-v192:256-v256:256-v512:512-v1024:1024-v2048:2048-n32:64-S32-A5-G1-ni:7"
target triple = "amdgcn-amd-amdhsa"

; This test checks that the transformation always bitcasts to the correct type.

; CHECK: @_ZTS14example_kernel_shared_mem = external addrspace(3) global [0 x i8], align 4
;.
define amdgpu_kernel void @_ZTS14example_kernel(ptr addrspace(3) %a, ptr addrspace(3) %b, ptr addrspace(3) %c, ptr addrspace(3) %d) {
; CHECK-LABEL: define amdgpu_kernel void @_ZTS14example_kernel(
; CHECK-SAME: i32 [[TMP0:%.*]], i32 [[TMP1:%.*]], i32 [[TMP2:%.*]], i32 [[TMP3:%.*]]) {
; CHECK-NEXT:  [[ENTRY:.*:]]
; CHECK-NEXT:    [[TMP4:%.*]] = getelementptr inbounds [0 x i8], ptr addrspace(3) @_ZTS14example_kernel_shared_mem, i32 0, i32 [[TMP3]]
; CHECK-NEXT:    [[D:%.*]] = bitcast ptr addrspace(3) [[TMP4]] to ptr addrspace(3)
; CHECK-NEXT:    [[TMP5:%.*]] = getelementptr inbounds [0 x i8], ptr addrspace(3) @_ZTS14example_kernel_shared_mem, i32 0, i32 [[TMP2]]
; CHECK-NEXT:    [[C:%.*]] = bitcast ptr addrspace(3) [[TMP5]] to ptr addrspace(3)
; CHECK-NEXT:    [[TMP6:%.*]] = getelementptr inbounds [0 x i8], ptr addrspace(3) @_ZTS14example_kernel_shared_mem, i32 0, i32 [[TMP1]]
; CHECK-NEXT:    [[B:%.*]] = bitcast ptr addrspace(3) [[TMP6]] to ptr addrspace(3)
; CHECK-NEXT:    [[TMP7:%.*]] = getelementptr inbounds [0 x i8], ptr addrspace(3) @_ZTS14example_kernel_shared_mem, i32 0, i32 [[TMP0]]
; CHECK-NEXT:    [[A:%.*]] = bitcast ptr addrspace(3) [[TMP7]] to ptr addrspace(3)
; CHECK-NEXT:    [[TMP8:%.*]] = load i32, ptr addrspace(3) [[A]], align 4
; CHECK-NEXT:    [[TMP9:%.*]] = load i64, ptr addrspace(3) [[B]], align 8
; CHECK-NEXT:    [[TMP10:%.*]] = load i16, ptr addrspace(3) [[C]], align 2
; CHECK-NEXT:    [[TMP11:%.*]] = load i8, ptr addrspace(3) [[D]], align 1
; CHECK-NEXT:    ret void
;
entry:
  %0 = load i32, ptr addrspace(3) %a
  %1 = load i64, ptr addrspace(3) %b
  %2 = load i16, ptr addrspace(3) %c
  %3 = load i8, ptr addrspace(3) %d
  ret void
}

!llvm.module.flags = !{!0}

!0 = !{i32 1, !"sycl-device", i32 1}
;.
; CHECK: [[META0:![0-9]+]] = !{i32 1, !"sycl-device", i32 1}
;.
