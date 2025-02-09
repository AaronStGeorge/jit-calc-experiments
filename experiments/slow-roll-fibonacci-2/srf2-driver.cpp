#include "mlir/Conversion/ArithToLLVM/ArithToLLVM.h"
#include "mlir/Conversion/ControlFlowToLLVM/ControlFlowToLLVM.h"
#include "mlir/Conversion/FuncToLLVM/ConvertFuncToLLVMPass.h"
#include "mlir/Conversion/MemRefToLLVM/MemRefToLLVM.h"
#include "mlir/Conversion/ReconcileUnrealizedCasts/ReconcileUnrealizedCasts.h"
#include "mlir/Conversion/SCFToControlFlow/SCFToControlFlow.h"
#include "mlir/Conversion/VectorToSCF/VectorToSCF.h"
#include "mlir/Dialect/Func/IR/FuncOps.h"
#include "mlir/Dialect/Linalg/Passes.h"
#include "mlir/ExecutionEngine/MemRefUtils.h"
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
#include "llvm/Support/Error.h"
#include "llvm/Support/ErrorHandling.h"
#include "llvm/Support/TargetSelect.h"
#include "llvm/Support/raw_ostream.h"

#include "TowerRuntime/TowerRuntime.h"
#include "jit-calc-mlir/Dialect/Wasm/IR/WasmDialect.h"
#include "jit-calc-mlir/Dialect/Wasm/Transforms/Passes.h"

#include <cstdint>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <string>

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

int fib(int n) {
  int i = 0;
  int j = 1;

  for (int c = 0; c < n; c++) {
    int tmp = i;
    i = j;
    j = i + tmp;
  }

  return i;
}

void dump(TowerRuntimeContext *s) {
  std::cout << "{main=" << s->main << ", thread=" << s->thread
            << ", count=" << s->count << ", limit=" << s->limit << "}"
            << std::endl;
}

int main(int argc, char *argv[]) {
  // LLVM init
  llvm::InitializeNativeTarget();
  llvm::InitializeNativeTargetAsmPrinter();
  llvm::InitializeNativeTargetAsmParser();

  // create LLJIT a pre-built jit compiler for LLVM IR
  auto jit = llvm::cantFail(llvm::orc::LLJITBuilder().create());

  // Load each symbol from the runtime we want to be available to running
  // program into jit.
#define RUNTIME_FUNCTION(f)                                                    \
  llvm::detail::DenseMapPair<llvm::orc::SymbolStringPtr,                       \
                             llvm::orc::ExecutorSymbolDef> {                   \
    jit->mangleAndIntern(#f),                                                  \
        {                                                                      \
            llvm::orc::ExecutorAddr::fromPtr(f),                               \
            llvm::JITSymbolFlags::Exported,                                    \
        },                                                                     \
  }
  llvm::cantFail(jit->getMainJITDylib().define(llvm::orc::absoluteSymbols({
      RUNTIME_FUNCTION(dump),
  })));
#undef RUNTIME_FUNCTION

  // Load whatever we get from stdin into jit. This should result in a page with
  // execute permissions containing the assembly compiled from the LLVM IR we
  // produced.
  llvm::cantFail(jit->addIRModule(getLLVMIRFromStdin()));

  // grab address of fib function.
  auto entryAddr = llvm::cantFail(jit->lookup("fib_shim"));
  void (*entry)(TowerRuntimeContext *, void *, void *) =
      entryAddr.toPtr<void(TowerRuntimeContext *, void *, void *)>();

  // arguments
  int in = -1;
  int slowRollCompleted = -1;
  int slowRollComputing = -1;

  bool doesSlowRollNeedSetup = true;

  // setup stack
  size_t stackSize = 8 * 1024;
  uint8_t *stack = (uint8_t *)calloc(stackSize, sizeof(char));

  CPPTowerRuntimeContext trctx = CPPTowerRuntimeContext(stack, stackSize, 256);

  std::cout << "Fibonacci game 2!" << std::endl;
  std::cout << "game";
  std::cout << std::setw(6) << "user";
  std::cout << std::endl;

  for (int i = 0; i < 20; i++) {
    if (doesSlowRollNeedSetup) {
      doesSlowRollNeedSetup = false;

      trctx = CPPTowerRuntimeContext(stack, stackSize, 256);

      in = i;

      // Start uses pointers to make the ABI simpler, the function is
      // implemented in assembly so such things matter.
      start(&trctx.ctx, entry, &in, &slowRollComputing);
    }

    if (swap(/*from*/ trctx.ctx.main, /*to*/ trctx.ctx.thread)) {
      slowRollCompleted = slowRollComputing;
      doesSlowRollNeedSetup = true;
    }

    int slowRoll = slowRollCompleted;
    int fastRoll = fib(i);

    std::cout << std::setw(4) << fastRoll;
    std::cout << std::setw(6) << slowRoll;
    std::cout << std::endl;
  }
  free(stack);

  return 0;
}
