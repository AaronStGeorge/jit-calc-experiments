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
#include "mlir/Target/LLVMIR/Export.h"
#include "llvm/LineEditor/LineEditor.h"
#include "llvm/Support/TargetSelect.h"
#include "llvm/Support/raw_ostream.h"

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

  // Parse static mlir function for adding two numbers
  std::string moduleStr = R"mlir(
  func.func @foo(%arg0 : i32, %arg1 : i32) -> i32 attributes { llvm.emit_c_interface } {
    %res = arith.addi %arg0, %arg1 : i32
    return %res : i32
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

  std::cout << "Welcome to jit-calc! ===================================="
            << std::endl;
  // Process unput until user sends ctrl+d end-of-transmission character
  llvm::LineEditor LineEditor("");
  while (auto line = LineEditor.readLine()) {
    if (!line) {
      continue;
    }
    // maximum-hack-overdrive parse of user input
    int firstOperand;
    int secondOperand;
    std::string op;
    std::stringstream(*line) >> firstOperand >> op >> secondOperand;

    if (op != "+") {
      std::cout << "only addition supported" << std::endl;
      continue;
    }

    // run our jit-ed function
    int result = 0;
    llvm::Error error = jit->invoke("foo", firstOperand, secondOperand,
                                    mlir::ExecutionEngine::Result<int>(result));
    if (error) {
      llvm::errs() << "calling jit-ed function returned error\n";
      return 1;
    }

    // And lo there was math
    std::cout << result << std::endl;
  }

  return 0;
}
