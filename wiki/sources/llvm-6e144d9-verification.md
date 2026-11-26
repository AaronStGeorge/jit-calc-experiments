# LLVM `6e144d9` verification record

- Date: 2026-11-26 UTC.
- Project baseline: `991f594e00b2bb8099973caa5a123383cf288c96`, with the compatibility changes in the pin update applied.
- LLVM: upstream [`6e144d9372f2169817e07f6a828438ec0880f34f`](https://github.com/llvm/llvm-project/commit/6e144d9372f2169817e07f6a828438ec0880f34f), unmodified on both machines.

| Check | macOS ARM64 (observed locally) | Linux x86_64 (reported by Fabric worker) |
|---|---|---|
| Vendored Release default build | Passed | Passed |
| `check-jit-calc-experiments` | 5/5 lit tests | 5/5 lit tests |
| `unittests/integration/IntegrationTests` | 4/4 passed | 4/4 passed |
| `count-to-n` | Exit 0, `return_val=9` | Exit 0, `return_val=9` |
| Python → MLIR → JIT Fibonacci | Exit 0, final row `4181 2584` | Matched baseline byte-for-byte, final row `4181 2584` |

- Local logs: `build/llvm-6e144d9-arm64/` (`configure.log`, `build.log`, `tests.log`, `integration.log`, `count-to-n.log`, and `fibonacci.log`). Apple Clang and libc++, ISO C++20.
- Linux logs: `~/Dev/build-jit-calc-llvm-6e144d9/` on `agents.prompt.pi.aaron.jim_x86-worker-1`; final report `call_405d24c94e2c`. Clang 21.1.8 and libstdc++ 15, ISO C++20; CMake configured with `-DCMAKE_CXX_SCAN_FOR_MODULES=OFF` after a dependency-scanner failure. MLIR's `Dominance.cpp` compiled without the old LLVM-only C++17 override.

Neither run covers native Linux ARM64 or GUI interaction.
