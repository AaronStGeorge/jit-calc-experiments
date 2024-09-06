#ifndef TOWER_RUNTIME_C_DIALECT_H
#define TOWER_RUNTIME_C_DIALECT_H

#include "mlir-c/IR.h"

#ifdef __cplusplus
extern "C" {
#endif

MLIR_DECLARE_CAPI_DIALECT_REGISTRATION(TowerRuntime, towerRuntime);

#ifdef __cplusplus
}
#endif

#endif // TOWER_RUNTIME_C_DIALECT_H
