#include "Dialect/Wasm/IR/WasmOps.h"
#include "mlir/Dialect/Arith/IR/Arith.h"
#include "mlir/Dialect/Func/IR/FuncOps.h"
#include "mlir/IR/PatternMatch.h"
#include "mlir/Rewrite/FrozenRewritePatternSet.h"
#include "mlir/Support/LogicalResult.h"
#include "mlir/Transforms/GreedyPatternRewriteDriver.h"

#include "Dialect/Wasm/Transforms/Passes.h"

namespace mlir::wasm {
#define GEN_PASS_DEF_LOWERWASM
#include "Dialect/Wasm/Transforms/Passes.h.inc"

namespace {

class AddOps : public OpRewritePattern<wasm::I32AddOp> {
public:
  using OpRewritePattern<wasm::I32AddOp>::OpRewritePattern;
  LogicalResult matchAndRewrite(wasm::I32AddOp op,
                                PatternRewriter &rewriter) const final {
    rewriter.replaceOpWithNewOp<arith::AddIOp>(op, op.getLhs(), op.getRhs());
    return success();
  }
};

// TODO: this should probably be a folder?
class LocalGet : public OpRewritePattern<wasm::LocalGetOp> {
public:
  using OpRewritePattern<wasm::LocalGetOp>::OpRewritePattern;
  LogicalResult matchAndRewrite(wasm::LocalGetOp op,
                                PatternRewriter &rewriter) const final {
    rewriter.replaceAllOpUsesWith(op, op.getInput());
    return success();
  }
};

class SwitchBarFoo : public impl::LowerWasmBase<SwitchBarFoo> {
public:
  using impl::LowerWasmBase<SwitchBarFoo>::LowerWasmBase;
  void runOnOperation() final {
    RewritePatternSet patterns(&getContext());
    patterns.add<AddOps, LocalGet>(&getContext());
    FrozenRewritePatternSet patternSet(std::move(patterns));
    if (failed(applyPatternsAndFoldGreedily(getOperation(), patternSet)))
      signalPassFailure();
  }
};
} // namespace
} // namespace mlir::wasm
