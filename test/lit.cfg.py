# -*- Python -*-

import os

import lit.formats
import lit.util

from lit.llvm import llvm_config

# Configuration file for the 'lit' test runner.

# name: The name of this test suite.
config.name = "JIT_CALC_EXPERIMENTS"

# not sure what this does, but it's needed
config.test_format = lit.formats.ShTest(not llvm_config.use_lit_shell)

# suffixes: A list of file extensions to treat as test files.
config.suffixes = [".mlir"]

# test_source_root: The root path where tests are located.
config.test_source_root = os.path.dirname(__file__)

# test_exec_root: The root path where tests should be run.
config.test_exec_root = os.path.join(config.jit_calc_experiments_obj_root, "test")

# substitutions: tokens beginning with a sigil "%" that will be replaced
# shlib_ext: shared library extension, which will be ".so" on linux
config.substitutions.append(("%shlibext", config.llvm_shlib_ext))
# jit_calc_experiments_libs: path of lib directory, probably used for generated
# code to call into  runtime functions compiled into .so files
config.substitutions.append(
    (
        "%jit_calc_experiments_libs",
        os.path.join(config.jit_calc_experiments_obj_root, "lib"),
    )
)
# mlir_lib_dir: same thing as jit_calc_experiments_libs but for shared libraries
# that are from MLIR
config.substitutions.append(("%mlir_lib_dir", config.mlir_lib_dir))

llvm_config.use_default_substitutions()

# test_exec_root: The root path where tests should be run.
config.test_exec_root = os.path.join(config.jit_calc_experiments_obj_root, "test")

# Tweak the PATH to include the tools dir.
llvm_config.with_environment("PATH", config.llvm_tools_dir, append_path=True)

# setup substitutions for tools
config.jit_calc_experiments_tools_dir = os.path.join(
    config.jit_calc_experiments_obj_root, "bin"
)
tool_dirs = [config.jit_calc_experiments_tools_dir, config.llvm_tools_dir]
tools = [
    "jit-calc-opt",
    "mlir-opt",
    "mlir-runner",
]
llvm_config.add_tool_substitutions(tools, tool_dirs)
