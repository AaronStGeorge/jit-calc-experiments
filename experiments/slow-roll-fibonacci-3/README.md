# `slow-roll-fibonacci-3`

This experiment uses the [MLIR WebAssembly importer](https://discourse.llvm.org/t/rfc-mlir-dialect-for-webassembly/86758/41) to translate a WebAssembly binary to the `wasmssa` dialect.

## Running

From the repository root, after building the project, try a straight-line
function. Replace `build` if you configured elsewhere, and use the same prebuilt
WASM-capable Clang you configured for the build:

```sh
tmp=$(mktemp -d)
printf 'int add(int a, int b) { return a + b; }\n' > "$tmp/add.c"
"$(brew --prefix llvm)/bin/clang" -O1 --target=wasm32 --no-standard-libraries \
  -Wl,--export-all -Wl,--no-entry "$tmp/add.c" -o "$tmp/add.wasm"
build/bin/srf3-translate "$tmp/add.wasm"
```

The output should include `wasmssa.func exported @add` and `wasmssa.add`.
This is an importer experiment, not a replacement for the Fibonacci pipeline:
the pinned LLVM importer rejects the Fibonacci fixture's `br` instruction
(opcode `12`). Use [slow-roll-fibonacci-2](../slow-roll-fibonacci-2) to run that demo.
