import argparse
from typing import List, Tuple

import decode


class Function:
    def __init__(self, nparams: int, code: decode.Code):
        self.nparams = nparams
        self.nparams = nparams
        self.code = code


class Machine:
    def __init__(self, functions: List[Function]):
        self.functions = functions
        # 1 gb stack
        self.memory = bytearray(1024 * 1024 * 1024)

    def call(self, func_idx: decode.func_idx, *args: Tuple[any, ...]):
        func = self.functions[func_idx]
        if len(args) != func.nparams:
            s = ("s" if func.nparams > 1 else "",)
            raise ValueError(
                f"func with index {func_idx} expected {func.nparams} arg{s}"
            )
        print(func.code)


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description="Wasm interpreter")
    parser.add_argument("wasm_file", type=str, help="Wasm file to load")
    parser.add_argument("func", type=str, help="Exported function to run")
    parser.add_argument(
        "args",
        type=int,
        nargs="*",
        help="Arguments to exported function(currently limited to just int types)",
    )

    args = parser.parse_args()

    with open(args.wasm_file, "rb") as file:
        # decode module
        stream = iter(bytes(file.read()))
        module = decode.parse_module(stream)

        # pull functions from module
        functions = []
        for type_idx, code in zip(module.func_sec, module.code_sec):
            func_type = module.type_sec[type_idx]
            functions.append(Function(nparams=len(func_type.param_types), code=code))

        # find the function user wants
        exports = {
            export.name: export.ref
            for export in module.export_sec
            if isinstance(export, decode.ExportFunction)
        }
        if args.func not in exports:
            raise ValueError(
                f'"{args.wasm_file}" does not export function named "{args.func}"'
            )

        # let's do it like they do on the discovery channel
        machine = Machine(functions=functions)
        machine.call(exports[args.func], *args.args)
