#include "mlir/Conversion/ArithToLLVM/ArithToLLVM.h"
#include "mlir/Conversion/FuncToLLVM/ConvertFuncToLLVMPass.h"
#include "mlir/Conversion/MemRefToLLVM/MemRefToLLVM.h"
#include "mlir/Conversion/ReconcileUnrealizedCasts/ReconcileUnrealizedCasts.h"
#include "mlir/Conversion/VectorToLLVM/ConvertVectorToLLVM.h"
#include "mlir/Conversion/VectorToSCF/VectorToSCF.h"
#include "mlir/Dialect/Func/IR/FuncOps.h"
#include "mlir/Dialect/Linalg/Passes.h"
#include "mlir/ExecutionEngine/CRunnerUtils.h"
#include "mlir/ExecutionEngine/ExecutionEngine.h"
#include "mlir/ExecutionEngine/MemRefUtils.h"
#include "mlir/ExecutionEngine/RunnerUtils.h"
#include "mlir/IR/MLIRContext.h"
#include "mlir/InitAllDialects.h"
#include "mlir/Parser/Parser.h"
#include "mlir/Pass/PassManager.h"
#include "mlir/Target/LLVMIR/Dialect/Builtin/BuiltinToLLVMIRTranslation.h"
#include "mlir/Target/LLVMIR/Dialect/LLVMIR/LLVMToLLVMIRTranslation.h"
#include "llvm/Support/TargetSelect.h"

// steps
// -[ ] get an MLIR fibonacci to run through LLVM
// -[ ] take a look at the intermediate results get some idea to design the VM
// -[ ] create an initial version of the VM
//      - maybe create a standalone tool for running VM code via python parser with pyparsing?
//        - I'm sad that this is basically exactly where we were at like two years ago
// -[ ]
// -[ ] swap in and out of fibonacci at least once using library developed in count-to-n
// -[ ] compile MLIR fibonacci -> "vm dialect"
// -[ ] write an MLIR pass to insert the swap calls behind the hood after everything
//      - this will require figuring out what an "instruction" should be
//      - this step could just use a timer to decide when to swap back in
// -[ ] count number of instructions swap out when number is over

int main(int argc, char **argv) {
  // LLVM init
  llvm::InitializeNativeTarget();
  llvm::InitializeNativeTargetAsmPrinter();
  llvm::InitializeNativeTargetAsmParser();

  // Register MLIR dialects
  mlir::DialectRegistry registry;
  mlir::registerAllDialects(registry);
  mlir::registerBuiltinDialectTranslation(registry);
  mlir::registerLLVMDialectTranslation(registry);

  mlir::MLIRContext context(registry);

  // Create lowering pipeline
  mlir::PassManager pm(&context);
  pm.addPass(mlir::createFinalizeMemRefToLLVMConversionPass());
  pm.addNestedPass<mlir::func::FuncOp>(mlir::createArithToLLVMConversionPass());
  pm.addPass(mlir::createConvertFuncToLLVMPass());
  pm.addPass(mlir::createReconcileUnrealizedCastsPass());

  // Parse static mlir function
  std::string moduleStr = R"mlir(
  func.func @fib(%n : i32) -> i32 attributes { llvm.emit_c_interface } {
		%i = arith.constant 0 : i32
    return %i : i32
  }
  )mlir";
  mlir::OwningOpRef<mlir::ModuleOp> module =
      mlir::parseSourceString<mlir::ModuleOp>(moduleStr, &context);
  if (!module) {
    llvm::errs() << "module couldn't be created\n";
    return 1;
  }

  // run lowering pipeline
  if (failed(pm.run(*module))) {
    llvm::errs() << "lowering pipeline failed\n";
    return 1;
  }

  // create jit
  auto jitOrError = mlir::ExecutionEngine::create(*module);
  if (!jitOrError) {
    llvm::errs() << "execution engine couldn't be created\n";
    return 1;
  }
  std::unique_ptr<mlir::ExecutionEngine> jit = std::move(jitOrError.get());

  // run our jit-ed function
  int result = 0;
  llvm::Error error =
      jit->invoke("fib", 3, mlir::ExecutionEngine::Result<int>(result));
  if (error) {
    llvm::errs() << "calling jit-ed function returned error\n";
    return 1;
  }

	std::cout << result << std::endl;

  return 0;
}
