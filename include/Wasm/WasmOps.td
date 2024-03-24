#ifndef WASM_OPS
#define WASM_OPS

include "Wasm/WasmDialect.td"
include "mlir/Interfaces/InferTypeOpInterface.td"
include "mlir/Interfaces/SideEffectInterfaces.td"

def Wasm_FooOp : Wasm_Op<"foo", [Pure, SameOperandsAndResultType]> {
    let description = [{
        TODO
    }];

    let arguments = (ins I32:$input);
    let results = (outs I32:$res);

    let assemblyFormat = [{
        $input attr-dict `:` type($input)
    }];
}

#endif // WASM_OPS
