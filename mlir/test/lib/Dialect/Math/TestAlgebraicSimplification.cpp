//===- TestAlgebraicSimplification.cpp - Test algebraic simplification ----===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
// This file contains test passes for algebraic simplification patterns.
//
//===----------------------------------------------------------------------===//

#include "mlir/Dialect/Math/IR/Math.h"
#include "mlir/Dialect/Math/Transforms/Passes.h"
#include "mlir/Dialect/Vector/IR/VectorOps.h"
#include "mlir/Pass/Pass.h"
#include "mlir/Transforms/GreedyPatternRewriteDriver.h"

using namespace mlir;

namespace {
struct TestMathAlgebraicSimplificationPass
    : public PassWrapper<TestMathAlgebraicSimplificationPass, OperationPass<>> {
  MLIR_DEFINE_EXPLICIT_INTERNAL_INLINE_TYPE_ID(
      TestMathAlgebraicSimplificationPass)

  void runOnOperation() override;
  void getDependentDialects(DialectRegistry &registry) const override {
    registry.insert<vector::VectorDialect, math::MathDialect>();
  }
  StringRef getArgument() const final {
    return "test-math-algebraic-simplification";
  }
  StringRef getDescription() const final {
    return "Test math algebraic simplification";
  }
};
} // namespace

void TestMathAlgebraicSimplificationPass::runOnOperation() {
  RewritePatternSet patterns(&getContext());
  populateMathAlgebraicSimplificationPatterns(patterns);
  (void)applyPatternsGreedily(getOperation(), std::move(patterns));
}

namespace mlir {
namespace test {
void registerTestMathAlgebraicSimplificationPass() {
  PassRegistration<TestMathAlgebraicSimplificationPass>();
}
} // namespace test
} // namespace mlir
