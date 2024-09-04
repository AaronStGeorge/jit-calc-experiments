#include "jit-calc-mlir/Dialect/Wasm/IR/WasmDialect.h"
#include "jit-calc-mlir/Dialect/Wasm/IR/WasmOps.h"

using namespace mlir;
using namespace mlir::wasm;

#include "jit-calc-mlir/Dialect/Wasm/IR/WasmOpsDialect.cpp.inc"

//===----------------------------------------------------------------------===//
// Wasm dialect.
//===----------------------------------------------------------------------===//

void WasmDialect::initialize() {
  addOperations<
#define GET_OP_LIST
#include "jit-calc-mlir/Dialect/Wasm/IR/WasmOps.cpp.inc"
      >();
}
