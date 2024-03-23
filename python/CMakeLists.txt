include(AddMLIRPython)

# Specifies that all MLIR packages are co-located under the `mlir_standalone`
# top level package (the API has been embedded in a relocatable way).
# TODO: Add an upstream cmake param for this vs having a global here.
add_compile_definitions("MLIR_PYTHON_PACKAGE_PREFIX=mlir_standalone.")


################################################################################
# Sources
################################################################################

declare_mlir_python_sources(MLIRWasmPythonSources)

declare_mlir_dialect_python_bindings(
  ADD_TO_PARENT MLIRWasmPythonSources
  ROOT_DIR "${CMAKE_CURRENT_SOURCE_DIR}/mlir_standalone"
  TD_FILE dialects/StandaloneOps.td
  SOURCES
    dialects/standalone.py
  DIALECT_NAME standalone)

declare_mlir_python_extension(MLIRWasmPythonSources.Extension
  MODULE_NAME _standaloneDialects
  ADD_TO_PARENT MLIRWasmPythonSources
  SOURCES
    StandaloneExtension.cpp
  EMBED_CAPI_LINK_LIBS
    MLIRWasmCAPI
)

################################################################################
# Common CAPI
################################################################################

add_mlir_python_common_capi_library(MLIRWasmPythonCAPI
  INSTALL_COMPONENT MLIRWasmPythonModules
  INSTALL_DESTINATION python_packages/standalone/mlir_standalone/_mlir_libs
  OUTPUT_DIRECTORY "${MLIR_BINARY_DIR}/python_packages/standalone/mlir_standalone/_mlir_libs"
  RELATIVE_INSTALL_ROOT "../../../.."
  DECLARED_SOURCES
    MLIRWasmPythonSources
    # TODO: Remove this in favor of showing fine grained registration once
    # available.
    MLIRPythonExtension.RegisterEverything
    MLIRPythonSources.Core
)

################################################################################
# Instantiation of all Python modules
################################################################################

add_mlir_python_modules(MLIRWasmPythonModules
  ROOT_PREFIX "${MLIR_BINARY_DIR}/python_packages/standalone/mlir_standalone"
  INSTALL_PREFIX "python_packages/standalone/mlir_standalone"
  DECLARED_SOURCES
    MLIRWasmPythonSources
    # TODO: Remove this in favor of showing fine grained registration once
    # available.
    MLIRPythonExtension.RegisterEverything
    MLIRPythonSources
  COMMON_CAPI_LINK_LIBS
    MLIRWasmPythonCAPI
  )
