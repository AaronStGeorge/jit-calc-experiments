#include "Dialect/Wasm/IR/WasmDialect.h"
#include "Dialect/Wasm/IR/WasmOps.h"

using namespace mlir;
using namespace mlir::wasm;

#include "Dialect/Wasm/IR/WasmOpsDialect.cpp.inc"

//===----------------------------------------------------------------------===//
// Wasm dialect.
//===----------------------------------------------------------------------===//

void WasmDialect::initialize() {
  addOperations<
#define GET_OP_LIST
#include "Dialect/Wasm/IR/WasmOps.cpp.inc"
      >();
}
