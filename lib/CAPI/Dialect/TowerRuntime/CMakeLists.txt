add_mlir_public_c_api_library(TowerRuntimeCAPI
  Dialects.cpp
  LINK_LIBS PUBLIC
  MLIRTowerRuntimeDialect
)
