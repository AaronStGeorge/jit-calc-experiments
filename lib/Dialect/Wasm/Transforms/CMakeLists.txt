add_mlir_dialect_library(WasmTransforms
        WasmPasses.cpp

        ADDITIONAL_HEADER_DIRS
        ${PROJECT_SOURCE_DIR}/include/Wasm

        DEPENDS
        MLIRWasmOpsIncGen
        MLIRWasmPassesIncGen

				Wasm

        LINK_LIBS PUBLIC
        MLIRIR
        MLIRInferTypeOpInterface
        MLIRFuncDialect
        )
