include(AddMLIRPython)

# Specifies that all MLIR packages are co-located under the `jit_calc`
# top level package (the API has been embedded in a relocatable way).
# TODO: Add an upstream cmake param for this vs having a global here.
add_compile_definitions("MLIR_PYTHON_PACKAGE_PREFIX=jitcalc.")


################################################################################
# Sources
################################################################################

declare_mlir_python_sources(JitCalcPythonSources)

#### WASM

declare_mlir_dialect_python_bindings(
  ADD_TO_PARENT JitCalcPythonSources
  ROOT_DIR "${CMAKE_CURRENT_SOURCE_DIR}/jit_calc"
  TD_FILE dialects/WasmOps.td
  SOURCES
    dialects/wasm.py
  DIALECT_NAME wasm)

declare_mlir_python_extension(JitCalcPythonSources.WasmExtension
  MODULE_NAME _wasmDialect
  ADD_TO_PARENT JitCalcPythonSources
  SOURCES
    WasmExtension.cpp
  EMBED_CAPI_LINK_LIBS
    WasmCAPI
)

#### TOWER_RUNTIME

declare_mlir_dialect_python_bindings(
  ADD_TO_PARENT JitCalcPythonSources
  ROOT_DIR "${CMAKE_CURRENT_SOURCE_DIR}/jit_calc"
  TD_FILE dialects/TowerRuntimeOps.td
  SOURCES
    dialects/tower_runtime.py
  DIALECT_NAME tower_runtime)

declare_mlir_python_extension(JitCalcPythonSources.TowerRuntimeExtension
  MODULE_NAME _towerRuntimeDialect
  ADD_TO_PARENT JitCalcPythonSources
  SOURCES
    TowerRuntimeExtension.cpp
  EMBED_CAPI_LINK_LIBS
    TowerRuntimeCAPI
)

################################################################################
# Common CAPI
################################################################################

add_mlir_python_common_capi_library(JitCalcPythonCAPI
  INSTALL_COMPONENT JitCalcPythonModules
  INSTALL_DESTINATION python_packages/jit_calc/jitcalc/_mlir_libs
  OUTPUT_DIRECTORY "${MLIR_BINARY_DIR}/python_packages/jit_calc/jitcalc/_mlir_libs"
  RELATIVE_INSTALL_ROOT "../../../.."
  DECLARED_SOURCES
    JitCalcPythonSources
    # TODO: Remove this in favor of showing fine grained registration once
    # available.
    MLIRPythonExtension.RegisterEverything
    MLIRPythonSources.Core
)

################################################################################
# Instantiation of all Python modules
################################################################################

add_mlir_python_modules(JitCalePythonModules
  ROOT_PREFIX "${MLIR_BINARY_DIR}/python_packages/jit_calc/jitcalc"
  INSTALL_PREFIX "python_packages/jit_calc/jitcalc"
  DECLARED_SOURCES
    JitCalcPythonSources
    # TODO: Remove this in favor of showing fine grained registration once
    # available.
    MLIRPythonExtension.RegisterEverything
    MLIRPythonSources
  COMMON_CAPI_LINK_LIBS
    JitCalcPythonCAPI
  )
