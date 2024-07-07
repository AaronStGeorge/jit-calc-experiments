add_mlir_dialect_library(TowerRuntimeTransforms
        CreateShimFunction.cpp

        ADDITIONAL_HEADER_DIRS
        ${PROJECT_SOURCE_DIR}/include/Dialect/TowerRuntime

        DEPENDS
        MLIRTowerRuntimeOpsIncGen
        MLIRTowerRuntimePassesIncGen

				MLIRTowerRuntimeDialect

        LINK_LIBS PUBLIC
        MLIRIR
        MLIRInferTypeOpInterface
        MLIRFuncDialect
        )
