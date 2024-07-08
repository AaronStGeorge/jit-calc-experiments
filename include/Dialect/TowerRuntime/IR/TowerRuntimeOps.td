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

def TowerRuntime_CheckSwapOp : TowerRuntime_Op<"check_swap"> {
    let summary = "Swaps if context op count is reached.";
    let description = [{
        TowerRuntime context stores a tread context and an op limit. CheckSwapOp
        checks if the op count has reached the op limit and if so swaps to the
        stored thread context.
    }];

    let arguments = (ins TowerRuntime_Context:$input);

    let assemblyFormat = [{
        $input attr-dict `:` type($input)
    }];
}

def TowerRuntime_IncrementCountOp : TowerRuntime_Op<"increment_count"> {
    let summary = "Increment TowerRuntime context op count.";
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
