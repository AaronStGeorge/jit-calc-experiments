#include "jit-calc-mlir-c/Dialect/TowerRuntime.h"

#include "jit-calc-mlir/Dialect/TowerRuntime/IR/TowerRuntimeDialect.h"
#include "mlir/CAPI/Registration.h"

MLIR_DEFINE_CAPI_DIALECT_REGISTRATION(TowerRuntime, towerRuntime, mlir::towerRuntime::TowerRuntimeDialect)
