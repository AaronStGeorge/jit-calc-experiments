# LLVM build verification record (2026-11-21)

Project HEAD: `63bd5ce6df9cc17ae8cdafca79baa32beb304447`. The Mac worktree already had unrelated experiment edits and untracked files; they were left in place. The Linux worker reported a clean project worktree at the same HEAD.

## Apple Silicon Mac — observed locally

`llvm-config --version` and `mlir-opt --version` reported `22.0.0git`; `mlir-opt` reported a Debug build with assertions. `$(llvm-config --obj-root)/include/llvm/Support/VCSRevision.h` recorded:

```c
#define LLVM_REVISION "377148fd60df55e486dc72b26248006ae092725a"
#define LLVM_REPOSITORY "git@github.com:AaronStGeorge/llvm-project.git"
```

The LLVM checkout HEAD matched. The LLVM CMake cache had `LLVM_ENABLE_PROJECTS=mlir`, `LLVM_TARGETS_TO_BUILD=AArch64`, `LLVM_INSTALL_UTILS=ON`, `LLVM_INSTALL_GTEST=ON`, and `MLIR_ENABLE_BINDINGS_PYTHON=ON`.

The first project build (`/tmp/jit-calc-build.r826UA`) failed with libc++ errors starting with `<cmath> tried including <math.h> but didn't find libc++'s <math.h> header`. The compile command added `/Library/Developer/CommandLineTools/SDKs/MacOSX.sdk/usr/include` ahead of the active Xcode SDK's C++ headers. Reconfiguring with the active SDK and clearing cached package detection entries (see [build guide](../llvm-build.md)) found LibEdit and ZLIB under Xcode and removed the conflicting include flag from the generated Ninja file. `cmake --build build --parallel 4` finished all 108 scheduled steps (`/tmp/jit-calc-sdk.5MXcou`); a repeat reported no work. `cmake --build build --target check-jit-calc-experiments --parallel 4` completed with `Total Discovered Tests: 5` and `Passed: 5 (100.00%)` (`/tmp/jit-calc-check-macos.log`). Log paths are local to the Mac.

## Linux x86_64 — Fabric worker report

`agents.prompt.pi.aaron.jim_x86-worker-1`, Fabric calls `call_90b3346d3176` and `call_61fc701c900a`, reported:

- Fetched the same LLVM SHA from the fork; both Git HEAD and a generated LLVM revision header matched. No existing build at that SHA was found. Started a Release build targeting `X86`, with MLIR Python bindings, LLVM utilities, and gtest enabled, using three jobs and one link job. The last snapshot was 331/5,476 Ninja steps, with no failure logged.
- The project configured against that LLVM build tree. Early default and test target attempts were blocked by `mlir-tblgen` not yet being built. A direct `HomeRolledCoroutines` target build confirmed that the x86 assembler rejects its AArch64 source. The worker queued separate retries of the default and test targets after LLVM finishes. No Linux tests had run at the last snapshot.
- Worker directory: `~/Dev/build-jit-calc-linux-377148fd/`. `build.sh` records the commands; `llvm-build.log` tracks progress; `jit-early-configure-retry.log` records the successful early configuration; the queued retries will write `jit-build.log` and `jit-check.log` there.

## Vendoring follow-up — observed locally

- Inspected [IREE `.gitmodules`](https://github.com/iree-org/iree/blob/318812edbe0bc0d937c15609e8d789c211343668/.gitmodules) and [its LLVM CMake setup](https://github.com/iree-org/iree/blob/318812edbe0bc0d937c15609e8d789c211343668/build_tools/cmake/iree_llvm.cmake) at `318812edbe0bc0d937c15609e8d789c211343668`. IREE's gitlink points to `2cc5b84402a81e9d50f359f78ef2a3ee5cb552c7`; `iree_llvm_configure_bundled` adds the LLVM directory with `EXCLUDE_FROM_ALL` and sets variables normally exported by package configs. It is a macro, not a separate `ExternalProject` build.
- `git submodule status` confirmed jit-calc's LLVM at `377148fd60df55e486dc72b26248006ae092725a` and raylib at `dbc56a87da87d973a9c5baa4e7438a9d20121d28` (tag `6.0`).
- The fully bundled Release configuration generated clean Ninja graphs for both the default and check targets. In the generated WebAssembly commands, both `clang-22` and `wasm-ld` pointed into the vendored LLVM output directory. `cmake --build build/vendored --target try-ecs MLIRWasmOpsIncGen --parallel 4` completed, building raylib and `mlir-tblgen`, generating `WasmOps.h.inc`, and linking `try-ecs` (`/tmp/jit-calc-vendored-smoke.log`). The full build from that configuration was later stopped at 1,522/5,611 remaining steps (`/tmp/jit-calc-bundled-full.build`) when the user requested that Clang not be built. Its tests did not run. The earlier 5/5 result above used standalone LLVM; the vendored test result is recorded below.
- After removing Clang, LLD, and the WebAssembly backend from the LLVM sub-build, reconfigured `build/vendored` with `-DJIT_CALC_EXPERIMENTS_WASM_CLANG=/opt/homebrew/opt/llvm/bin/clang`. CMake reported `clang project is disabled`, `lld project is disabled`, and `Targeting AArch64`. Both default and test target dry runs scheduled no Clang or LLD source targets. The same prebuilt Clang compiled the `simple.c` fixture to a WebAssembly binary. The smoke targets were up to date.
- The incremental default build in `build/vendored` completed all 2,543 remaining Ninja steps (`/tmp/jit-calc-minimal.build`). `check-jit-calc-experiments` then discovered and passed all five tests (`/tmp/jit-calc-minimal.check`), and `/tmp/jit-calc-minimal.full-status` recorded `0`. No Clang, LLD, or LLVM WebAssembly backend source-build steps appeared in either log. This verifies the minimal vendored Release configuration on macOS ARM64, not a from-scratch build or a Linux pass.

## README smoke checks — observed locally

- With `.venv` active and `JIT_CALC_BUILD_DIR=build/vendored`, sourcing `source_me_for_python_bindings.sh` followed by the [README pipeline](../../README.md#running-wasm_to_mlirpy) printed all 20 game/user rows, ending in `4181 2584`. Rebuilding the fixture with `experiments/slow-roll-fibonacci-2/fixtures/compile_fib.sh` in a temporary directory gave the same result.
- The [individual lit command](../../test/README.md#running-an-individual-lit-test) passed `tower_runtime_code.mlir`; the old `test.mlir` filter matched nothing. Re-running the full check passed 5/5.
- `jit-calc` printed `5` for `2 + 3` and rejected subtraction; `count-to-n` printed `return_val=9` and exited successfully. Both raylib demos initialized their displays and OpenGL and were still running after two seconds; interaction was not tested.
- The [straight-line Wasm example](../../experiments/slow-roll-fibonacci-3/README.md#running) emitted an exported `wasmssa.func @add` and `wasmssa.add`. Running `srf3-translate` on the existing Fibonacci fixture instead exited 1 with `unknown instruction opcode: 12` (`br`).

## Standalone LLVM removal audit — observed locally

- After the sibling `llvm-project` checkout was removed, `git status` failed because the local LLVM submodule had a Git alternates file pointing into that checkout. Moved the dead alternates file aside and fetched the pinned commit from the configured origin with `--depth=1 --no-tags --no-recurse-submodules`. Both submodule and parent status then worked, without replacing the submodule worktree. A Git connectivity check found 307 broken local tag and remote-tracking refs left over from the borrowed store. Backed up `packed-refs`, removed only those broken refs, and verified `git fsck --connectivity-only` passed. The local branch and pinned checkout were preserved. See `/tmp/jit-calc-standalone-free.Uwz70o/` for the fetch log and refs backup.
- The old `build/CMakeCache.txt` and `build/mixed/CMakeCache.txt` still reference the removed path via `LLVM_DIR`, `MLIR_DIR`, and `LLVM_EXTERNAL_LIT`. The login environment inherited a stale PATH entry from `~/.bashrc`. These caches and the shell file were left untouched. No removed path was found in the generated Ninja/CMake files for `build/vendored` or in Mach-O load commands for 28 executables and Python extensions.
- With a clean PATH and no LLVM/MLIR or Python path overrides, `build/vendored` passed all five tests, the filtered test, the Python Fibonacci pipeline, CLI smoke tests, both Wasm compile helpers, the straight-line Wasm import, and GUI startup checks. See `/tmp/jit-calc-standalone-free.Uwz70o/smoke.log`.
- Configured a fresh `build/standalone-free` with both CMake package registries disabled. CMake selected AArch64 with Clang and LLD disabled in the vendored sub-build. The from-scratch default build completed all 4,527 Ninja steps (`build.log`), and `check-jit-calc-experiments` discovered and passed all five tests (`check.log`: 5/5, 100%). The combined exit status was `0` (`build.status`). The run used vendored LLVM/MLIR without a system LLVM fallback or a Clang/LLD source build. Logs: `/tmp/jit-calc-standalone-free.Uwz70o/`.
