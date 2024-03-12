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
class Break:
    target_idx: decode.label_idx


@dataclass
class Executor(decode.Visitor[Optional[Break]]):
    wasm_locals: List[WasmType]
    stack: List[WasmType] = field(default_factory=list)

    def execute(self, instructions: List[decode.Instruction]) -> Optional[Break]:
        for instruction in instructions:
            maybe_br = instruction.accept(self)
            if maybe_br:
                return maybe_br

    def push(self, item: WasmType):
        self.stack.append(item)

    def pop(self):
        return self.stack.pop()

    def binary(self, op: Callable[[WasmType, WasmType], WasmType]):
        right = self.pop()
        left = self.pop()
        self.push(op(left, right))

    def visit_block(self, instruction: decode.block) -> Optional[Break]:
        """
        From the wasm spec:
            Unlike with other index spaces, indexing of labels is relative by
            nesting depth, that is, label 0 refers to the innermost structured
            control instruction enclosing the referring branch instruction, while
            increasing indices refer to those farther out. Consequently, labels can
            only be referenced from within the associated structured control
            instruction. This also implies that branches can only be directed
            outwards, “breaking” from the block of the control construct they
            target.  The exact effect depends on that control construct. From the
            wasm spec "In case of block or if it is a forward jump, resuming
            execution after the matching end. In case of loop it is a backward jump
            to the beginning of the loop.

        Example of block behavior:
            # if (test) { consequence } else {alternative }
            ('block', [
                        ('block', [
                                    test
                                    ('br_if, 0),  # Goto 0
                                    alternative,
                                    ('br', 1),    # Goto 1
                                  ]
                        ),  # Label: 0
                        consequence,
                      ]
            ) # Label: 1
        """
        maybe_br = self.execute(instruction.instructions)
        if maybe_br and maybe_br.target_idx == 0:
            # a break statement targeted this block, we should execute
            # "consequence" from the example in the docstring AKA the next
            # instruction in the current scope.
            return None
        elif maybe_br and maybe_br.target_idx > 0:
            # a break statement targeted a scope greater than this block,
            # decrement the scope count and continue breaking.
            maybe_br.target_idx -= 1
            return maybe_br
        else:
            # we ran off the end of the block, execute next instruction
            return None

    def visit_loop(self, instruction: decode.loop) -> None:
        """
        From the wasm spec:
            Unlike with other index spaces, indexing of labels is relative by
            nesting depth, that is, label 0 refers to the innermost structured
            control instruction enclosing the referring branch instruction, while
            increasing indices refer to those farther out. Consequently, labels can
            only be referenced from within the associated structured control
            instruction. This also implies that branches can only be directed
            outwards, “breaking” from the block of the control construct they
            target.  The exact effect depends on that control construct. From the
            wasm spec "In case of block or if it is a forward jump, resuming
            execution after the matching end. In case of loop it is a backward jump
            to the beginning of the loop.

        Examples of loop behavior:
            # while (test) { body }
            ('block', [
                        ('loop', [ # Label: 0
                                    not test
                                    ('br_if', 1), # Goto 1:  (break)
                                    body
                                    ('br', 0),    # Goto 0:  (continue)
                                 ]
                        ),
                      ]
            ) # Label: 1


            # for(int i = 0; i < n; i++) { body }
            # # which gets converted to # #
            #   while (n) {
            #       body;
            #       n --;
            #   }
            ('block', [
                        ('block', [
                                    load n >= 1 onto stack
                                    ('br_if, 0),  # Goto 0
                                    ('br', 1),    # Goto 1
                                  ]
                        ),  # Label: 0 from inner block
                        ('loop', [ # Label: 0 from inner loop
                                    body
                                    n --;
                                    load n >= 1 onto stack
                                    ('br_if', 0),    # Goto 0:  (continue)
                                 ]
                        ),
                      ]
            ) # Label: 1 from inner block
        """
        # execute first iteration of loop
        maybe_br = self.execute(instruction.instructions)
        # continue to execute loop iterations as long as a break targeting the
        # loop ("continue") is issued from inside the loop.
        while maybe_br and maybe_br.target_idx == 0:
            maybe_br = self.execute(instruction.instructions)

        if maybe_br and maybe_br.target_idx > 0:
            # a break statement targeted a scope greater than this block,
            # decrement the scope count and continue breaking.
            maybe_br.target_idx -= 1
            return maybe_br
        else:
            # we ran off the end of the loop
            return None

    def visit_unreachable(self, instruction: decode.unreachable) -> None:
        pass

    def visit_nop(self, instruction: decode.nop) -> None:
        pass

    def visit_br(self, instruction: decode.br) -> Optional[Break]:
        return Break(instruction.args[0])

    def visit_br_if(self, instruction: decode.br_if) -> Optional[Break]:
        if self.pop():
            return Break(instruction.args[0])

    def visit_local_get(self, instruction: decode.local_get) -> None:
        self.push(self.wasm_locals[instruction.args[0]])

    def visit_local_set(self, instruction: decode.local_set) -> None:
        self.wasm_locals[instruction.args[0]] = self.pop()

    def visit_local_tee(self, instruction: decode.local_tee) -> None:
        self.wasm_locals[instruction.args[0]] = self.stack[-1]

    def visit_i32_const(self, instruction: decode.i32_const) -> None:
        self.push(decode.i32(instruction.args[0]))

    def visit_i32_ge_s(self, instruction: decode.i32_ge_s) -> None:
        self.binary(lambda right, left: decode.i32(right >= left))

    def visit_i32_add(self, instruction: decode.i32_add) -> None:
        self.binary(lambda right, left: decode.i32(right + left))


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
        # expecting the locals to be ordered: [i32,i32,i32,i32, f32,f32].
        #
        # Python doesn't care about types, so we just splat everything as one
        # big array of None`s. A well formed program won't read the local values
        # before writing them, so the value doesn't matter, we're just creating
        # slots.
        splatted_wasm_locals = sum(([None] * local.n for local in func.code.locals), [])
        # From the wasm spec: "The index space for locals is only accessible
        # inside a function and includes the parameters of that function, which
        # precede the local variables." So for a function with 1 param of type
        # i32, 2 params of type i64, 4 locals of type i32, and 2 locals of type
        # f32 the locals are expected to be laid out in this order:
        #   [i32, i64,i64, i32,i32,i32,i32, f32,f32]
        wasm_locals = list(args) + splatted_wasm_locals
        exec = Executor(wasm_locals=wasm_locals)
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

    prog_args = parser.parse_args()

    with open(prog_args.wasm_file, "rb") as file:
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
        if prog_args.func not in exports:
            raise ValueError(
                f'"{prog_args.wasm_file}" does not export function named "{prog_args.func}"'
            )

        # let's do it like they do on the discovery channel
        machine = Machine(functions=functions)
        result = machine.call(exports[prog_args.func], *prog_args.args)
        print(result)
