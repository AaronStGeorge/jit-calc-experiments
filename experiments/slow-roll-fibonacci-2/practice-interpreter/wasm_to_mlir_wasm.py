import argparse

from dataclasses import dataclass, field
from typing import List

import mlir_wasm._mlir_libs._mlir.ir
from mlir_wasm.ir import *
from mlir_wasm.dialects import func, wasm as wasm_d

import wasm


@dataclass
class Translator(wasm.InstructionVisitor[None]):
    wasm_locals: List[Value]
    stack: List[Value] = field(default_factory=list)

    def push(self, item: wasm.WasmType):
        self.stack.append(item)

    def pop(self):
        return self.stack.pop()

    def visit_loop(self, instruction: wasm.loop) -> None:
        pass

    def visit_unreachable(self, instruction: wasm.unreachable) -> None:
        pass

    def visit_nop(self, instruction: wasm.nop) -> None:
        pass

    def visit_br(self, instruction: wasm.br) -> None:
        pass

    def visit_br_if(self, instruction: wasm.br_if) -> None:
        pass

    def visit_local_get(self, instruction: wasm.local_get) -> None:
        self.push(wasm_d.local_get(wasm_locals[instruction.args[0]]))

    def visit_local_set(self, instruction: wasm.local_set) -> None:
        pass

    def visit_local_tee(self, instruction: wasm.local_tee) -> None:
        pass

    def visit_i32_const(self, instruction: wasm.i32_const) -> None:
        pass

    def visit_i32_ge_s(self, instruction: wasm.i32_ge_s) -> None:
        pass

    def visit_i32_add(self, instruction: wasm.i32_add) -> None:
        lhs = self.pop()
        rhs = self.pop()
        self.push(wasm_d.i32_add(lhs, rhs))

    def visit_block(self, instruction: wasm.block) -> None:
        pass


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description="Translate wasm to mlir")
    parser.add_argument("wasm_file", type=str, help="Wasm file to load")

    prog_args = parser.parse_args()

    with open(prog_args.wasm_file, "rb") as file:
        # decode module
        stream = iter(bytes(file.read()))
        wasm_module = wasm.decode(stream)

    with Context():
        i32 = IntegerType.get_signless(32)

        wasm_to_mlir_type = {
            "i32": i32
        }
        wasm_d.register_dialect()
        with Location.unknown() as loc:
            mlir_module = Module.create()

            for wasm_func in wasm_module.export_sec:
                if isinstance(wasm_func, wasm.ExportFunction):
                    # hack for now, I don't really know what "__wasm_call_ctors" does which seems to be  there in every
                    # wasm blob compiled from a C program and stuff
                    if not wasm_func.name.startswith("__"):
                        func_type = wasm_module.type_sec[wasm_func.ref]
                        code = wasm_module.code_sec[wasm_func.ref]
                        inputs = [wasm_to_mlir_type[t] for t in func_type.param_types]
                        results = [wasm_to_mlir_type[t] for t in func_type.result_types]
                        with InsertionPoint(mlir_module.body):
                            mlir_func = func.FuncOp(name=wasm_func.name,
                                                    type=FunctionType.get(inputs=inputs, results=results)
                                                    )
                            with InsertionPoint(mlir_func.add_entry_block()):
                                # From the wasm spec: "The index space for locals is only accessible
                                # inside a function and includes the parameters of that function, which
                                # precede the local variables." So for a function with 1 param of type
                                # i32, 2 params of type i64, 4 locals of type i32, and 2 locals of type
                                # f32 the locals are expected to be laid out in this order:
                                #   [i32, i64,i64, i32,i32,i32,i32, f32,f32]
                                wasm_locals = mlir_func.entry_block.arguments

                                translator = Translator(wasm_locals=wasm_locals)
                                for instr in code.expr.instructions:
                                    instr.accept(translator)

                                # add return if there is one
                                if func_type.result_types:
                                    func.ReturnOp([translator.pop()])

            print(mlir_module)
