//===- StandaloneDialect.cpp - Standalone dialect ---------------*- C++ -*-===//
//
// This file is licensed under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "Wasm/StandaloneDialect.h"
#include "Wasm/StandaloneOps.h"
#include "Wasm/StandaloneTypes.h"

using namespace mlir;
using namespace mlir::standalone;

#include "Wasm/StandaloneOpsDialect.cpp.inc"

//===----------------------------------------------------------------------===//
// Standalone dialect.
//===----------------------------------------------------------------------===//

void StandaloneDialect::initialize() {
  addOperations<
#define GET_OP_LIST
#include "Wasm/StandaloneOps.cpp.inc"
      >();
  registerTypes();
}
