#include "mlir/Conversion/LLVMCommon/StructBuilder.h"
#include "mlir/Dialect/Func/IR/FuncOps.h"
#include "mlir/Dialect/LLVMIR/LLVMDialect.h"
#include "mlir/Dialect/LLVMIR/LLVMTypes.h"
#include "mlir/IR/Builders.h"
#include "mlir/IR/BuiltinOps.h"
#include "mlir/IR/ImplicitLocOpBuilder.h"
#include "mlir/IR/Location.h"
#include "mlir/IR/SymbolTable.h"
#include "mlir/Interfaces/FunctionInterfaces.h"
#include "mlir/Support/LogicalResult.h"

#include "jit-calc-mlir/Dialect/TowerRuntime/IR/TowerRuntimeOps.h"
#include "jit-calc-mlir/Dialect/TowerRuntime/Transforms/Passes.h"

#include "llvm/ADT/STLExtras.h"
#include "llvm/ADT/SmallVector.h"
#include "llvm/Support/Debug.h"
#include "llvm/Support/FormatVariadic.h"

#include <string>

#define DEBUG_TYPE "taco"
namespace mlir::towerRuntime {
#define GEN_PASS_DEF_SHIMFUNCTION
#include "jit-calc-mlir/Dialect/TowerRuntime/Transforms/Passes.h.inc"

// Game code calls into generated code using a very general, type erased,
// C++ interface that looks like:
//   void start(TowerRuntimeContext* ctx, void* in, void* out);
// Our job here is to create a shim function between the general type erased
// interface to a generated interface that accepts arguments with a known type.
//
// For example, if the generated entry point has an interface---example is in
// C++ but, but generated code will be in MLIR---that looks like:
//   int add(TowerRuntimeContext* ctx, int, int)
// The C++ game code calling into generated code will be expected to look like
// this:
//   struct In {
//     int a;
//     int b;
//   };
//   struct Out {
//     int result;
//   };
//   ...
//   In in = {.a = 400, .b = 20};
//   Out out = {.result = -1};
//   start(&ctx, &in, &out);
// The shim function we're generating---again written in C++, but it would
// actually be in MLIR---would will look like:
//   void add_shim(TowerRuntimeContext *ctx, struct In *in, struct Out *out)
//   {
//     int a = in->a;
//     int b = in->b;
//     int result = add(ctx, a, b);
//     out->result = result;
//   }
// It's all a little gnarly, but game code needs to call into co-routine
// functions that are hand written in assembly... So doing something else would
// likely be even more gnarly... Maybe someday it would be worth generating code
// for all types that could pass through so we don't have to do all this
// shim-function-packing-unpacking nonsense.
//
// TODO: there should be some sort of type checking on the function to shim
// I'm not sure what the best place to do that is, but it should happen
// somewhere.
class CreateShimFunction : public impl::ShimFunctionBase<CreateShimFunction> {
public:
  using impl::ShimFunctionBase<CreateShimFunction>::ShimFunctionBase;

  void runOnOperation() final {
    if (functionToShim.empty()) {
      return;
    }

    OpBuilder builder(&getContext());
    Location loc = builder.getUnknownLoc();

    // 1. grab function to shim
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
    auto funcToShim = cast<func::FuncOp>(toShimOp);

    // 1.a. apply prefix if requested
    if (!prefix.empty()) {
      funcToShim.setName(
          llvm::formatv("{0}_{1}", prefix, functionToShim).str());
    }

    // 2. create shim function prototype
    builder.setInsertionPointToStart(&module.getBodyRegion().front());

    auto ptrTy = LLVM::LLVMPointerType::get(builder.getContext());
    auto shimFuncTy =
        FunctionType::get(builder.getContext(), {ptrTy, ptrTy, ptrTy}, {});
    std::string shimFunctionName =
        llvm::formatv("{0}_shim", funcToShim.getSymName());
    auto shimFunc =
        builder.create<func::FuncOp>(loc, shimFunctionName, shimFuncTy);

    // 3. create shim function body
    builder.setInsertionPointToStart(shimFunc.addEntryBlock());
    auto ctxLoad =
        builder.create<towerRuntime::LoadOp>(loc, shimFunc.getArgument(0));

    // 3.a. Read arguments from type erased pointer to inputs struct
    FunctionType toShimFuncTy = funcToShim.getFunctionType();
    llvm::SmallVector<Value> argsForFuncToShim(toShimFuncTy.getNumInputs());
    {
      // First argument will be TowerRuntimeCtx pointer, we can just pass that
      // through no munging required.
      argsForFuncToShim[0] = Value(ctxLoad);

      // Other arguments will be read by loading out of type erased inputs
      // pointer.
      ArrayRef<Type> nonCtxInputTys = toShimFuncTy.getInputs().slice(1);
      LLVM::LLVMStructType inStructTy = LLVM::LLVMStructType::getLiteral(
          builder.getContext(), nonCtxInputTys);

      for (auto [i, inputTy] : llvm::enumerate(nonCtxInputTys)) {
        auto gepPtr = builder.create<LLVM::GEPOp>(loc, ptrTy, inStructTy,
                                                  shimFunc.getArgument(1),
                                                  ArrayRef<LLVM::GEPArg>{0, i});
        argsForFuncToShim[i + 1] =
            builder.create<LLVM::LoadOp>(loc, inputTy, gepPtr);
      }
    }

    // 3.b. Create the call into generated function we're creating a shim for.
    auto call = builder.create<func::CallOp>(builder.getUnknownLoc(),
                                             funcToShim, argsForFuncToShim);

    // 3.c. Write results to type erased pointer to outputs struct
    LLVM::LLVMStructType outStructTy = LLVM::LLVMStructType::getLiteral(
        builder.getContext(), toShimFuncTy.getResults());
    for (auto [i, result] : llvm::enumerate(call->getResults())) {
      auto gepPtr = builder.create<LLVM::GEPOp>(loc, ptrTy, outStructTy,
                                                shimFunc.getArgument(2),
                                                ArrayRef<LLVM::GEPArg>{0, i});
      builder.create<LLVM::StoreOp>(loc, result, gepPtr);
    }

    builder.create<func::ReturnOp>(builder.getUnknownLoc());
  };
};
} // namespace mlir::towerRuntime
