#include "mlir/Conversion/ConvertToLLVM/ToLLVMInterface.h"
#include "mlir/Conversion/LLVMCommon/ConversionTarget.h"
#include "mlir/Conversion/LLVMCommon/MemRefBuilder.h"
#include "mlir/Conversion/LLVMCommon/TypeConverter.h"
#include "mlir/Conversion/LLVMCommon/VectorPattern.h"
#include "mlir/Dialect/Arith/IR/Arith.h"
#include "mlir/Dialect/Func/IR/FuncOps.h"
#include "mlir/Dialect/LLVMIR/LLVMAttrs.h"
#include "mlir/Dialect/LLVMIR/LLVMDialect.h"
#include "mlir/Dialect/LLVMIR/LLVMTypes.h"
#include "mlir/IR/MLIRContext.h"
#include "mlir/IR/SymbolTable.h"
#include "mlir/IR/TypeUtilities.h"
#include "mlir/IR/ValueRange.h"
#include "mlir/Pass/Pass.h"
#include "mlir/Transforms/DialectConversion.h"

#include "llvm/ADT/StringRef.h"
#include "llvm/Support/Debug.h"
#include "llvm/Support/ErrorHandling.h"
#include "llvm/Support/FormatVariadic.h"

#include "Conversion/TowerRuntimeToLLVM/TowerRuntimeToLLVM.h"
#include "Dialect/TowerRuntime/IR/TowerRuntimeDialect.h"
#include "Dialect/TowerRuntime/IR/TowerRuntimeOps.h"
#include "Dialect/TowerRuntime/IR/TowerRuntimeTypes.h"

#define DEBUG_TYPE "taco"

namespace mlir {
#define GEN_PASS_DEF_TOWERRUNTIMETOLLVMCONVERSIONPASS
#include "Conversion/Passes.h.inc"
} // namespace mlir

using namespace mlir;

namespace {

static constexpr unsigned kCountPosInContextStruct = 2;
class ContextStructBuilder : public StructBuilder {
public:
  /// Construct a helper for the given complex number value.
  using StructBuilder::StructBuilder;
  static ContextStructBuilder load(OpBuilder &builder, Location loc,
                                   Value ptr) {
    auto i64Ty = builder.getI64Type();
    auto contextTy = LLVM::LLVMStructType::getLiteral(
        builder.getContext(), {i64Ty, i64Ty, i64Ty, i64Ty});
    auto load = builder.create<LLVM::LoadOp>(loc, contextTy, ptr);
    return ContextStructBuilder(load.getResult());
  }

  Value count(OpBuilder &builder, Location loc) {
    return extractPtr(builder, loc, kCountPosInContextStruct);
  }
  void setCount(OpBuilder &builder, Location loc, Value real) {
    setPtr(builder, loc, kCountPosInContextStruct, real);
  }
};

struct LowerIncrement
    : public OpConversionPattern<towerRuntime::IncrementCountOp> {
  using OpConversionPattern::OpConversionPattern;

  LogicalResult
  matchAndRewrite(towerRuntime::IncrementCountOp op, OpAdaptor adaptor,
                  ConversionPatternRewriter &rewriter) const override {
    // Increment the count
    auto i64Ty = rewriter.getI64Type();
    ContextStructBuilder contextStructBuilder =
        ContextStructBuilder::load(rewriter, op->getLoc(), adaptor.getInput());
    auto count = contextStructBuilder.count(rewriter, op->getLoc());
    auto constOp = rewriter.create<arith::ConstantOp>(
        op->getLoc(), i64Ty, rewriter.getI64IntegerAttr(1));
    auto increment =
        rewriter.create<arith::AddIOp>(op->getLoc(), constOp, count);
    contextStructBuilder.setCount(rewriter, op->getLoc(), increment);

    // Store results back
    rewriter.create<LLVM::StoreOp>(op->getLoc(), contextStructBuilder,
                                   adaptor.getInput());

    rewriter.eraseOp(op);
    return success();
  }
};

// Map op to runtime call, the type converter is expected to have done all the
// work needed to convert operands.
template <typename OpTy>
struct ToRuntimeCall : public OpConversionPattern<OpTy> {
  explicit ToRuntimeCall(const TypeConverter &typeConverter,
                         MLIRContext *context, SymbolTable symbolTable,
                         StringRef calleeName, PatternBenefit benefit = 1)
      : OpConversionPattern<OpTy>(typeConverter, context, benefit),
        symbolTable(symbolTable), calleeName(calleeName) {}

  LogicalResult
  matchAndRewrite(OpTy op, typename OpTy::Adaptor adaptor,
                  ConversionPatternRewriter &rewriter) const override {
    auto callee = symbolTable.lookup(calleeName);
    if (!callee || !isa<func::FuncOp>(callee)) {
      llvm_unreachable(
          llvm::formatv("couldn't find runtime function named {0}", calleeName)
              .str()
              .c_str());
    }
    rewriter.replaceOpWithNewOp<func::CallOp>(op, cast<func::FuncOp>(callee),
                                              adaptor.getOperands());
    return success();
  }

private:
  SymbolTable symbolTable;
  StringRef calleeName;
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

    SymbolTable symbolTable = SymbolTable(module);

    RewritePatternSet patterns(context);
    patterns.add<ToRuntimeCall<towerRuntime::DumpContextOp>>(
        typeConverter, context, symbolTable, "dump");
    patterns.add<ToRuntimeCall<towerRuntime::CheckSwapOp>>(
        typeConverter, context, symbolTable, "check_swap");
    patterns.add<LowerIncrement>(typeConverter, context);

    ConversionTarget target(*context);
    target.addDynamicallyLegalOp<func::FuncOp>([&](func::FuncOp op) {
      return typeConverter.isSignatureLegal(op.getFunctionType()) &&
             typeConverter.isLegal(&op.getBody());
    });
    target.addLegalOp<func::CallOp>();
    target.addLegalDialect<LLVM::LLVMDialect>();
    target.addLegalDialect<arith::ArithDialect>();

    populateFunctionOpInterfaceTypeConversionPattern<func::FuncOp>(
        patterns, typeConverter);

    if (failed(applyPartialConversion(module, target, std::move(patterns))))
      signalPassFailure();
  }
};
} // namespace
