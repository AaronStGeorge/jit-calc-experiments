// Lower TowerRuntime operations and context types to LLVM-compatible MLIR.

#include "mlir/Conversion/LLVMCommon/StructBuilder.h"
#include "mlir/Conversion/LLVMCommon/TypeConverter.h"
#include "mlir/Dialect/Arith/IR/Arith.h"
#include "mlir/Dialect/ControlFlow/IR/ControlFlowOps.h"
#include "mlir/Dialect/Func/IR/FuncOps.h"
#include "mlir/Dialect/Func/Transforms/FuncConversions.h"
#include "mlir/Dialect/LLVMIR/LLVMDialect.h"
#include "mlir/Dialect/LLVMIR/LLVMTypes.h"
#include "mlir/Dialect/MemRef/IR/MemRef.h"
#include "mlir/Dialect/SCF/IR/SCF.h"
#include "mlir/IR/Builders.h"
#include "mlir/IR/BuiltinAttributes.h"
#include "mlir/IR/BuiltinOps.h"
#include "mlir/IR/MLIRContext.h"
#include "mlir/IR/PatternMatch.h"
#include "mlir/IR/SymbolTable.h"
#include "mlir/IR/TypeRange.h"
#include "mlir/IR/Value.h"
#include "mlir/IR/ValueRange.h"
#include "mlir/Pass/Pass.h"
#include "mlir/Transforms/DialectConversion.h"

#include "llvm/ADT/SmallVector.h"
#include "llvm/ADT/StringRef.h"
#include "llvm/Support/ErrorHandling.h"
#include "llvm/Support/FormatVariadic.h"

#include "jit-calc-mlir/Conversion/TowerRuntimeToLLVM/TowerRuntimeToLLVM.h"
#include "jit-calc-mlir/Dialect/TowerRuntime/IR/TowerRuntimeDialect.h"
#include "jit-calc-mlir/Dialect/TowerRuntime/IR/TowerRuntimeOps.h"
#include "jit-calc-mlir/Dialect/TowerRuntime/IR/TowerRuntimeTypes.h"

#include <vector>

namespace mlir {
#define GEN_PASS_DEF_TOWERRUNTIMETOLLVMCONVERSIONPASS
#include "jit-calc-mlir/Conversion/Passes.h.inc"
} // namespace mlir

using namespace mlir;

namespace {

LLVM::LLVMStructType getTowerRuntimeContextType(MLIRContext *context) {
  auto i64Ty = IntegerType::get(context, 64);
  auto ptrTy = LLVM::LLVMPointerType::get(context);
  return LLVM::LLVMStructType::getLiteral(context,
                                          {ptrTy, ptrTy, i64Ty, i64Ty});
}

// Field indices match getTowerRuntimeContextType and the C++ runtime layout.
class ContextStructBuilder : public StructBuilder {
  enum Field : unsigned { Main, Thread, Count, Limit };

public:
  using StructBuilder::StructBuilder;

  Value mainCtx(OpBuilder &builder, Location loc) {
    return extractPtr(builder, loc, Main);
  }
  Value threadCtx(OpBuilder &builder, Location loc) {
    return extractPtr(builder, loc, Thread);
  }
  Value count(OpBuilder &builder, Location loc) {
    return extractPtr(builder, loc, Count);
  }
  void setCount(OpBuilder &builder, Location loc, Value count) {
    setPtr(builder, loc, Count, count);
  }
  Value limit(OpBuilder &builder, Location loc) {
    return extractPtr(builder, loc, Limit);
  }
};

func::FuncOp lookupRuntimeFunction(const SymbolTable &symbolTable,
                                   StringRef calleeName) {
  auto callee = symbolTable.lookup<func::FuncOp>(calleeName);
  if (!callee)
    llvm::report_fatal_error(
        llvm::formatv("couldn't find runtime function named {0}", calleeName));
  return callee;
}

struct LowerIncrement
    : public OpConversionPattern<towerRuntime::IncrementCountOp> {
  using OpConversionPattern::OpConversionPattern;

  LogicalResult
  matchAndRewrite(towerRuntime::IncrementCountOp op, OpAdaptor adaptor,
                  ConversionPatternRewriter &rewriter) const override {
    Location loc = op.getLoc();
    ContextStructBuilder ctx(adaptor.getInput());
    Value count = ctx.count(rewriter, loc);
    Value one = arith::ConstantIntOp::create(rewriter, loc, 1, 64);
    Value increment = arith::AddIOp::create(rewriter, loc, one, count);
    ctx.setCount(rewriter, loc, increment);

    rewriter.replaceOp(op, Value(ctx));
    return success();
  }
};

struct LowerCheckSwap : public OpConversionPattern<towerRuntime::CheckSwapOp> {
  explicit LowerCheckSwap(const TypeConverter &typeConverter,
                          MLIRContext *context, SymbolTable symbolTable,
                          PatternBenefit benefit = 1)
      : OpConversionPattern<towerRuntime::CheckSwapOp>(typeConverter, context,
                                                       benefit),
        symbolTable(symbolTable) {}

  LogicalResult
  matchAndRewrite(towerRuntime::CheckSwapOp op, OpAdaptor adaptor,
                  ConversionPatternRewriter &rewriter) const override {
    Location loc = op.getLoc();
    Value input = adaptor.getInput();
    ContextStructBuilder ctx(input);
    Value count = ctx.count(rewriter, loc);
    Value limit = ctx.limit(rewriter, loc);
    Value overBudget = arith::CmpIOp::create(
        rewriter, loc, arith::CmpIPredicate::uge, count, limit);
    func::FuncOp swap = lookupRuntimeFunction(symbolTable, "swap");

    rewriter.replaceOpWithNewOp<scf::IfOp>(
        op, overBudget,
        /*thenBuilder=*/[input, swap](OpBuilder &b, Location loc) {
          ContextStructBuilder ctx(input);
          // Suspend the player and resume the host.
          Value thread = ctx.threadCtx(b, loc);
          Value main = ctx.mainCtx(b, loc);
          func::CallOp::create(b, loc, swap, ValueRange({thread, main}));

          // When the host resumes the player, reset the instruction count.
          Value zero = arith::ConstantIntOp::create(b, loc, 0, 64);
          ctx.setCount(b, loc, zero);
          scf::YieldOp::create(b, loc, Value(ctx));
        },
        /*elseBuilder=*/[input](OpBuilder &b, Location loc) {
          scf::YieldOp::create(b, loc, input);
        });
    return success();
  }

private:
  SymbolTable symbolTable;
};

struct LowerLoad : public OpConversionPattern<towerRuntime::LoadOp> {
  using OpConversionPattern::OpConversionPattern;

  LogicalResult
  matchAndRewrite(towerRuntime::LoadOp op, OpAdaptor adaptor,
                  ConversionPatternRewriter &rewriter) const override {
    rewriter.replaceOpWithNewOp<LLVM::LoadOp>(
        op, getTowerRuntimeContextType(op->getContext()), adaptor.getInput());
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
    func::FuncOp callee = lookupRuntimeFunction(symbolTable, calleeName);

    // The runtime takes a pointer, so materialize the context on the stack.
    Location loc = op.getLoc();
    MLIRContext *context = rewriter.getContext();
    LLVMTypeConverter llvmTypeConverter(context);
    auto ptrTy = LLVM::LLVMPointerType::get(context);
    Type indexTy = llvmTypeConverter.convertType(rewriter.getIndexType());
    Value one = LLVM::ConstantOp::create(rewriter, loc, indexTy, 1);
    Value storage = LLVM::AllocaOp::create(
        rewriter, loc, ptrTy, getTowerRuntimeContextType(context), one,
        /*alignment=*/0);
    LLVM::StoreOp::create(rewriter, loc, adaptor.getInput(), storage);
    rewriter.replaceOpWithNewOp<func::CallOp>(op, callee, storage);
    return success();
  }

private:
  SymbolTable symbolTable;
  StringRef calleeName;
};

struct RuntimeFunction {
  StringRef name;
  TypeRange inputs;
  TypeRange results;
};

// Add prototypes for runtime functions defined in C++ (or whatever).
//
// Example:
//   func.func private @swap(!llvm.ptr, !llvm.ptr) -> (i1)
void addRuntimeFunctionPrototypes(
    MLIRContext *context, ModuleOp module,
    std::vector<RuntimeFunction> runtimeFunctions) {
  mlir::OpBuilder builder(context);
  Location loc = builder.getUnknownLoc();
  builder.setInsertionPointToStart(&module->getRegion(0).front());

  for (auto &runtimeFunction : runtimeFunctions) {
    auto functionType = FunctionType::get(context, runtimeFunction.inputs,
                                          runtimeFunction.results);
    auto functionOp =
        func::FuncOp::create(builder, loc, runtimeFunction.name, functionType);
    functionOp.setPrivate();
  }
}

// A branch can pass a !tower_runtime.ctx to a successor block, for example
// to carry the current context into the next loop iteration. Convert the
// arguments of every block so branches pass the lowered LLVM struct to
// destinations that expect it, and update the function signature to match
// the converted entry block and result types.
struct ConvertFunction : OpConversionPattern<func::FuncOp> {
  using OpConversionPattern::OpConversionPattern;

  LogicalResult
  matchAndRewrite(func::FuncOp op, OpAdaptor,
                  ConversionPatternRewriter &rewriter) const override {
    const TypeConverter &converter = *getTypeConverter();
    TypeConverter::SignatureConversion signature(op.getNumArguments());
    SmallVector<Type> results;
    if (failed(converter.convertSignatureArgs(op.getArgumentTypes(),
                                              signature)) ||
        failed(converter.convertTypes(op.getResultTypes(), results)) ||
        failed(rewriter.convertRegionTypes(&op.getBody(), converter,
                                           &signature)))
      return failure();

    auto type = FunctionType::get(op.getContext(), signature.getConvertedTypes(),
                                  results);
    rewriter.modifyOpInPlace(op, [&] { op.setType(type); });
    return success();
  }
};
} // namespace

struct TowerRuntimeToLLVMConversionPass
    : public impl::TowerRuntimeToLLVMConversionPassBase<
          TowerRuntimeToLLVMConversionPass> {
  using Base::Base;

  void runOnOperation() override {
    ModuleOp module = getOperation();
    MLIRContext *context = module.getContext();

    auto ptrTy = LLVM::LLVMPointerType::get(context);
    auto i1Ty = IntegerType::get(context, 1);
    addRuntimeFunctionPrototypes(
        context, module,
        {
            {.name = "swap", .inputs = {ptrTy, ptrTy}, .results = {i1Ty}},
            {.name = "dump", .inputs = {ptrTy}, .results = {}},
        });

    TypeConverter typeConverter;
    typeConverter.addConversion([](Type type) { return type; });
    typeConverter.addConversion([context](towerRuntime::ContextType type) {
      return getTowerRuntimeContextType(context);
    });

    SymbolTable symbolTable = SymbolTable(module);

    RewritePatternSet patterns(context);
    patterns.add<ToRuntimeCall<towerRuntime::DumpContextOp>>(
        typeConverter, context, symbolTable, "dump");
    patterns.add<LowerCheckSwap>(typeConverter, context, symbolTable);
    patterns.add<LowerIncrement>(typeConverter, context);
    patterns.add<LowerLoad>(typeConverter, context);

    ConversionTarget target(*context);

    // Converting from
    target.addIllegalDialect<towerRuntime::TowerRuntimeDialect>();

    // Converting to
    target.addLegalDialect<LLVM::LLVMDialect>();
    target.addLegalDialect<arith::ArithDialect>();
    target.addLegalDialect<scf::SCFDialect>();
    target.addLegalDialect<memref::MemRefDialect>();
    target.addLegalOp<func::ReturnOp>();

    patterns.add<ConvertFunction>(typeConverter, context);
    target.addDynamicallyLegalOp<func::FuncOp>([&](func::FuncOp op) {
      return typeConverter.isSignatureLegal(op.getFunctionType()) &&
             typeConverter.isLegal(&op.getBody());
    });

    // Convert call sites.
    populateCallOpTypeConversionPattern(patterns, typeConverter);
    target.addDynamicallyLegalOp<func::CallOp>([&](func::CallOp op) {
      return typeConverter.isSignatureLegal(op.getCalleeType());
    });

    // Ensure block arguments + branch instructions are converted
    populateBranchOpInterfaceTypeConversionPattern(patterns, typeConverter);
    target.addDynamicallyLegalOp<cf::BranchOp>([&](cf::BranchOp op) {
      return typeConverter.isLegal(op->getOperandTypes());
    });
    target.addDynamicallyLegalOp<cf::CondBranchOp>([&](cf::CondBranchOp op) {
      return typeConverter.isLegal(op->getOperandTypes());
    });

    if (failed(applyPartialConversion(module, target, std::move(patterns))))
      signalPassFailure();
  }
};
