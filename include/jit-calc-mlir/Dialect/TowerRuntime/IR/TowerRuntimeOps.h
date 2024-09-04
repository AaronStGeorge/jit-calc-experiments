#ifndef TOWERRUNTIME_TOWERRUNTIMEEOPS_H
#define TOWERRUNTIME_TOWERRUNTIMEEOPS_H

#include "mlir/IR/BuiltinTypes.h"
#include "mlir/IR/Dialect.h"
#include "mlir/IR/OpDefinition.h"
#include "mlir/Interfaces/InferTypeOpInterface.h"
#include "mlir/Interfaces/SideEffectInterfaces.h"

#include "jit-calc-mlir/Dialect/TowerRuntime/IR/TowerRuntimeTypes.h"

#define GET_OP_CLASSES
#include "jit-calc-mlir/Dialect/TowerRuntime/IR/TowerRuntimeOps.h.inc"

#endif // TOWERRUNTIME_TOWERRUNTIMEEOPS_H
