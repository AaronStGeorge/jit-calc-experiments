add_mlir_dialect_library(MLIRTowerRuntimeDialect
        StandaloneTypes.cpp
        StandaloneDialect.cpp
        StandaloneOps.cpp

        ADDITIONAL_HEADER_DIRS
        ${PROJECT_SOURCE_DIR}/include/Dialect/TowerRuntime

        DEPENDS
        MLIRTowerRuntimeOpsIncGen

        LINK_LIBS PUBLIC
        MLIRIR
        MLIRInferTypeOpInterface
        MLIRFuncDialect
        )
