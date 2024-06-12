#ifndef WASM_WASMPASSES_H
#define WASM_WASMPASSES_H

#include "Dialect/Wasm/IR/WasmDialect.h"
#include "Dialect/Wasm/IR/WasmOps.h"
#include "mlir/Pass/Pass.h"
#include <memory>

namespace mlir {
namespace wasm {
#define GEN_PASS_DECL
#include "Dialect/Wasm/Transforms/Passes.h"

#define GEN_PASS_REGISTRATION
#include "Dialect/Wasm/Transforms/Passes.h.inc"
} // namespace wasm
} // namespace mlir

#endif // WASM_WASMPASSES_H
