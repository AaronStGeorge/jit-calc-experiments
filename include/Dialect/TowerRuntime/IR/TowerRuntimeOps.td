#ifndef TOWERRUNTIME_OPS
#define TOWERRUNTIME_OPS

include "Dialect/TowerRuntime/IR/TowerRuntimeTypes.td"
include "mlir/Interfaces/InferTypeOpInterface.td"
include "mlir/Interfaces/SideEffectInterfaces.td"

def TowerRuntime_FooOp : TowerRuntime_Op<"foo"> {
    let summary = "Illustrates how to define an operation.";
    let description = [{
        TODO
    }];

    let arguments = (ins TowerRuntime_Context:$input);

    let assemblyFormat = [{
        $input attr-dict `:` type($input)
    }];
}

#endif // TOWERRUNTIME_OPS
