# LLVM/MLIR revision and build notes

The project vendors **LLVM/MLIR `22.0.0git` at `377148fd60df55e486dc72b26248006ae092725a`** ([LLVM fork revision](https://github.com/AaronStGeorge/llvm-project/commit/377148fd60df55e486dc72b26248006ae092725a)) and **raylib 6.0 at `dbc56a87da87d973a9c5baa4e7438a9d20121d28`** as Git submodules. Use `git submodule status` to check the pins.

The [dependency helpers](../cmake/JitCalcDependencies.cmake) follow [IREE's bundled LLVM pattern](https://github.com/iree-org/iree/blob/318812edbe0bc0d937c15609e8d789c211343668/build_tools/cmake/iree_llvm.cmake): add the vendored source with `EXCLUDE_FROM_ALL` and use real CMake targets, not `find_package`. Jit-calc uses functions to scope dependency options and exports only the LLVM/MLIR settings its own CMake helpers need. The LLVM sub-build enables only MLIR and the host backend. Player-code WebAssembly generation uses a separately supplied prebuilt `clang` with `wasm-ld`; neither Clang nor LLD is built from the submodule.

## Verified: minimal vendored build on macOS ARM64

The Release build in `build/vendored` completed, and `check-jit-calc-experiments` passed all five tests on 2026-11-21. The run used vendored LLVM/MLIR and raylib, with AArch64 as the only LLVM backend. It used `/opt/homebrew/opt/llvm/bin/clang` for WebAssembly fixtures; it did not build Clang or LLD from source.

Local smoke checks also verified the Python Fibonacci pipeline and individual lit test against `build/vendored`. The experimental `srf3-translate` imports straight-line Wasm but not the Fibonacci fixture.

After removing the standalone checkout, the vendored binaries and tests still ran. The local LLVM submodule did need its Git object store repaired because it had borrowed objects from that checkout. Use a fresh CMake build directory instead of reusing an old standalone-LLVM cache.

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

## Coroutine builds on ARM64 and x86_64

[HomeRolledCoroutines](../lib/HomeRolledCoroutines/) and [TowerRuntime](../lib/TowerRuntime/) each have separate ARM64 and x86_64 assembly files. The [CMake architecture setting](../cmake/JitCalcArch.cmake) defaults to `JIT_CALC_ARCH=AUTO`. To pin it, set `-DJIT_CALC_ARCH=ARM64` or `-DJIT_CALC_ARCH=X86_64` when configuring. It must match the compiler target; it does not set up cross-compilation. The x86_64 backend does not support CET shadow stacks.

[PR #2](https://github.com/AaronStGeorge/jit-calc-experiments/pull/2) removed the ARM64-only build blocker. Before rebasing, the separate backends passed the vendored default build, `check-jit-calc-experiments` (5/5 lit tests and the standalone coroutine state check), and `count-to-n` (`return_val=9`) on both Apple ARM64 (observed locally) and Linux x86_64 (reported by the Fabric worker). The Linux worker built both GUI demos but could not verify startup on its headless machine.

The vendored LLVM/MLIR sub-build uses C++17 to avoid a libstdc++ 15 build error at the pinned revision; jit-calc itself still uses C++20.
