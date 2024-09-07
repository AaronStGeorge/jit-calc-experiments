add_mlir_public_c_api_library(TowerRuntimeCAPI
  TowerRuntime.cpp
  PARTIAL_SOURCES_INTENDED
  LINK_LIBS PUBLIC
  MLIRTowerRuntimeDialect
)

add_mlir_public_c_api_library(WasmCAPI
  Wasm.cpp
  PARTIAL_SOURCES_INTENDED
  LINK_LIBS PUBLIC
  Wasm
)