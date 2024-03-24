//===- StandaloneDialect.cpp - Standalone dialect ---------------*- C++ -*-===//
//
// This file is licensed under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "Wasm/WasmDialect.h"
#include "Wasm/WasmOps.h"

using namespace mlir;
using namespace mlir::wasm;

#include "Wasm/WasmOpsDialect.cpp.inc"

//===----------------------------------------------------------------------===//
// Standalone dialect.
//===----------------------------------------------------------------------===//

void WasmDialect::initialize() {
  addOperations<
#define GET_OP_LIST
#include "Wasm/WasmOps.cpp.inc"
      >();
}
