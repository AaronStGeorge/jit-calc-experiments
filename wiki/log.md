# Wiki log

## [2026-11-21] setup | Initialize wiki

Started the wiki on `vibesperimentation` with an index, a log, and maintenance instructions in `AGENTS.md`.

## [2026-11-21] ingest | Fabric agent discovery

Added the [Fabric discovery guide](fabric-agent-discovery.md) and its [test record](sources/fabric-discovery-test.md). The retry worked; the cause of the earlier discovery failure remains unknown.

## [2026-11-21] verify | List all visible Fabric agents

Verified that `discover_agents` with `{}` on the installed MCP server and `npx @synadia-ai/agent-fabric agents` both returned the same three Pi agents. Updated the guide to explain unfiltered discovery first, then how to target one address.

## [2026-11-21] lint | Add a context-aware discovery fallback

Agent 1 confirmed that the guide worked for messaging a single agent. Its tool interface didn't submit `{}`, and the CLI failed because the required environment variables weren't set. Added a CLI recipe for an active Fabric-managed NATS context and instructions for obtaining authorized caller credentials when no context is available.

Agent 3 independently ran the new fallback as written, listed all three agents, and got a reply from agent 1. Its tool interface also couldn't submit `{}`. Clarified that the CLI prints the server URL and that an exact lookup after CLI listing supplies the target's signing requirement and instance count.

## [2026-11-21] verify | Pin LLVM and build on Apple Silicon

Added the [LLVM build guide](llvm-build.md) and its verification record. Verified the exact LLVM revision, fixed a mixed-SDK CMake cache, and passed the default build and all five tests on macOS ARM64. The Linux x86 worker started an LLVM build at the same revision and reproduced the existing AArch64 assembly blocker; Linux build and test results are pending.

## [2026-11-21] ingest | Vendor LLVM and raylib

Updated the [LLVM build guide](llvm-build.md) with the pinned submodules and IREE-style in-tree CMake setup. Recorded the upstream IREE snapshot, successful Ninja graph generation and vendored smoke build, and pending full-build verification in the verification record.

## [2026-11-21] verify | Keep the LLVM sub-build minimal

Removed Clang, LLD, and the WebAssembly backend from the vendored LLVM build. Verified that the Ninja plan excludes Clang and LLD and that the separately supplied Clang produces a WebAssembly fixture. Restarted the macOS build with MLIR and the host backend only.

## [2026-11-21] verify | Pass the minimal vendored build on Apple Silicon

The incremental Release build completed in `build/vendored`, and `check-jit-calc-experiments` passed all five tests without building Clang or LLD from source. Recorded the result in the [build guide](llvm-build.md) and the supporting logs in the verification record.

## [2026-11-21] verify | Run README smoke checks

Verified the Python Fibonacci pipeline and individual lit command against `build/vendored`, fixed their setup instructions, and documented the LLVM Wasm importer's Fibonacci fixture limitation. Recorded the observed results, including the CLI and GUI startup checks.

## [2026-11-21] verify | Audit removal of standalone LLVM

Repaired the local LLVM submodule's dangling Git alternate, reran the vendored smoke checks without the removed checkout, and started a clean build. Recorded the remaining legacy cache and shell references in the verification record.

## [2026-11-21] verify | Complete the standalone LLVM removal check

The from-scratch build in `build/standalone-free` completed all 4,527 Ninja steps, and `check-jit-calc-experiments` passed all five tests (exit status `0`). Recorded the result and log paths in the verification record.

## [2026-11-25] lint | Remove retired documentation links

Removed the root README and LLVM build verification record, and cleaned up links to them.

## [2026-11-25] verify | Separate coroutine backends and rebase PR #2

Recorded the [architecture options and cross-platform results](llvm-build.md#coroutine-builds-on-arm64-and-x86_64) for PR #2. Rebased onto `vibesperimentation` without restoring the retired documentation, and recorded the default base branch in `AGENTS.md`.

## [2026-11-25] update | Select coroutine assembly automatically

Removed the redundant `JIT_CALC_ARCH` config option. CMake selects the assembly from its target architecture.

## [2026-11-25] update | Make x86_64 coroutine backends Linux-only

Removed the Mach-O symbol handling from both x86_64 backends. CMake rejects x86_64 macOS targets; macOS builds use ARM64.

## [2026-11-25] update | Build TowerRuntime as one static library

Renamed the static target to `TowerRuntime` and removed the unused shared target and its lit substitution. The demos and integration tests link the static library.
