#ifndef WASM_WASMPASSES_H
#define WASM_WASMPASSES_H

#include "jit-calc-mlir/Dialect/Wasm/IR/WasmDialect.h"
#include "jit-calc-mlir/Dialect/Wasm/IR/WasmOps.h"
#include "mlir/Pass/Pass.h"
#include <memory>

namespace mlir {
namespace wasm {
#define GEN_PASS_DECL
#include "jit-calc-mlir/Dialect/Wasm/Transforms/Passes.h"

#define GEN_PASS_REGISTRATION
#include "jit-calc-mlir/Dialect/Wasm/Transforms/Passes.h.inc"
} // namespace wasm
} // namespace mlir

#endif // WASM_WASMPASSES_H
