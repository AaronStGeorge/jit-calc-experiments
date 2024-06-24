#include "Dialect/TowerRuntime/IR/TowerRuntimeDialect.h"
#include "Dialect/TowerRuntime/IR/TowerRuntimeOps.h"
#include "Dialect/TowerRuntime/IR/TowerRuntimeTypes.h"
#include "mlir/Conversion/ConvertToLLVM/ToLLVMInterface.h"
#include "mlir/Conversion/LLVMCommon/ConversionTarget.h"
#include "mlir/Conversion/LLVMCommon/MemRefBuilder.h"
#include "mlir/Conversion/LLVMCommon/TypeConverter.h"
#include "mlir/Conversion/LLVMCommon/VectorPattern.h"
#include "mlir/Dialect/Func/IR/FuncOps.h"
#include "mlir/Dialect/LLVMIR/LLVMAttrs.h"
#include "mlir/Dialect/LLVMIR/LLVMDialect.h"
#include "mlir/Dialect/LLVMIR/LLVMTypes.h"
#include "mlir/IR/MLIRContext.h"
#include "mlir/IR/TypeUtilities.h"
#include "mlir/Pass/Pass.h"

#include "mlir/Transforms/DialectConversion.h"

#include "Conversion/TowerRuntimeToLLVM/TowerRuntimeToLLVM.h"

namespace mlir {
#define GEN_PASS_DEF_TOWERRUNTIMETOLLVMCONVERSIONPASS
#include "Conversion/Passes.h.inc"
} // namespace mlir

using namespace mlir;

//===----------------------------------------------------------------------===//
// Pass Definition
//===----------------------------------------------------------------------===//

namespace {

struct FooOpLowering : public OpConversionPattern<towerRuntime::FooOp> {
  using OpConversionPattern::OpConversionPattern;

  LogicalResult
  matchAndRewrite(towerRuntime::FooOp op, OpAdaptor adaptor,
                  ConversionPatternRewriter &rewriter) const override {
    rewriter.eraseOp(op);
    return success();
  }
};

struct TowerRuntimeToLLVMConversionPass
    : public impl::TowerRuntimeToLLVMConversionPassBase<
          TowerRuntimeToLLVMConversionPass> {
  using Base::Base;

  void runOnOperation() override {
    auto module = getOperation();
    auto context = module.getContext();

    TypeConverter typeConverter;
    typeConverter.addConversion([](Type type) { return type; });
    typeConverter.addConversion([context](towerRuntime::ContextType type) {
      return LLVM::LLVMPointerType::get(context);
    });

    RewritePatternSet patterns(context);
    patterns.add<FooOpLowering>(typeConverter, context);

    ConversionTarget target(*context);
    target.addDynamicallyLegalOp<func::FuncOp>([&](func::FuncOp op) {
      return typeConverter.isSignatureLegal(op.getFunctionType()) &&
             typeConverter.isLegal(&op.getBody());
    });

    populateFunctionOpInterfaceTypeConversionPattern<func::FuncOp>(
        patterns, typeConverter);

    if (failed(applyPartialConversion(module, target, std::move(patterns))))
      signalPassFailure();
  }
};
} // namespace
