add_mlir_dialect(WasmOps wasm)

set(LLVM_TARGET_DEFINITIONS WasmPasses.td)
mlir_tablegen(WasmPasses.h.inc --gen-pass-decls)
add_public_tablegen_target(MLIRWasmPassesIncGen)
