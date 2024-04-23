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

#include "Wasm/WasmDialect.h"
#include "Wasm/WasmPasses.h"

#include <cstdint>
#include <iostream>
#include <sstream>
#include <string>

#include "mlir/Support/FileUtilities.h"
#include "llvm/Support/MemoryBuffer.h"
#include "llvm/Support/raw_ostream.h"

llvm::orc::ThreadSafeModule getLLVMIRFromStdin() {
  // Register MLIR dialects
  mlir::DialectRegistry registry;
  registry.insert<mlir::wasm::WasmDialect>();
  mlir::registerAllDialects(registry);
  mlir::registerBuiltinDialectTranslation(registry);
  mlir::registerLLVMDialectTranslation(registry);

  mlir::MLIRContext mlirContext(registry);

  // Create lowering pipeline
  mlir::PassManager pm(&mlirContext);
  pm.addPass(mlir::wasm::createLowerWasm());
  pm.addPass(mlir::createConvertVectorToSCFPass());
  pm.addPass(mlir::createConvertSCFToCFPass());
  pm.addPass(mlir::createConvertControlFlowToLLVMPass());
  pm.addPass(mlir::createFinalizeMemRefToLLVMConversionPass());
  pm.addNestedPass<mlir::func::FuncOp>(mlir::createArithToLLVMConversionPass());
  pm.addPass(mlir::createConvertFuncToLLVMPass());
  pm.addPass(mlir::createReconcileUnrealizedCastsPass());

  // read + parse module from stdin
  std::stringstream ss;
  std::string line;
  while (std::getline(std::cin, line)) {
    ss << line << std::endl;
  }

  mlir::OwningOpRef<mlir::ModuleOp> module =
      mlir::parseSourceString<mlir::ModuleOp>(ss.str(), &mlirContext);
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

int a_func() {
  std::cout << "func_a called" << std::endl;
  return 69;
}

int b_func() {
  std::cout << "func_b called" << std::endl;
  return 420;
}

int main(int argc, char *argv[]) {
  // LLVM init
  llvm::InitializeNativeTarget();
  llvm::InitializeNativeTargetAsmPrinter();
  llvm::InitializeNativeTargetAsmParser();

  // create LLJIT a pre-built jit compiler for LLVM IR
  auto jit = llvm::cantFail(llvm::orc::LLJITBuilder().create());

  // load symbols into jit that we would like to call from jit-ed code
  llvm::cantFail(jit->getMainJITDylib().define(llvm::orc::absoluteSymbols({
      {
          jit->mangleAndIntern("a_func"),
          {
              llvm::orc::ExecutorAddr::fromPtr(a_func),
              llvm::JITSymbolFlags::Exported,
          },
      },
      {
          jit->mangleAndIntern("b_func"),
          {
              llvm::orc::ExecutorAddr::fromPtr(b_func),
              llvm::JITSymbolFlags::Exported,
          },
      },
  })));

  // load fib example into jit. This should result in a page with execute
  // permissions containing the assembly compiled from the LLVM IR we produced
  llvm::cantFail(jit->addIRModule(getLLVMIRFromStdin()));

  // grab address of add function
  auto condAddress = llvm::cantFail(jit->lookup("fib"));
  int (*cond)(int) = condAddress.toPtr<int(int)>();

  for (int i = 0; i < 10; i++) {
    std::cout << cond(i) << std::endl;
  }

  return 0;
}
