#ifndef TOWER_RUNTIME_TOWERRUNTIMEUTILS_H
#define TOWER_RUNTIME_TOWERRUNTIMEUTILS_H

#include "mlir/Dialect/LLVMIR/LLVMTypes.h"
#include "mlir/IR/MLIRContext.h"

namespace mlir::towerRuntime {

LLVM::LLVMStructType getTowerRuntimeContextType(MLIRContext *context);

} // namespace mlir::towerRuntime

#endif // TOWER_RUNTIME_TOWERRUNTIMEUTILS_H