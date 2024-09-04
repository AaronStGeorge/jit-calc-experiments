//===- StandaloneTypes.cpp - Standalone dialect types -----------*- C++ -*-===//
//
// This file is licensed under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "jit-calc-mlir/Dialect/TowerRuntime/IR/TowerRuntimeTypes.h"

#include "jit-calc-mlir/Dialect/TowerRuntime/IR/TowerRuntimeDialect.h"
#include "mlir/IR/Builders.h"
#include "mlir/IR/DialectImplementation.h"
#include "llvm/ADT/TypeSwitch.h"

using namespace mlir::towerRuntime;

#define GET_TYPEDEF_CLASSES
#include "jit-calc-mlir/Dialect/TowerRuntime/IR/TowerRuntimeOpsTypes.cpp.inc"

void TowerRuntimeDialect::registerTypes() {
  addTypes<
#define GET_TYPEDEF_LIST
#include "jit-calc-mlir/Dialect/TowerRuntime/IR/TowerRuntimeOpsTypes.cpp.inc"
      >();
}
