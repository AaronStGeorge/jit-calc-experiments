# jit-calc-experiments tests

## Running the tests
```
cmake --build build --target check-jit-calc-experiments
```

## Where are the tests?
`jit-calc-experiments` uses LLVM's testing infrastructure.
It's a little odd frankly.
`lit` tests are in the `test` directory, and "unit" tests---as LLVM refers to C++ tests using [gtest](https://github.com/google/googletest)---are in the `unittests` directory.
LLVM cajoles all the tests "unit" and `lit` into the `check-jit-calc-experiments` CMake target by creating an `Unit` subdirectory in the `test` directory that just points at the tests in the `unittests` directory.

### what's a `lit` test?

[`lit`](https://llvm.org/docs/CommandGuide/lit.html) is an LLVM tool for gold file testing.
With `lit`, rather than specifying a “golden” output file associated a given input file, you embed the expected output in comments.
For example: `// RUN: some_command %s` and `// CHECK: <expected output>`.
Embedding commands in comments is a little strange, but it preserves the ability to run an input file outside of the testing framework, so it's nice too.

### Running an individual `lit` test

After building the test target above, run the vendored `lit` from the
repository root. Replace `build` in the test path if you configured elsewhere.

```sh
python third_party/llvm-project/llvm/utils/lit/lit.py build/test \
  --filter=tower_runtime_code.mlir
```

#### explanation

Because `lit` needs to know a lot of environment information that's only available at build time, a user will usually interact with `lit` tests through the build system.
For example, to test a compiler you're building `lit` needs to know where the compiler executable will be built.
`lit` gets this information by generating config files during build that it later loads when tests are run.
[There’s a great article on how this works](https://medium.com/@mshockwave/using-llvm-lit-out-of-tree-5cddada85a78), the below diagram is from that article.
`lit` is a python program by the way, that's why all the config files are python.
![out](https://github.com/AaronStGeorge/jit-calc-experiments/assets/2278731/bee0c910-d494-4c6a-a239-a2fedd30fc51)

Point `lit` at `build/test`, which contains the site configuration generated
by CMake. The filter selects the checked-in
[`tower_runtime_code.mlir`](Conversion/TowerRuntimeToLLVM/tower_runtime_code.mlir)
regression test.
