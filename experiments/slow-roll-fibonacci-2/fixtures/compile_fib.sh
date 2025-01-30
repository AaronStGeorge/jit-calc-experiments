#! /bin/bash

INPUT="$1"
INPUT_STRIP_FILENAME_EXTENSION=${INPUT%.c}
STEM=${INPUT_STRIP_FILENAME_EXTENSION##*/}

clang "$INPUT" \
			-O1 \
      --target=wasm32 \
      --no-standard-libraries \
      -Wl,--allow-undefined \
      -Wl,--export-all \
      -Wl,--no-entry \
      -o "${STEM}.wasm"