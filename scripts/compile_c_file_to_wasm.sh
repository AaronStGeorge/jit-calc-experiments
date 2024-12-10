#!/bin/bash
set -euo pipefail

C_PROGRAM="$1"

BASENAME="${C_PROGRAM%.c}"

WASM_OUTPUT="$BASENAME.wasm"

clang -O1 --target=wasm32 --no-standard-libraries -Wl,--allow-undefined -Wl,--export-all -Wl,--no-entry -o "$WASM_OUTPUT" "$C_PROGRAM"
