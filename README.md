# jit-calc-experiments

🧪 Experiments around runtime systems and jit compilers. Stuff that could maybe,
one day, be squished into the shape of a runtime engine for a game where NPC
robots execute player code.

## Pre-requisites

Look, "experiments" is in the name, there's going to be some weird shit.
`jit-calc-experiments` requires:

#### An Apple silicon mac

An ARM64 linux machine would probably work as well, but an x86 machine definitely won't; some functions are written in [ARM64 assembly](https://github.com/AaronStGeorge/jit-calc-experiments/blob/main/lib/HomeRolledCoroutines/home_rolled_coroutines.s).

#### A `clang` that can compile WebAssembly, some other C++ build tools, and raylib

On Mac, brew `clang` seems to handle WebAssembly.
Brew has the other stuff we need as well.

```sh
$ brew install llvm cmake ninja pkg-config raylib
```
Homebrew doesn’t add `clang` to its standard path (IDK why).
So, outside of regular homebrew setup, you'll want to add this to your `.bashrc` or `.zshrc`.
```sh
export PATH="/opt/homebrew/opt/llvm/bin:$PATH"
```

#### A python virtual environment with the MLIRy stuff installed

You'll need to have [LLVM](https://github.com/llvm/llvm-project) pulled down for this.
```sh
$ git clone https://github.com/llvm/llvm-project.git
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
(mlirdev) $ python -m pip install -r llvm-project/mlir/python/requirements.txt
```

See MLIR python bindings virtual environment setup [docs](https://mlir.llvm.org/docs/Bindings/Python/#building) for more info on the python bindings.

#### LLVM Source build

`jit-calc-experiments` doesn't care _where_ LLVM is built, but it requires a few LLVM utilities and projects that aren't part of a "regular" build that one might download from brew.
```sh
(mlirdev) $ mkdir llvm-project/build
(mlirdev) $ cd llvm-project/build

# To compile LLVM, we first need to generate the build system with CMake. We’ll use CMake
# variables (spelled -DVARIABLE=VALUE in CMake) to turn on some non-standard build
# features. Here’s an overview of the variables we’re setting:
#
#   LLVM_ENABLE_PROJECTS="mlir"        - MLIR project must be enabled.
#   LLVM_INSTALL_UTILS=ON              - Install utilities, like lit, needed for
#                                        testing jit-calc.
#   LLVM_TARGETS_TO_BUILD="AArch64"    - Target architectures.
#   CMAKE_BUILD_TYPE=Debug             - The options are Release, Debug, or RelWithDebInfo.
#                                        You'll probably want to step through stuff
#                                        in a debugger, hence Debug.
#   LLVM_ENABLE_LLD=ON                 - Link with LLD rather than system linker. Seconds
#                                        instead of minutes, well worth it.
#   LLVM_PARALLEL_LINK_JOBS=1          - To prevent swapping do one link job per 16 GB of
#                                        RAM available. Linking will take FOREVER without
#                                        this.
#   MLIR_ENABLE_BINDINGS_PYTHON=ON     - We use the python bindings in jit-calc-experiments.
#   LLVM_INSTALL_GTEST=ON              - LLVM has a vendored version of gtest, we want to
#                                        use that version for writing our tests in
#                                        jit-calc-experiments.
#   CMAKE_EXPORT_COMPILE_COMMANDS=1    - This creates whatever clangd needs to do it's jump
#                                        to definition magic. If you're not using clnagd you
#                                        don't need this.
#   LLVM_USE_SPLIT_DWARF=ON            - This option stores debug information in a separate
#                                        file rather than embedding it directly in the
#                                        binary. Normally, a debugger reads debug data from
#                                        the binary itself, but with Split DWARF, it can
#                                        instead load that data from an external file. Debug
#                                        information is often large, keeping it out of the
#                                        main binary reduces the amount of data the linker
#                                        must handle, significantly shortening link times.
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
    -DLLVM_USE_SPLIT_DWARF=ON \
    -DCMAKE_EXPORT_COMPILE_COMMANDS=1

# From the llvm-project/build directory run `ninja` to build all targets. Go grab some
# coffee, it'll take a while.
(mlirdev) $ ninja
```

For more info, the `LLVM` docs have great [instructions on how to build LLVM](https://llvm.org/docs/CMake.html).

## Building `jit-calc-experiments`

The `jit-calc-experiments` needs to find `raylib`.
If you're using homebrew `raylib` it should be able to find it automagically; if not you'll need to do some _stuff_, and you get to figure that shit out yourself 😊.

The command below uses `$LLVM_BUILD_DIR` environment variable to hold the path of your LLVM build; something like: `/Users/ralph5/Dev/c++/llvm-project/build`.
Running the subshell with `set -u` first ensures that the command fails if `$LLVM_BUILD_DIR` isn't bound.
If `$LLVM_BUILD_DIR` isn't set or is set incorrectly, the build won't fail immediately; instead, CMake will pick up your homebrew LLVM and fail in some inscrutable way _later_.
```sh
(mlirdev) $ git clone git@github.com:AaronStGeorge/jit-calc-experiments.git
(mlirdev) $ mkdir jit-calc-experiments/build
(mlirdev) $ cd jit-calc-experiments/build

# The jit-calc-experiments CMake build piggybacks on LLVM’s, so the build system generation
# process is similar, but there are a few extra variables you need to define:
#
#   MLIR_DIR=$LLVM_BUILD_DIR/lib/cmake/mlir           - The `jit-calc-experiments` build
#                                                       needs to know where the LLVM build
#                                                       is.
#   LLVM_EXTERNAL_LIT=$LLVM_BUILD_DIR/bin/llvm-lit    - The `lit` tool (built during `LLVM`
#                                                       build) is needed for testing. Not
#                                                       sure why this needs its own flag
#                                                       rather than being inferred from
#                                                       DMLIR_DIR.
(mlirdev) $ (set -u; echo ${LLVM_BUILD_DIR} > /dev/null) && cmake -G Ninja .. \
    -DMLIR_DIR=${LLVM_BUILD_DIR}/lib/cmake/mlir \
    -DLLVM_EXTERNAL_LIT=${LLVM_BUILD_DIR}/bin/llvm-lit \
    -DCMAKE_BUILD_TYPE=Debug \
    -DLLVM_PARALLEL_LINK_JOBS=1 \
    -DCMAKE_C_COMPILER=$(which clang) \
    -DCMAKE_CXX_COMPILER=$(which clang++) \
    -DPython3_EXECUTABLE=$(which python) \
    -DLLVM_USE_LINKER=lld \
    -DMLIR_ENABLE_BINDINGS_PYTHON=ON \
    -DLLVM_USE_SPLIT_DWARF=ON \
    -DCMAKE_EXPORT_COMPILE_COMMANDS=1

# build all targets
(mlirdev) $ ninja
```
At this point you should be able to run the tests
```sh
(mlirdev) $ ninja check-jit-calc-experiments
```

## Running `wasm_to_mlir.py`

To run `wasm_to_mlir.py`—for example, to execute the `slow-roll-fibonacci-2` experiment—make sure the python virtual environment we set up earlier is active (`source ~/.venv/mlirdev/bin/activate`) and your `$PYTHONPATH` is set to include the MLIR python bindings.
You can source [`source_me_for_python_bindings.sh`](https://github.com/AaronStGeorge/jit-calc-experiments/blob/main/source_me_for_python_bindings.sh) to set everything up.
```sh
(mlirdev) $ source source_me_for_python_bindings.sh
```
This script modifies your `$PATH` to include `jit-calc-experiements` build directory and sets `$PYTHONPATH` for the MLIR python bindings.
Then you can run `slow-roll-fibonacci-2`.
```sh
(mlirdev) $ python python/wasm_to_mlir.py \
              experiments/slow-roll-fibonacci-2/fixtures/fib.wasm \
            | jit-calc-opt \
              --shim-function='function-to-shim=fib' \
              --convert-tower-runtime-to-llvm \
            | srf2-driver
```
