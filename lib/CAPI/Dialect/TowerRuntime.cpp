#include "jit-calc-mlir-c/Dialect/TowerRuntime.h"

#include "jit-calc-mlir/Dialect/TowerRuntime/IR/TowerRuntimeDialect.h"
#include "jit-calc-mlir/Dialect/TowerRuntime/IR/TowerRuntimeTypes.h"
#include "mlir/CAPI/Registration.h"

#include "mlir-c/IR.h"

MLIR_DEFINE_CAPI_DIALECT_REGISTRATION(TowerRuntime, towerRuntime,
                                      mlir::towerRuntime::TowerRuntimeDialect)

MlirType mlirTowerRuntimeContextTypeGet(MlirContext ctx) {
  return wrap(mlir::towerRuntime::ContextType::get(unwrap(ctx)));
}

bool mlirTypeIsATowerRuntimeContextType(MlirType type) {
  return mlir::isa<mlir::towerRuntime::ContextType>(unwrap(type));
}