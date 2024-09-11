#ifndef TOWER_RUNTIME_PASS
#define TOWER_RUNTIME_PASS

include "mlir/Pass/PassBase.td"

def ShimFunction: Pass<"shim-function", "::mlir::ModuleOp"> {
  let summary = "TODO";
  let description = [{
    TODO
  }];
  let dependentDialects = ["LLVM::LLVMDialect"];
  let options = [
    Option<"functionToShim", "function-to-shim", "std::string",
      /*default=*/"\"\"",
      "Create shim function for interacting with C code. Shim function loads tower runtime context from pointer.">
  ];
}

#endif // TOWER_RUNTIME_PASS
