add_mlir_dialect_library(Wasm
        WasmDialect.cpp
        WasmOps.cpp

        ADDITIONAL_HEADER_DIRS
        ${PROJECT_SOURCE_DIR}/include/Wasm

        DEPENDS
        MLIRWasmOpsIncGen
        MLIRWasmPassesIncGen

        LINK_LIBS PUBLIC
        MLIRIR
        MLIRInferTypeOpInterface
        MLIRFuncDialect
        )
