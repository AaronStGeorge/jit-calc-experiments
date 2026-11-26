#include "mlir/Conversion/ConvertToLLVM/ToLLVMInterface.h"
#include "mlir/Conversion/LLVMCommon/MemRefBuilder.h"
#include "mlir/Conversion/LLVMCommon/TypeConverter.h"
#include "mlir/Dialect/Arith/IR/Arith.h"
#include "mlir/Dialect/Bufferization/IR/BufferizableOpInterface.h"
#include "mlir/Dialect/ControlFlow/IR/ControlFlow.h"
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
#include "mlir/IR/TypeUtilities.h"
#include "mlir/IR/Value.h"
#include "mlir/IR/ValueRange.h"
#include "mlir/Pass/Pass.h"
#include "mlir/Transforms/DialectConversion.h"

#include "llvm/ADT/StringRef.h"
#include "llvm/Support/Casting.h"
#include "llvm/Support/Debug.h"
#include "llvm/Support/ErrorHandling.h"
#include "llvm/Support/FormatVariadic.h"

#include "jit-calc-mlir/Conversion/TowerRuntimeToLLVM/TowerRuntimeToLLVM.h"
#include "jit-calc-mlir/Dialect/TowerRuntime/IR/TowerRuntimeDialect.h"
#include "jit-calc-mlir/Dialect/TowerRuntime/IR/TowerRuntimeOps.h"
#include "jit-calc-mlir/Dialect/TowerRuntime/IR/TowerRuntimeTypes.h"

#define DEBUG_TYPE "taco"

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

static constexpr unsigned kMainCtxPosInContextStruct = 0;
static constexpr unsigned kThreadCtxPosInContextStruct = 1;
static constexpr unsigned kCountPosInContextStruct = 2;
static constexpr unsigned kLimitPosInContextStruct = 3;
class ContextStructBuilder : public StructBuilder {
public:
  /// Construct a helper for the given complex number value.
  using StructBuilder::StructBuilder;
  static ContextStructBuilder load(OpBuilder &builder, Location loc,
                                   Value ptr) {
    LLVM::LLVMStructType contextTy =
        getTowerRuntimeContextType(builder.getContext());
    auto load = builder.create<LLVM::LoadOp>(loc, contextTy, ptr);
    return ContextStructBuilder(load.getResult());
  }

  Value mainCtx(OpBuilder &builder, Location loc) {
    return extractPtr(builder, loc, kMainCtxPosInContextStruct);
  }
  Value threadCtx(OpBuilder &builder, Location loc) {
    return extractPtr(builder, loc, kThreadCtxPosInContextStruct);
  }
  Value count(OpBuilder &builder, Location loc) {
    return extractPtr(builder, loc, kCountPosInContextStruct);
  }
  void setCount(OpBuilder &builder, Location loc, Value real) {
    setPtr(builder, loc, kCountPosInContextStruct, real);
  }
  Value limit(OpBuilder &builder, Location loc) {
    return extractPtr(builder, loc, kLimitPosInContextStruct);
  }
  void setLimit(OpBuilder &builder, Location loc, Value real) {
    setPtr(builder, loc, kLimitPosInContextStruct, real);
  }
};

func::FuncOp lookupRuntimeFunction(SymbolTable symbolTable,
                                   StringRef calleeName) {
  auto callee = symbolTable.lookup(calleeName);
  if (!callee || !isa<func::FuncOp>(callee)) {
    llvm_unreachable(
        llvm::formatv("couldn't find runtime function named {0}", calleeName)
            .str()
            .c_str());
  }
  return cast<func::FuncOp>(callee);
}

struct LowerIncrement
    : public OpConversionPattern<towerRuntime::IncrementCountOp> {
  using OpConversionPattern::OpConversionPattern;

  LogicalResult
  matchAndRewrite(towerRuntime::IncrementCountOp op, OpAdaptor adaptor,
                  ConversionPatternRewriter &rewriter) const override {
    // Increment the count
    auto i64Ty = rewriter.getI64Type();
    ContextStructBuilder contextStructBuilder(adaptor.getInput());
    auto count = contextStructBuilder.count(rewriter, op->getLoc());
    auto constOp = rewriter.create<arith::ConstantOp>(
        op->getLoc(), i64Ty, rewriter.getI64IntegerAttr(1));
    auto increment =
        rewriter.create<arith::AddIOp>(op->getLoc(), constOp, count);
    contextStructBuilder.setCount(rewriter, op->getLoc(), increment);

    rewriter.replaceOp(op, mlir::Value(contextStructBuilder));
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
    ContextStructBuilder contextStructBuilder(adaptor.getInput());
    Value count = contextStructBuilder.count(rewriter, op->getLoc());
    Value limit = contextStructBuilder.limit(rewriter, op->getLoc());

    Value isCountGreaterEqualLimit = rewriter.create<arith::CmpIOp>(
        op.getLoc(), arith::CmpIPredicate::uge, count, limit);

    rewriter.replaceOpWithNewOp<scf::IfOp>(
        op, isCountGreaterEqualLimit,
        /*thenBuilder*/
        [input = adaptor.getInput(),
         symbolTable = this->symbolTable](OpBuilder &b, Location loc) {
          ContextStructBuilder contextStructBuilder(input);
          func::FuncOp callee = lookupRuntimeFunction(symbolTable, "swap");

          // call swap(/*from*/ thread, /*to*/ main);
          Value threadCtx = contextStructBuilder.threadCtx(b, loc);
          Value mainCtx = contextStructBuilder.mainCtx(b, loc);
          b.create<func::CallOp>(loc, callee, ValueRange({threadCtx, mainCtx}));

          // once the game swaps back, reset instruction count to 0
          auto i64Ty = IntegerType::get(b.getContext(), 64);
          auto constOp =
              b.create<arith::ConstantOp>(loc, i64Ty, b.getI64IntegerAttr(0));
          contextStructBuilder.setCount(b, loc, constOp);

          b.create<scf::YieldOp>(loc, Value(contextStructBuilder));
        },
        /*elseBuilder*/
        [input = adaptor.getInput()](OpBuilder &b, Location loc) {
          b.create<scf::YieldOp>(loc, input);
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

    // promote tower runtime context to stack
    LLVMTypeConverter llvmTypeConverter(rewriter.getContext());
    auto ptrTy = LLVM::LLVMPointerType::get(rewriter.getContext());
    Type indexTy = llvmTypeConverter.convertType(rewriter.getIndexType());
    Value one = rewriter.create<LLVM::ConstantOp>(
        op.getLoc(), indexTy, rewriter.getIntegerAttr(indexTy, 1));
    Value allocated = rewriter.create<LLVM::AllocaOp>(
        op.getLoc(), ptrTy, getTowerRuntimeContextType(rewriter.getContext()),
        one, /*alignment=*/0);
    rewriter.create<LLVM::StoreOp>(op.getLoc(), adaptor.getInput(), allocated);

    // call runtime function with pointer to promoted context
    rewriter.replaceOpWithNewOp<func::CallOp>(op, callee, allocated);
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
        builder.create<func::FuncOp>(loc, runtimeFunction.name, functionType);
    functionOp.setPrivate();
  }
}
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

    // Convert non-entry CFG block arguments as well as function signatures.
    // The upstream function-interface pattern converts only the entry block.
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
        rewriter.modifyOpInPlace(op, [&] {
          op.setType(FunctionType::get(op.getContext(),
                                       signature.getConvertedTypes(), results));
        });
        return success();
      }
    };
    patterns.add<ConvertFunction>(typeConverter, context);
    target.addDynamicallyLegalOp<func::FuncOp>([&](func::FuncOp op) {
      return typeConverter.isSignatureLegal(op.getFunctionType()) &&
             typeConverter.isLegal(&op.getBody());
    });

    // Ensure calls sites are converted
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
