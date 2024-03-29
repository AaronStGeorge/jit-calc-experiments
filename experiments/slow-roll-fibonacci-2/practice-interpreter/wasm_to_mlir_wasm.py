import argparse

from mlir_wasm.ir import *
from mlir_wasm.dialects import func, wasm as wasm_d

import wasm


class Translator(wasm.InstructionVisitor[None]):

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
        pass

    def visit_local_set(self, instruction: wasm.local_set) -> None:
        pass

    def visit_local_tee(self, instruction: wasm.local_tee) -> None:
        pass

    def visit_i32_const(self, instruction: wasm.i32_const) -> None:
        pass

    def visit_i32_ge_s(self, instruction: wasm.i32_ge_s) -> None:
        pass

    def visit_i32_add(self, instruction: wasm.i32_add) -> None:
        pass

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
        i32 = IntegerType.get_signed(32)

        wasm_to_mlir_type = {
            "i32": i32
        }
        wasm_d.register_dialect()
        with Location.unknown() as loc:
            mlir_module = Module.create()

            for export in wasm_module.export_sec:
                if isinstance(export, wasm.ExportFunction):
                    # hack for now, I don't really know what "__wasm_call_ctors" does which seems to be  there in every
                    # wasm blob compiled from a C program and stuff
                    if not export.name.startswith("__"):
                        func_type = wasm_module.type_sec[export.ref]
                        code = wasm_module.func_sec[export.ref]
                        inputs = [wasm_to_mlir_type[t] for t in func_type.param_types]
                        results = [wasm_to_mlir_type[t] for t in func_type.result_types]
                        with InsertionPoint(mlir_module.body):
                            f = func.FuncOp(name=export.name,
                                            type=FunctionType.get(inputs=inputs, results=results)
                                            )
                            with InsertionPoint(f.add_entry_block()):
                                func.ReturnOp([f.entry_block.arguments[0]])

                        # t = Translator
                        # with InsertionPoint(f.body):
                        #     for instr in code.instructions:
                        #         instr.acccept(instr)

            print(mlir_module)
