//===- StandaloneOps.cpp - Standalone dialect ops ---------------*- C++ -*-===//
//
// This file is licensed under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "jit-calc-mlir/Dialect/TowerRuntime/IR/TowerRuntimeDialect.h"
#include "jit-calc-mlir/Dialect/TowerRuntime/IR/TowerRuntimeOps.h"

#define GET_OP_CLASSES
#include "jit-calc-mlir/Dialect/TowerRuntime/IR/TowerRuntimeOps.cpp.inc"
