#!/bin/bash

cmake --build build

# put built C++ programs on path
export PATH="$PWD/build/bin:$PATH"
# tell python where to find the MLIR python bindings
export PYTHONPATH="$PWD/build/python_packages/jit_calc"