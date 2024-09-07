#ifndef TOWER_RUNTIME_C_DIALECT_H
#define TOWER_RUNTIME_C_DIALECT_H

#include "mlir-c/IR.h"

#ifdef __cplusplus
extern "C" {
#endif

MLIR_DECLARE_CAPI_DIALECT_REGISTRATION(TowerRuntime, towerRuntime);

/// Creates an tower_runtime.ctx type.
MLIR_CAPI_EXPORTED MlirType mlirTowerRuntimeContextTypeGet(MlirContext ctx);

/// Returns `true` if the type is a TowerRuntime context type.
MLIR_CAPI_EXPORTED bool mlirTypeIsATowerRuntimeContextType(MlirType type);

#ifdef __cplusplus
}
#endif

#endif // TOWER_RUNTIME_C_DIALECT_H
