# LLVM/MLIR revision and build notes

The project vendors **LLVM/MLIR `22.0.0git` at `377148fd60df55e486dc72b26248006ae092725a`** ([LLVM fork revision](https://github.com/AaronStGeorge/llvm-project/commit/377148fd60df55e486dc72b26248006ae092725a)) and **raylib 6.0 at `dbc56a87da87d973a9c5baa4e7438a9d20121d28`** as Git submodules. Use `git submodule status` to check the pins, and follow the [README](../README.md#building-jit-calc-experiments) to configure a fresh build.

The [dependency helpers](../cmake/JitCalcDependencies.cmake) follow [IREE's bundled LLVM pattern](https://github.com/iree-org/iree/blob/318812edbe0bc0d937c15609e8d789c211343668/build_tools/cmake/iree_llvm.cmake): add the vendored source with `EXCLUDE_FROM_ALL` and use real CMake targets, not `find_package`. Jit-calc uses functions to scope dependency options and exports only the LLVM/MLIR settings its own CMake helpers need. The LLVM sub-build enables only MLIR and the host backend. Player-code WebAssembly generation uses a separately supplied prebuilt `clang` with `wasm-ld`; neither Clang nor LLD is built from the submodule.

## Verified: minimal vendored build on macOS ARM64

The Release build in `build/vendored` completed, and `check-jit-calc-experiments` passed all five tests on 2026-11-21. The run used vendored LLVM/MLIR and raylib, with AArch64 as the only LLVM backend. It used `/opt/homebrew/opt/llvm/bin/clang` for WebAssembly fixtures; it did not build Clang or LLD from source. See the [verification record](sources/llvm-build-verification.md#vendoring-follow-up--observed-locally) for the configuration and logs.

The [README smoke checks](sources/llvm-build-verification.md#readme-smoke-checks--observed-locally) also verified the Python Fibonacci pipeline and individual lit test against `build/vendored`. The experimental `srf3-translate` imports straight-line Wasm but not the Fibonacci fixture.

After removing the standalone checkout, the vendored binaries and tests still ran. The local LLVM submodule did need its Git object store repaired because it had borrowed objects from that checkout. Use a fresh CMake build directory instead of reusing an old standalone-LLVM cache. See the [removal audit](sources/llvm-build-verification.md#standalone-llvm-removal-audit--observed-locally).

## Historical: macOS ARM64 mixed SDK headers with standalone LLVM

At project HEAD `63bd5ce6df9cc17ae8cdafca79baa32beb304447`, the default build and all five tests passed locally with the pinned LLVM. If libc++ complains that `<cmath>` cannot find libc++'s `<math.h>`, inspect the failing compile command. The preexisting CMake cache on this machine was injecting a Command Line Tools SDK include directory while `/usr/bin/clang++` was using the Xcode SDK. This reconfiguration of the existing build directory fixed that mismatch without rebuilding LLVM:

```sh
# Select llvm-config from the pinned build on PATH first.
llvm_build=$(llvm-config --obj-root)
cmake -S . -B build \
  -DCMAKE_OSX_SYSROOT="$(xcrun --show-sdk-path)" \
  -U '*LibEdit*' -U '*LIBEDIT*' -U '*LibXml2*' -U '*LIBXML*' -U '*ZLIB*' \
  -DLLVM_DIR="$llvm_build/lib/cmake/llvm" \
  -DMLIR_DIR="$llvm_build/lib/cmake/mlir" \
  -DLLVM_EXTERNAL_LIT="$llvm_build/bin/llvm-lit"
cmake --build build --parallel 4
cmake --build build --target check-jit-calc-experiments --parallel 4
```

This command is retained for reference when reproducing the pre-vendoring results at that commit. It is not the configure command for the current build.

## Linux x86_64: matching LLVM does not make the full build portable

The Linux worker reported successful project configuration, but confirmed the default build has an architecture blocker: [HomeRolledCoroutines assembly](../lib/HomeRolledCoroutines/home_rolled_coroutines.s) uses AArch64 instructions. The separate [TowerRuntime assembly](../lib/TowerRuntime/tower_runtime.s) may also block the integration tests; the worker had not reached it. The worker queued both build and test retries after LLVM finishes. No Linux build or test pass has been established.

Source: [LLVM build verification record](sources/llvm-build-verification.md).
