#ifndef WASM_WASMOPS_H
#define WASM_WASMOPS_H

#include "mlir/IR/BuiltinTypes.h"
#include "mlir/IR/Dialect.h"
#include "mlir/IR/OpDefinition.h"
#include "mlir/Interfaces/InferTypeOpInterface.h"
#include "mlir/Interfaces/SideEffectInterfaces.h"

#define GET_OP_CLASSES
#include "Wasm/WasmOps.h.inc"

#endif // WASM_WASMOPS_H
