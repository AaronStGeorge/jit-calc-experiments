#ifndef TOWER_RUNTIME_TOWERRUNTIMEPASSES_H
#define TOWER_RUNTIME_TOWERRUNTIMEPASSES_H

#include "jit-calc-mlir/Dialect/TowerRuntime/IR/TowerRuntimeDialect.h"
#include "jit-calc-mlir/Dialect/TowerRuntime/IR/TowerRuntimeOps.h"
#include "mlir/Dialect/LLVMIR/LLVMDialect.h"
#include "mlir/Pass/Pass.h"

namespace mlir {
namespace towerRuntime {
#define GEN_PASS_DECL
#define GEN_PASS_REGISTRATION
#include "jit-calc-mlir/Dialect/TowerRuntime/Transforms/Passes.h.inc"
} // namespace towerRuntime
} // namespace mlir

#endif // TOWER_RUNTIME_TOWERRUNTIMEPASSES_H
