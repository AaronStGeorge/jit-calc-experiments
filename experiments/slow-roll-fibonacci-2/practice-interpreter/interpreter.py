import argparse
from dataclasses import dataclass, field
from typing import Callable, List, Optional, Tuple, Union, Dict
import decode

WasmType = Union[
    decode.i32,
    decode.u32,
    decode.s32,
    decode.i64,
    decode.u64,
    decode.s64,
    decode.f32,
    decode.f64,
]


@dataclass
class Function:
    nparams: int
    code: decode.Code
    does_return: bool


@dataclass
class Executor(decode.Visitor):
    locals: List[WasmType]
    stack: List[WasmType] = field(default_factory=list)

    def execute(self, instructions: List[decode.Instruction]):
        for instruction in instructions:
            instruction.accept(self)

    def push(self, item: WasmType):
        self.stack.append(item)

    def pop(self):
        return self.stack.pop()

    def binary(self, op: Callable[[WasmType, WasmType], WasmType]):
        right = self.pop()
        left = self.pop()
        self.push(op(left, right))

    def visit_block(self, instruction: decode.block) -> None:
        pass

    def visit_loop(self, instruction: decode.loop) -> None:
        pass

    def visit_unreachable(self, instruction: decode.unreachable) -> None:
        pass

    def visit_nop(self, instruction: decode.nop) -> None:
        pass

    def visit_br(self, instruction: decode.br) -> None:
        pass

    def visit_br_if(self, instruction: decode.br_if) -> None:
        pass

    def visit_local_get(self, instruction: decode.local_get) -> None:
        item = self.locals[instruction.args[0]]
        self.push(item)

    def visit_local_set(self, instruction: decode.local_set) -> None:
        pass

    def visit_local_tee(self, instruction: decode.local_tee) -> None:
        pass

    def visit_i32_const(self, instruction: decode.i32_const) -> None:
        pass

    def visit_i32_ge_s(self, instruction: decode.i32_ge_s) -> None:
        pass

    def visit_i32_add(self, instruction: decode.i32_add) -> None:
        self.binary(lambda right, left: right + left)


class Machine:
    def __init__(self, functions: List[Function]):
        self.functions = functions
        # 1 gb
        self.memory = bytearray(1024 * 1024 * 1024)

    def call(
        self, func_idx: decode.func_idx, *args: Tuple[WasmType, ...]
    ) -> Optional[WasmType]:
        func: Function = self.functions[func_idx]
        if len(args) != func.nparams:
            s = ("s" if func.nparams > 1 else "",)
            raise ValueError(
                f"func with index {func_idx} expected {func.nparams} arg{s}"
            )
        # If a function has 4 locals of type i32 and 2 of type f32 (spelled out
        # in python: "[Local(type="i32", n=4), Local(type="f32", n=2)]") the
        # instructions of said function will contain get and set instructions
        # expecting the locals to be ordered: [i32,i32,i32,i32, f32,f32]. Since
        # python doesn't care about types. we just splat everything into one big
        # array of 0s.
        splatted_locals = sum(([0] * local.n for local in func.code.locals), [])
        # From the wasm spec: "The index space for locals is only accessible
        # inside a function and includes the parameters of that function, which
        # precede the local variables." So for a function with 1 param of type
        # i32, 2 params of type i64, 4 locals of type i32, and 2 locals of type
        # f32 the locals are expected to be laid out in this order:
        #   [i32, i64,i64, i32,i32,i32,i32, f32,f32]
        locals = list(args) + splatted_locals
        exec = Executor(locals=locals)
        exec.execute(func.code.expr.instructions)
        if func.does_return:
            return exec.pop()


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
            functions.append(
                Function(
                    nparams=len(func_type.param_types),
                    code=code,
                    does_return=bool(func_type.result_types),
                )
            )

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
        result = machine.call(exports[args.func], *args.args)
        if result:
            print(result)
