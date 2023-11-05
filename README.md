# jit-calc-experiments

🧪 Experiments around runtime systems and jit compilers. Stuff that could maybe,
one day, be squished into the shape of a runtime engine for a game where NPC
robots execute player code.

## Building

Look, "experiments" is in the name, there's going to be some weird shit.
`jit-calc-experiments` requires:

- A machine with the [Netwide Assembler](https://www.nasm.us/) installed.
- An x86_64 machine (currently).
- An LLVM source build.

### LLVM Source build

The `LLVM` docs have great [instructions on how to build
LLVM](https://llvm.org/docs/CMake.html). `jit-calc-experiments` needs a few
`LLVM` utilities and projects to be enabled that aren't part of a regular build.
An example `LLVM` build with the required flags is shown here.
`jit-calc-experiments` isn't fussy about _where_ you build LLVM.
```sh
# shallow clone of llvm to save time
git clone --depth 1 https://github.com/llvm/llvm-project.git

mkdir llvm-project/build && cd llvm-project/build

# To compile LLVM you first have to generate the build script with CMake. Taking
# a look at the flags:
# LLVM_ENABLE_PROJECTS="mlir"        - MLIR project must be enabled.
# LLVM_INSTALL_UTILS=ON              - Install utilities, like lit, needed for
#                                      testing jit-calc.
cmake -G Ninja ../llvm \
   -DLLVM_ENABLE_PROJECTS="mlir" \
   -DLLVM_INSTALL_UTILS=ON

# build all targets
ninja
```
### Building `jit-calc-experiments`

Building `jit-calc-experiments` requires flags to set the path to the `LLVM`
build, and the path to the `lit` tool (built during `LLVM` build). This setup
assumes that you have built `LLVM` (with MLIR enabled) in `$BUILD_DIR`. To build
run
```sh
mkdir build && cd build
# generate build script
cmake -G Ninja .. \
    -DMLIR_DIR=$BUILD_DIR/lib/cmake/mlir \
    -DLLVM_EXTERNAL_LIT=$BUILD_DIR/bin/llvm-lit

# build all targets
ninja
```