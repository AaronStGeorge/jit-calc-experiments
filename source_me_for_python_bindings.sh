#!/bin/bash

# Source from the repository root. Allow a different CMake build directory.
if ! cmake --build "${JIT_CALC_BUILD_DIR:-build}"; then
  return 1
fi

_jit_calc_build_dir=$(cd "${JIT_CALC_BUILD_DIR:-build}" && pwd -P) || return 1
export PATH="$_jit_calc_build_dir/bin:$PATH"
export PYTHONPATH="$_jit_calc_build_dir/python_packages/jit_calc${PYTHONPATH:+:$PYTHONPATH}"
unset _jit_calc_build_dir