from mlir_wasm.ir import *
from mlir_wasm.dialects import builtin as builtin_d, wasm as wasm_d

with Context():
    wasm_d.register_dialect()
    module = Module.parse(
        """
    %0 = arith.constant 2 : i32
    %1 = wasm.local.get %0 : i32
    """
    )
    print(str(module))
