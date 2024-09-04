#include "mlir/Dialect/Func/IR/FuncOps.h"
#include "mlir/IR/BuiltinOps.h"
#include "mlir/IR/SymbolTable.h"

#include "jit-calc-mlir/Dialect/TowerRuntime/IR/TowerRuntimeOps.h"
#include "jit-calc-mlir/Dialect/TowerRuntime/Transforms/Passes.h"
#include "mlir/Support/LogicalResult.h"

#include "llvm/Support/Debug.h"
#include "llvm/Support/FormatVariadic.h"

#include <string>

#define DEBUG_TYPE "taco"
namespace mlir::towerRuntime {
#define GEN_PASS_DEF_SHIMFUNCTION
#include "jit-calc-mlir/Dialect/TowerRuntime/Transforms/Passes.h.inc"

class CreateShimFunction : public impl::ShimFunctionBase<CreateShimFunction> {
public:
  using impl::ShimFunctionBase<CreateShimFunction>::ShimFunctionBase;

  void runOnOperation() final {
    if (functionToShim.empty()) {
      return;
    }

    // grab function to shim
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
    auto toShimFunc = cast<func::FuncOp>(toShimOp);
    // TODO: check operands. For now, we're just assuming that it's right.

    // create shim function prototype
    mlir::OpBuilder builder(&getContext());
    builder.setInsertionPointToStart(&module->getRegion(0).front());
    auto opFunctionTy = FunctionType::get(
        builder.getContext(), {LLVM::LLVMPointerType::get(&getContext())}, {});
    std::string shimFunctionName = llvm::formatv("{0}_shim", functionToShim);
    auto opFunc = builder.create<func::FuncOp>(builder.getUnknownLoc(),
                                               shimFunctionName, opFunctionTy);

    // create shim function body
    builder.setInsertionPointToStart(opFunc.addEntryBlock());
    auto load = builder.create<towerRuntime::LoadOp>(builder.getUnknownLoc(),
                                                     opFunc.getArgument(0));
    builder.create<func::CallOp>(builder.getUnknownLoc(), toShimFunc,
                                 Value(load));
    builder.create<func::ReturnOp>(builder.getUnknownLoc());
  }
};
} // namespace mlir::towerRuntime
