# jit-calc

We want a basic command line calculator... but wouldn't it be fun if it were
massively over-engineered? We could implement a simple interpreter to do the
parse-y and calculation-y bits of a calculator in maybe eighty lines of C++.
But eighty lines of C++ isn't cool, you know what's cool? Eight million lines
of C++. So let's use LLVM!

By using LLVM, we can create an entirely different type of calculator. To do
a calculation, a normal calculator executes instructions from its binary. If
we smuggle a compiler inside the calculator binary, it can *generate* the
instructions required to do a calculation. The calculator binary won't need
to physically contain the instructions necessary to do the calculation, it
can build them on the fly!

This is hot stuff! Using LLVM, we can cut the calculator's binary size down by
*dozens* of bytes, and we'll only need to pack a few gigs of compiler code in
there to do so! When the user requests some calculation be done, we can compile
their request in *milliseconds*. A computer requires a single digit number of
nanoseconds to execute a pre-compiled arithmetic instruction. Our users won't
have to slog through that! This is an excellent trade.

----

Next Steps:

- [x] Learn enough about ORC APIs to materialize a static MLIR function with CAPI wrappers to add two numbers as a callable function.
- [ ] Rather than relying on a static pre written pile of code, actually generate the MLIR based on command from user.
- [ ] Extend the calculator into a golang compiler.

## Building

This project builds with [`CMake`](https://cmake.org/) and depends on
[`LLVM`](https://github.com/llvm/llvm-project). An existing source build of
`LLVM` is required before building `jit-calc`.

The `LLVM` docs have great [instructions on how to build
LLVM](https://llvm.org/docs/CMake.html). `jit-calc` requires a few `LLVM`
utilities and projects to be enabled that aren't part of a regular build. An
example `LLVM` build with the required flags is shown here
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

Building `jit-calc` (once `LLVM` is built) requires flags to set the path to the
`LLVM` build, and the path to the `lit` tool (built during `LLVM` build). This setup
assumes that you have built `LLVM` (with MLIR enabled) in `$BUILD_DIR`. To build run
```sh
mkdir build && cd build
# generate build script
cmake -G Ninja .. \
    -DMLIR_DIR=$BUILD_DIR/lib/cmake/mlir \
    -DLLVM_EXTERNAL_LIT=$BUILD_DIR/bin/llvm-lit

# build all targets
ninja
```
