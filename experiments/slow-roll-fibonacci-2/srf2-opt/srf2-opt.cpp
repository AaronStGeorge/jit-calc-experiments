#include "Conversion/TowerRuntimeToLLVM/TowerRuntimeToLLVM.h"
#include "mlir/Dialect/Arith/IR/Arith.h"
#include "mlir/IR/Dialect.h"
#include "mlir/IR/MLIRContext.h"
#include "mlir/InitAllDialects.h"
#include "mlir/InitAllPasses.h"
#include "mlir/Pass/Pass.h"
#include "mlir/Pass/PassManager.h"
#include "mlir/Support/FileUtilities.h"
#include "mlir/Tools/mlir-opt/MlirOptMain.h"
#include "llvm/Support/CommandLine.h"
#include "llvm/Support/InitLLVM.h"
#include "llvm/Support/SourceMgr.h"
#include "llvm/Support/ToolOutputFile.h"

#include "Dialect/Wasm/IR/WasmDialect.h"
#include "Dialect/TowerRuntime/IR/TowerRuntimeDialect.h"
#include "Dialect/Wasm/Transforms/Passes.h"
#include "Conversion/Passes.h"
#include "mlir/InitAllExtensions.h"

int main(int argc, char **argv) {
  mlir::registerAllPasses();
  mlir::jitCalcExperiments::registerConversionPasses();
  mlir::wasm::registerLowerWasm();

  mlir::DialectRegistry registry;
  registry.insert<mlir::wasm::WasmDialect>();
  registry.insert<mlir::towerRuntime::TowerRuntimeDialect>();
  registerAllDialects(registry);
  registerAllExtensions(registry);

  return mlir::asMainReturnCode(
      mlir::MlirOptMain(argc, argv, "SPF optimizer driver\n", registry));
}
