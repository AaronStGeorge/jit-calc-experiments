#ifndef TOWER_RUNTIME_TOWERRUNTIMEPASSES_H
#define TOWER_RUNTIME_TOWERRUNTIMEPASSES_H

#include "Dialect/TowerRuntime/IR/TowerRuntimeDialect.h"
#include "Dialect/TowerRuntime/IR/TowerRuntimeOps.h"
#include "mlir/Pass/Pass.h"

namespace mlir {
namespace towerRuntime {
#define GEN_PASS_DECL
#define GEN_PASS_REGISTRATION
#include "Dialect/TowerRuntime/Transforms/Passes.h.inc"
} // namespace towerRuntime
} // namespace mlir

#endif // TOWER_RUNTIME_TOWERRUNTIMEPASSES_H
