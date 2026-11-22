# jit-calc-experiments

🧪 Experiments around runtime systems and jit compilers. Stuff that could maybe,
one day, be squished into the shape of a runtime engine for a game where NPC
robots execute player code.

## Prerequisites

- An Apple Silicon Mac with the Xcode or Command Line Tools SDK. A native x86
  build is blocked by the project's [AArch64 assembly](lib/HomeRolledCoroutines/home_rolled_coroutines.s).
- CMake 3.25+, Ninja, and Python 3 with a virtual environment. On macOS you can
  install these with `brew install cmake ninja python`.
- A prebuilt Clang that supports `wasm32` and can find `wasm-ld`. On macOS,
  `brew install llvm lld` provides these. They are used only to compile and
  link player code to WebAssembly, not as the project's LLVM/MLIR libraries.

## Building `jit-calc-experiments`

From the repository root, initialize the pinned LLVM and raylib submodules
(skip this step if you cloned with `--recurse-submodules`):

```sh
git submodule update --init --recursive
python3 -m venv .venv
source .venv/bin/activate
python -m pip install -r third_party/llvm-project/mlir/python/requirements.txt
```

Configure and build using the host compiler. The first build compiles the
needed vendored LLVM/MLIR targets for the host architecture and builds raylib.
It does not build Clang or LLD from source. The example points CMake at
Homebrew's prebuilt Clang for WebAssembly; substitute your own path if needed.

```sh
cmake -S . -B build -G Ninja \
  -DCMAKE_BUILD_TYPE=Release \
  -DCMAKE_OSX_SYSROOT="$(xcrun --show-sdk-path)" \
  -DPython3_EXECUTABLE="$(command -v python)" \
  -DJIT_CALC_EXPERIMENTS_WASM_CLANG="$(brew --prefix llvm)/bin/clang" \
  -DLLVM_PARALLEL_LINK_JOBS=1
cmake --build build --parallel 4
cmake --build build --target check-jit-calc-experiments --parallel 4
```

Use a fresh build directory when switching from an older configuration that
used a standalone LLVM. Use `Debug` if you need full debug info, but expect
larger binaries. See the [build wiki](wiki/llvm-build.md) for dependency pins and
background.

## Running `wasm_to_mlir.py`

From the repository root, with the same Python environment active, source
[`source_me_for_python_bindings.sh`](source_me_for_python_bindings.sh) to add
`build/bin` to `PATH` and point `PYTHONPATH` at the built `jitcalc` package.
If you used a different CMake build directory, set `JIT_CALC_BUILD_DIR` first
(for example, `export JIT_CALC_BUILD_DIR=build/vendored`).

```sh
source source_me_for_python_bindings.sh
python python/wasm_to_mlir.py \
  experiments/slow-roll-fibonacci-2/fixtures/fib.wasm \
  | jit-calc-opt \
    --shim-function='function-to-shim=fib' \
    --convert-tower-runtime-to-llvm \
  | srf2-driver
```
