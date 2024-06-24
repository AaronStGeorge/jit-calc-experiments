//===- Passes.h - Conversion Pass Construction and Registration -----------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#ifndef JIT_CALC_EXPERIMENTS_CONVERSION_PASSES_H
#define JIT_CALC_EXPERIMENTS_CONVERSION_PASSES_H

#include "Conversion/TowerRuntimeToLLVM/TowerRuntimeToLLVM.h"

namespace mlir {
namespace jitCalcExperiments {

/// Generate the code for registering conversion passes.
#define GEN_PASS_REGISTRATION
#include "Conversion/Passes.h.inc"

} // namespace jitCalcExperiments
} // namespace mlir

#endif // JIT_CALC_EXPERIMENTS_CONVERSION_PASSES_H
