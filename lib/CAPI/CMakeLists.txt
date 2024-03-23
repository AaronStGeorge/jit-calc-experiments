add_mlir_public_c_api_library(MLIRWasmCAPI
  Dialects.cpp
  LINK_LIBS PUBLIC
  MLIRWasm
)
