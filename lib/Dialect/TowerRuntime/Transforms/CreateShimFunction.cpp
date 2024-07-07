#include "mlir/Dialect/Func/IR/FuncOps.h"
#include "mlir/IR/BuiltinOps.h"
#include "mlir/IR/SymbolTable.h"

#include "Dialect/TowerRuntime/IR/TowerRuntimeOps.h"
#include "Dialect/TowerRuntime/Transforms/Passes.h"

#include "llvm/Support/Debug.h"
#include "llvm/Support/FormatVariadic.h"
#include <sstream>
#define DEBUG_TYPE "taco"

namespace mlir::towerRuntime {
#define GEN_PASS_DEF_SHIMFUNCTION
#include "Dialect/TowerRuntime/Transforms/Passes.h.inc"

namespace {};

class SwitchBarFoo : public impl::ShimFunctionBase<SwitchBarFoo> {
public:
  using impl::ShimFunctionBase<SwitchBarFoo>::ShimFunctionBase;
  void runOnOperation() final {
    if (functionToShim.empty()) {
      return;
    }
    auto module = getOperation();
    SymbolTable symbolTable(module);
    Operation *toShimOp = symbolTable.lookup(functionToShim);
    if (!toShimOp) {
      module->emitError(llvm::formatv(
          "module does not contain function named \"{0}\"", functionToShim));
      signalPassFailure();
      return;
    }
    if (!isa<func::FuncOp>(toShimOp)) {
      module->emitError(llvm::formatv(
          "module contains \"{0}\" but it is not a function", functionToShim));
      signalPassFailure();
      return;
    }
    func::FuncOp toShimFunc = cast<func::FuncOp>(toShimOp);

    // TODO: check operands, for now we're just assuming that it's right

    std::string shimFunctionName = llvm::formatv("{0}_shim", functionToShim);

    mlir::OpBuilder builder(&getContext());

    builder.setInsertionPointToStart(&module->getRegion(0).front());
    auto opFunctionTy = FunctionType::get(
        builder.getContext(), {builder.getF32Type()}, {builder.getF32Type()});
    auto opFunc = builder.create<func::FuncOp>(builder.getUnknownLoc(),
                                               shimFunctionName, opFunctionTy);
    opFunc.setPrivate();

    // LLVM_DEBUG(llvm::dbgs()
    //            << "TACO functionToShim: " << functionToShim << "\n");
  }
};
} // namespace mlir::towerRuntime
