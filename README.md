# jit-calc-experiments

🧪 Experiments around runtime systems and jit compilers. Stuff that could maybe,
one day, be squished into the shape of a runtime engine for a game where NPC
robots execute player code.

## Pre-requisites

Look, "experiments" is in the name, there's going to be some weird shit.
`jit-calc-experiments` requires:

#### An Apple silicon mac

A ARM based linux machine would probably work as well, but an x86 machine definitely won't. Some functions are written in [AArch64 assembly](https://github.com/AaronStGeorge/jit-calc-experiments/blob/main/lib/HomeRolledCoroutines/home_rolled_coroutines.s).

#### A `clang` that can compile WebAssembly, some other C++ build tools, and raylib

On Mac, brew clang seems to handle WebAssembly.
Brew has the other stuff we need as well.

```sh
$ brew install llvm cmake ninja pkg-config raylib
```
Homebrew doesn't put clang on the regular homebrew path (IDK why).
So, outside of regular homebrew setup, you'll want to add this to your `.bashrc` or `.zshrc`.
```sh
export PATH="/opt/homebrew/opt/llvm/bin:$PATH"
```

#### A Python virtual environment with the MLIRy stuff installed

You'll need to have [LLVM](https://github.com/llvm/llvm-project) pulled down for this.
```sh
$ git clone https://github.com/llvm/llvm-project.git
$ cd llvm-project
```
Next, you'll need to create a python virtual environment.
```sh
$ python3 -m venv ~/.venv/mlirdev
# all commands after this point assume that you have your virtual environment activated.
$ source ~/.venv/mlirdev/bin/activate
```
Then install the MLIRy stuff.
```sh
(mlirdev) $ python -m pip install --upgrade pip
(mlirdev) $ python -m pip install -r mlir/python/requirements.txt
```

See MLIR python bindings virtual environment setup [docs](https://mlir.llvm.org/docs/Bindings/Python/#building) for more info on the python bindings.

#### LLVM Source build

Though `jit-calc-experiments` doesn't care _where_ LLVM is built, it requires a few `LLVM` utilities and projects that aren't part of some build you would just download from brew (I think).
We have to set a whole bunch of CMake flags to build LLVM with the right stuff.
```sh
(mlirdev) $ mkdir llvm-project/build && cd llvm-project/build

# To compile LLVM you first have to generate the build script with CMake. Taking
# a look at some of the flags:
#
#   LLVM_ENABLE_PROJECTS="mlir"        - MLIR project must be enabled.
#   LLVM_INSTALL_UTILS=ON              - Install utilities, like lit, needed for
#                                        testing jit-calc.
#   LLVM_TARGETS_TO_BUILD="AArch64"    - Target architectures.
#   DCMAKE_BUILD_TYPE=Debug            - The options are Release, Debug, or RelWithDebInfo.
#                                        You'll probably want to step through stuff
#                                        in a debugger, hence Debug.
#   DLLVM_ENABLE_LLD=ON                - Link with LLD rather than system linker. Seconds
#                                        instead of minutes, well worth it.
#   LLVM_PARALLEL_LINK_JOBS=1          - To prevent swapping do one link job per 16 GB of
#                                        RAM available. Linking will take FOREVER without
#                                        this.
#   DMLIR_ENABLE_BINDINGS_PYTHON=ON    - We use the python bindings in jit-calc-experiments.
#   LLVM_INSTALL_GTEST=ON              - LLVM has a vendored version of gtest, we want to
#                                        use that version for writing our tests in
#                                        jit-calc-experiments.
#   DCMAKE_EXPORT_COMPILE_COMMANDS=1   - This creates whatever clangd needs to do it's jump
#                                        to definition magic. If you're not using clnagd you
#                                        don't need this.
(mlirdev) $ cmake -G Ninja ../llvm \
    -DCMAKE_C_COMPILER=$(which clang) \
    -DCMAKE_CXX_COMPILER=$(which clang++) \
    -DPython3_EXECUTABLE=$(which python) \
    -DLLVM_ENABLE_PROJECTS="mlir" \
    -DLLVM_INSTALL_UTILS=ON \
    -DLLVM_TARGETS_TO_BUILD="AArch64" \
    -DCMAKE_BUILD_TYPE=Debug \
    -DLLVM_ENABLE_LLD=ON \
    -DLLVM_PARALLEL_LINK_JOBS=1 \
    -DMLIR_ENABLE_BINDINGS_PYTHON=ON \
    -DLLVM_INSTALL_GTEST=ON \
    -DCMAKE_EXPORT_COMPILE_COMMANDS=1

# build all targets
(mlirdev) $ ninja
```

For more info, the `LLVM` docs have great [instructions on how to build LLVM](https://llvm.org/docs/CMake.html).

## Building `jit-calc-experiments`

The `jit-calc-experiments` CMake build needs some flags to tell it where LLVM is.
It also needs to find `raylib`, but if you installed that with homebrew it should "just work."

The following command assumes `$LLVM_BUILD_DIR` environment variable holds the path of your LLVM build.
```sh
(mlirdev) $ mkdir build && cd build
# generate build script, the CMake options are pretty much the same as the LLVM
# with the following exceptions:
#
#   DMLIR_DIR=$LLVM_BUILD_DIR/lib/cmake/mlir          - The `jit-calc-experiments` build needs to know where the
#                                                       LLVM build is.
#   DLLVM_EXTERNAL_LIT=$LLVM_BUILD_DIR/bin/llvm-lit   - The `lit` tool (built during `LLVM` build) is needed for
#                                                       testing. Not sure why this needs its own flag rather than
#                                                       being inferred from DMLIR_DIR.
(mlirdev) $ cmake -G Ninja .. \
    -DMLIR_DIR=$LLVM_BUILD_DIR/lib/cmake/mlir \
    -DLLVM_EXTERNAL_LIT=$LLVM_BUILD_DIR/bin/llvm-lit
    -DCMAKE_BUILD_TYPE=Debug \
    -DLLVM_PARALLEL_LINK_JOBS=1 \
    -DCMAKE_C_COMPILER=$(which clang) \
    -DCMAKE_CXX_COMPILER=$(which clang++) \
    -DPython3_EXECUTABLE=$(which python) \
    -DLLVM_USE_LINKER=lld \
    -DMLIR_ENABLE_BINDINGS_PYTHON=ON \
    -DCMAKE_EXPORT_COMPILE_COMMANDS=1

# build all targets
(mlirdev) $ ninja
```
At this point you should be able to run the tests
```sh
(mlirdev) $ ninja check-jit-calc-experiments
```

## Running `wasm_to_mlir.py`

If you would like to run wasm_to_mlir.py, such as the `slow-roll-fibonacci-2` experiment
```sh
(mlirdev) $ python python/wasm_to_mlir.py experiments/slow-roll-fibonacci-2/fixtures/fib.wasm \
	| jit-calc-opt --shim-function='function-to-shim=fib' --convert-tower-runtime-to-llvm \
	| srf2-driver
```
you'll need to have the virtual environment active, and you'll need to set the `$PYTHONPATH` environment variable to pick up the MLIR python bindings.
The above command also assumes that `jit-calc-opt` (one of the programs `jit-calc-experiments` builds) is on your `$PATH`.
[`source_me_for_python_bindings.sh`](https://github.com/AaronStGeorge/jit-calc-experiments/blob/main/source_me_for_python_bindings.sh) will set your `$PATH` and `$PYTHONPATH`.
```sh
(mlirdev) $ source source_me_for_python_bindings.sh
```