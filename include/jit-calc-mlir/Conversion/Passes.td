//===-- Passes.td - Conversion pass definition file --------*- tablegen -*-===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#ifndef JIT_CALC_EXPERIMENTS_CONVERSION_PASSES
#define JIT_CALC_EXPERIMENTS_CONVERSION_PASSES

include "mlir/Pass/PassBase.td"

//===----------------------------------------------------------------------===//
// TowerRuntimeToLLVM
//===----------------------------------------------------------------------===//

def TowerRuntimeToLLVMConversionPass : Pass<"convert-tower-runtime-to-llvm", "ModuleOp"> {
  let summary = "Convert TowerRuntime dialect to LLVM dialect";
  let description = [{
    This pass converts supported TowerRuntime ops to LLVM dialect instructions.
  }];
  let dependentDialects = [
    "LLVM::LLVMDialect",
    "scf::SCFDialect",
  ];
}

#endif // JIT_CALC_EXPERIMENTS_CONVERSION_PASSES
