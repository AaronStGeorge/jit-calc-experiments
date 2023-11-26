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
#include "mlir/Target/LLVMIR/Export.h"

#include "llvm/ExecutionEngine/Orc/Core.h"
#include "llvm/ExecutionEngine/Orc/LLJIT.h"
#include "llvm/ExecutionEngine/Orc/Shared/ExecutorAddress.h"
#include "llvm/ExecutionEngine/Orc/ThreadSafeModule.h"
#include "llvm/LineEditor/LineEditor.h"
#include "llvm/Support/Error.h"
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
// -[ ] swap in and out of fibonacci at least once using library developed in
//      count-to-n maybe just swapping out every instruction
// -[ ] create an initial version of the "VM"
//      - maybe create a standalone tool for parsing VM IR with pyparsing or
//      something?
// -[ ] lower MLIR fibonacci to VM dialect(s).
// -[ ] write an MLIR pass to insert the swap calls behind the hood after
//      everything
// -[ ] count number of instructions swap out when number is over

void myFunction() { std::cout << "called" << std::endl; }

llvm::orc::ThreadSafeModule compileFibExample() {
  // Register MLIR dialects
  mlir::DialectRegistry registry;
  mlir::registerAllDialects(registry);
  mlir::registerBuiltinDialectTranslation(registry);
  mlir::registerLLVMDialectTranslation(registry);

  mlir::MLIRContext mlirContext(registry);

  // Create lowering pipeline
  mlir::PassManager pm(&mlirContext);
  pm.addPass(mlir::createConvertSCFToCFPass());
  pm.addPass(mlir::createConvertControlFlowToLLVMPass());
  pm.addPass(mlir::createConvertVectorToLLVMPass());
  pm.addPass(mlir::createFinalizeMemRefToLLVMConversionPass());
  pm.addNestedPass<mlir::func::FuncOp>(mlir::createArithToLLVMConversionPass());
  pm.addPass(mlir::createConvertFuncToLLVMPass());
  pm.addPass(mlir::createReconcileUnrealizedCastsPass());

  // Parse static mlir function
  std::string moduleStr = R"mlir(
  func.func private @myFunction() -> ()
  func.func @fib(%n : index) -> i32 attributes { llvm.emit_c_interface } {
    func.call @myFunction() : () -> ()
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
      mlir::parseSourceString<mlir::ModuleOp>(moduleStr, &mlirContext);
  if (!module) {
    llvm::errs() << "module couldn't be created\n";
    exit(1);
  }

  // Run lowering pipeline. Pipeline transforms MLIR dialects -> MLIR dialect
  // for LLVM. The MLIR dialect for LLVM maps LLVM operations 1:1 with LLVM
  // operations.
  if (failed(pm.run(*module))) {
    llvm::errs() << "lowering pipeline failed\n";
    exit(1);
  }

  // translate MLIR llvm dialect into llvm
  auto llvmContext = std::make_unique<llvm::LLVMContext>();
  auto llvmModule = translateModuleToLLVMIR(*module, *llvmContext);

  // wrap LLVM IR in thing from ORC. I have no idea what this does.
  return llvm::orc::ThreadSafeModule(std::move(llvmModule),
                                     std::move(llvmContext));
}

int main(int argc, char **argv) {
  // LLVM init
  llvm::InitializeNativeTarget();
  llvm::InitializeNativeTargetAsmPrinter();
  llvm::InitializeNativeTargetAsmParser();

  // create LLJIT a pre-built jit compiler for LLVM IR
  auto jit = llvm::cantFail(llvm::orc::LLJITBuilder().create());

  // load symbols into jit that we would like to call from jit-ed code
  llvm::cantFail(jit->getMainJITDylib().define(llvm::orc::absoluteSymbols({
      {
          jit->mangleAndIntern("myFunction"),
          {
              llvm::orc::ExecutorAddr::fromPtr(myFunction),
              llvm::JITSymbolFlags::Exported,
          },
      },
  })));

  // load fib example into jit. This should result in a page with execute
  // permissions containing the assembly compiled from the LLVM IR we produced
  cantFail(jit->addIRModule(compileFibExample()));

  // grab address of fib function
  auto fibAddress = llvm::cantFail(jit->lookup("fib"));
  int (*fib)(uint64_t) = fibAddress.toPtr<int(uint64_t)>();

  // Process unput until user sends ctrl+d end-of-transmission character
  llvm::LineEditor LineEditor("");
  while (auto line = LineEditor.readLine()) {
    // maximum-hack-overdrive parse of user input
    uint64_t n;
    std::stringstream(*line) >> n;

    // And lo there was math
    std::cout << fib(n) << std::endl;
  }
  return 0;
}
