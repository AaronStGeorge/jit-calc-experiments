//===- StandaloneDialect.cpp - Standalone dialect ---------------*- C++ -*-===//
//
// This file is licensed under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "jit-calc-mlir/Dialect/TowerRuntime/IR/TowerRuntimeDialect.h"
#include "jit-calc-mlir/Dialect/TowerRuntime/IR/TowerRuntimeOps.h"
#include "jit-calc-mlir/Dialect/TowerRuntime/IR/TowerRuntimeTypes.h"

using namespace mlir;
using namespace mlir::towerRuntime;

#include "jit-calc-mlir/Dialect/TowerRuntime/IR/TowerRuntimeOpsDialect.cpp.inc"

//===----------------------------------------------------------------------===//
// Standalone dialect.
//===----------------------------------------------------------------------===//

void TowerRuntimeDialect::initialize() {
  addOperations<
#define GET_OP_LIST
#include "jit-calc-mlir/Dialect/TowerRuntime/IR/TowerRuntimeOps.cpp.inc"
      >();
  registerTypes();
}
