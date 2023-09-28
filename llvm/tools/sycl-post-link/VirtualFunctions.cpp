//===- VirtualFunctions.h - SYCL special handling for virtual functions ---===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "VirtualFunctions.h"

#include <llvm/ADT/StringSet.h>

using namespace llvm;

PreservedAnalyses VirtualFunctionsPass::run(Module &M,
                                            ModuleAnalysisManager &MAM) {
  SmallVector<Function*, 8> Kernels;
  SmallVector<Function*, 8> IndirectlyCallableFuncs;

  for (Function &FR : M) {
    Function *F = &FR;
    if (F->isDeclaration())
      continue;

    if (F->getCallingConv() == CallingConv::SPIR_KERNEL)
      Kernels.push_back(F);
    else if (F->hasFnAttribute("indirectly-callable"))
      IndirectlyCallableFuncs.push_back(F);
  }

  if (!Kernels.empty()) {
    StringSet<> Sets;

    for (Function *F : Kernels) {
      Sets.insert(F->getFnAttribute("calls-indirectly").getValueAsString());
    }

    NamedMDNode *MD = M.getOrInsertNamedMetadata("calls-indirectly");
    SmallVector<Metadata *, 4> MDOps;
    for (auto &It : Sets)
      MDOps.push_back(MDString::get(M.getContext(), It.getKey()));

    MD->addOperand(MDNode::get(M.getContext(), MDOps));
  } else {
    StringRef Set = IndirectlyCallableFuncs.front()
                        ->getFnAttribute("indirectly-callable")
                        .getValueAsString();

    // This is to prevent those functions from being dropped by GlobalDCE, since
    // they could have linkonce_odr linkage type with no uses
    for (Function *F : IndirectlyCallableFuncs)
      F->setLinkage(GlobalValue::LinkageTypes::ExternalLinkage);

    NamedMDNode *MD = M.getOrInsertNamedMetadata("indirectly-callable");
    MD->addOperand(
        MDNode::get(M.getContext(), {MDString::get(M.getContext(), Set)}));
  }

  return PreservedAnalyses::none(); // FIXME: be more precise here
}
