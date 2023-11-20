#include "mlir/Conversion/ArithToLLVM/ArithToLLVM.h"
#include "mlir/Conversion/ControlFlowToLLVM/ControlFlowToLLVM.h"
#include "mlir/Conversion/FuncToLLVM/ConvertFuncToLLVMPass.h"
#include "mlir/Conversion/MemRefToLLVM/MemRefToLLVM.h"
#include "mlir/Conversion/ReconcileUnrealizedCasts/ReconcileUnrealizedCasts.h"
#include "mlir/Conversion/SCFToControlFlow/SCFToControlFlow.h"
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
#include <cstdint>

// steps
// -[x] get an MLIR fibonacci to run through LLVM
// -[ ] take a look at the intermediate results get some idea to design the VM
//      - "the VM" here is basically a (or maybe a set of) MLIR dialects where
//        each MLIR operation is considered an operation on the computers game
//        code runs on. So, we can say if each computer gets 400 cycles that
//        equates to 400 operations if each operation is equally costly.
//      - this could be existing MLIR dialects or a new one that maps fairly
//        straight-forwardly to LLVM
// -[ ] create an initial version of the VM
//      - maybe create a standalone tool for parsing VM IR with pyparsing or
//      something?
// -[ ] lower MLIR fibonacci to VM dialect(s).
// -[ ] write an MLIR pass to insert the swap calls behind the hood after
//      everything
// -[ ] swap in and out of fibonacci at least once using library developed in
//      count-to-n maybe just swapping out every instruction
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
  pm.addPass(mlir::createConvertSCFToCFPass());
  pm.addPass(mlir::createConvertControlFlowToLLVMPass());
  pm.addPass(mlir::createConvertVectorToLLVMPass());
  pm.addPass(mlir::createFinalizeMemRefToLLVMConversionPass());
  pm.addNestedPass<mlir::func::FuncOp>(mlir::createArithToLLVMConversionPass());
  pm.addPass(mlir::createConvertFuncToLLVMPass());
  pm.addPass(mlir::createReconcileUnrealizedCastsPass());

  // Parse static mlir function
  std::string moduleStr = R"mlir(
  func.func @fib(%n : index) -> i32 attributes { llvm.emit_c_interface } {
    %c0_index	 = arith.constant 0 : index
    %c1_index	 = arith.constant 1 : index

    %c0_i32	 = arith.constant 0 : i32
    %c1_i32	 = arith.constant 1 : i32

    %i = memref.alloca() : memref<i32>
    memref.store %c0_i32, %i[] : memref<i32>

    %j = memref.alloca() : memref<i32>
    memref.store %c1_i32, %j[] : memref<i32>

    scf.for %c = %c0_index to %n step %c1_index {
      %i_load = memref.load %i[] : memref<i32>
      %j_load = memref.load %j[] : memref<i32>
      %tmp = arith.addi %i_load, %j_load : i32
      memref.store %j_load, %i[] : memref<i32>
      memref.store %tmp, %j[] : memref<i32>
    }

    %0 = memref.load %i[] : memref<i32>
    return %0 : i32
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

  // Process unput until user sends ctrl+d end-of-transmission character
  while (!std::cin.eof()) {
    // maximum-hack-overdrive parse of user input
    uint64_t n;
    std::string op;
    std::cout << "> ";
    std::cin >> n;

    // error checking
    if (std::cin.eof()) {
      continue;
    }

    // run our jit-ed function
    int result = 0;
    llvm::Error error =
        jit->invoke("fib", n, mlir::ExecutionEngine::Result<int>(result));
    if (error) {
      llvm::errs() << "calling jit-ed function returned error\n";
      return 1;
    }

    // And lo there was math
    std::cout << result << std::endl;
  }

  std::cout << std::endl;
  return 0;
}
