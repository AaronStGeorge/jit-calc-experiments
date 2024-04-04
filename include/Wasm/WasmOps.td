#ifndef WASM_OPS
#define WASM_OPS

include "Wasm/WasmDialect.td"
include "mlir/Interfaces/InferTypeOpInterface.td"
include "mlir/Interfaces/SideEffectInterfaces.td"

def Wasm_LocalGetOp : Wasm_Op<"local.get", [Pure, SameOperandsAndResultType]> {
    let description = [{
        This is a no op to represent the local.get instruction. In MLIR's SSA
        format we wont't actually be pushing anything onto a stack. This
        instruction just takes the item that *would* have pushed onto the stack
        and returns it.
    }];

    let arguments = (ins AnyType:$input);
    let results = (outs AnyType:$res);

    let assemblyFormat = [{
        $input attr-dict `:` type($input)
    }];
}

def Wasm_I32AddOp : Wasm_Op<"i32.add", [Pure, SameOperandsAndResultType]> {
    let description = [{
        TODO
    }];

    let arguments = (ins I32:$lhs, I32:$rhs);
    let results = (outs I32:$res);

    let assemblyFormat = [{
        $lhs $rhs attr-dict `:` type($lhs)
    }];
}

#endif // WASM_OPS
