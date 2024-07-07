#ifndef TOWERRUNTIME_OPS
#define TOWERRUNTIME_OPS

include "Dialect/TowerRuntime/IR/TowerRuntimeTypes.td"
include "mlir/Interfaces/InferTypeOpInterface.td"
include "mlir/Interfaces/SideEffectInterfaces.td"

def TowerRuntime_DumpContextOp : TowerRuntime_Op<"dump"> {
    let summary = "Dumps a TowerRuntime context to stdout.";
    let description = [{
        Dumps a TowerRuntime context to stdout.
    }];

    let arguments = (ins TowerRuntime_Context:$input);

    let assemblyFormat = [{
        $input attr-dict `:` type($input)
    }];
}

def TowerRuntime_IncrementCountOp : TowerRuntime_Op<"increment_count"> {
    let summary = "Increment TowerRuntime context count.";
    let description = [{
        Increment TowerRuntime context count.
    }];

    let arguments = (ins TowerRuntime_Context:$input);
    let results = (outs TowerRuntime_Context);

    let assemblyFormat = [{
        $input attr-dict `:` type($input)
    }];
}

#endif // TOWERRUNTIME_OPS
