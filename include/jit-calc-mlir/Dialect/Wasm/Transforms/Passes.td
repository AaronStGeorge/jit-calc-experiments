#ifndef WASM_PASS
#define WASM_PASS

include "mlir/Pass/PassBase.td"

def LowerWasm: Pass<"lower-wasm", "::mlir::ModuleOp"> {
  let summary = "TODO";
  let description = [{
    TODO
  }];
  let dependentDialects = ["arith::ArithDialect"];
}

#endif // WASM_PASS
