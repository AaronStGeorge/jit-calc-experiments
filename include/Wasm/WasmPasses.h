#ifndef WASM_WASMPASSES_H
#define WASM_WASMPASSES_H

#include "Wasm/WasmDialect.h"
#include "Wasm/WasmOps.h"
#include "mlir/Pass/Pass.h"
#include <memory>

namespace mlir {
namespace wasm {
#define GEN_PASS_DECL
#include "Wasm/WasmPasses.h.inc"

#define GEN_PASS_REGISTRATION
#include "Wasm/WasmPasses.h.inc"
} // namespace wasm
} // namespace mlir

#endif // WASM_WASMPASSES_H
