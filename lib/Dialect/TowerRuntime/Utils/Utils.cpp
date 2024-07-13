#include "Dialect/TowerRuntime/Utils/Utils.h"

namespace mlir::towerRuntime {

LLVM::LLVMStructType getTowerRuntimeContextType(MLIRContext *context) {
  auto i64Ty = IntegerType::get(context, 64);
  return LLVM::LLVMStructType::getLiteral(context,
                                          {i64Ty, i64Ty, i64Ty, i64Ty});
}

} // namespace mlir::towerRuntime