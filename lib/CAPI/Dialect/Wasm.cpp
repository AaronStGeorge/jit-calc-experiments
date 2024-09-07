#include "jit-calc-mlir-c/Dialect/Wasm.h"

#include "jit-calc-mlir/Dialect/Wasm/IR/WasmDialect.h"
#include "mlir/CAPI/Registration.h"

MLIR_DEFINE_CAPI_DIALECT_REGISTRATION(Wasm, wasm, mlir::wasm::WasmDialect)
