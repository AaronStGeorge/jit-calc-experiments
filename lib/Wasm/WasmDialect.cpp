#include "Wasm/WasmDialect.h"
#include "Wasm/WasmOps.h"

using namespace mlir;
using namespace mlir::wasm;

#include "Wasm/WasmOpsDialect.cpp.inc"

//===----------------------------------------------------------------------===//
// Wasm dialect.
//===----------------------------------------------------------------------===//

void WasmDialect::initialize() {
  addOperations<
#define GET_OP_LIST
#include "Wasm/WasmOps.cpp.inc"
      >();
}
