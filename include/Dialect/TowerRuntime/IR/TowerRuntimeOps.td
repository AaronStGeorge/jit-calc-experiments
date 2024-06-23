#ifndef TOWERRUNTIME_OPS
#define TOWERRUNTIME_OPS

include "Dialect/TowerRuntime/IR/TowerRuntimeTypes.td"
include "mlir/Interfaces/InferTypeOpInterface.td"
include "mlir/Interfaces/SideEffectInterfaces.td"

def TowerRuntime_FooOp : TowerRuntime_Op<"foo", [Pure,
                                             SameOperandsAndResultType]> {
    let summary = "Illustrates how to define an operation.";
    let description = [{
        TODO
    }];

    let arguments = (ins I32:$input);
    let results = (outs I32:$res);

    let assemblyFormat = [{
        $input attr-dict `:` type($input)
    }];
}

#endif // TOWERRUNTIME_OPS
