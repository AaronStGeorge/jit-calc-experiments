#ifndef WASM_DIALECT
#define WASM_DIALECT

include "mlir/IR/OpBase.td"

//===----------------------------------------------------------------------===//
// Wasm dialect definition.
//===----------------------------------------------------------------------===//

def Wasm_Dialect : Dialect {
    let name = "wasm";
    let summary = "An MLIR representation for web assembly";
    let description = [{
        TODO
    }];
    let cppNamespace = "::mlir::wasm";
}

//===----------------------------------------------------------------------===//
// Base Wasm operation definition.
//===----------------------------------------------------------------------===//

class Wasm_Op<string mnemonic, list<Trait> traits = []> :
        Op<Wasm_Dialect, mnemonic, traits>;

#endif // WASM_DIALECT
