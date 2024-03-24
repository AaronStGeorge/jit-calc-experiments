//===- StandaloneDialect.cpp - Standalone dialect ---------------*- C++ -*-===//
//
// This file is licensed under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "Wasm/StandaloneDialect.h"
#include "Wasm/StandaloneOps.h"

using namespace mlir;
using namespace mlir::wasm;

#include "Wasm/StandaloneOpsDialect.cpp.inc"

//===----------------------------------------------------------------------===//
// Standalone dialect.
//===----------------------------------------------------------------------===//

void WasmDialect::initialize() {
  addOperations<
#define GET_OP_LIST
#include "Wasm/StandaloneOps.cpp.inc"
      >();
}
