#ifndef WASM_C_DIALECT_H
#define WASM_C_DIALECT_H

#include "mlir-c/IR.h"

#ifdef __cplusplus
extern "C" {
#endif

MLIR_DECLARE_CAPI_DIALECT_REGISTRATION(Wasm, wasm);

#ifdef __cplusplus
}
#endif

#endif // WASM_C_DIALECT_H
