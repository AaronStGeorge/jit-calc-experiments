#ifndef TOWERRUNTIME_DIALECT
#define TOWERRUNTIME_DIALECT

include "mlir/IR/OpBase.td"

//===----------------------------------------------------------------------===//
// TowerRuntime dialect definition.
//===----------------------------------------------------------------------===//

def TowerRuntime_Dialect : Dialect {
    let name = "tower_runtime";
    let summary = "TODO";
    let description = [{
        TODO
    }];
    let cppNamespace = "::mlir::towerRuntime";

    let useDefaultTypePrinterParser = 1;
    let extraClassDeclaration = [{
        void registerTypes();
    }];
}

//===----------------------------------------------------------------------===//
// Base tower runtime operation definition.
//===----------------------------------------------------------------------===//

class TowerRuntime_Op<string mnemonic, list<Trait> traits = []> :
        Op<TowerRuntime_Dialect, mnemonic, traits>;

#endif // TOWERRUNTIME_DIALECT
