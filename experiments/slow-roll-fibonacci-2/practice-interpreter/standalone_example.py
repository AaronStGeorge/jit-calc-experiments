# import gc, sys, os, tempfile
# from mlir.ir import *
# from mlir.passmanager import *
# from mlir.execution_engine import *
# from mlir.runtime import *
#
import ctypes
import sys
from mlir_wasm.ir import *
from mlir_wasm.passmanager import *
from mlir_wasm.dialects import builtin as builtin_d, standalone as standalone_d

def lowerToLLVM(module):
    pm = PassManager.parse(
        "builtin.module(convert-complex-to-llvm,finalize-memref-to-llvm,convert-func-to-llvm,reconcile-unrealized-casts)"
    )
    pm.run(module.operation)
    return module

def log(*args):
    print(*args, file=sys.stderr)
    sys.stderr.flush()


with Context():
    standalone_d.register_dialect()
    module = Module.parse(
        """
    %0 = arith.constant 2 : i32
    %1 = standalone.foo %0 : i32
    """
    )
    # CHECK: %[[C:.*]] = arith.constant 2 : i32
    # CHECK: standalone.foo %[[C]] : i32
    print(str(module))

