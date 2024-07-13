add_mlir_conversion_library(TowerRuntimeToLLVM
  TowerRuntimeToLLVM.cpp

  ADDITIONAL_HEADER_DIRS
  ${PROJECT_SOURCE_DIR}/include/Conversion/TowerRuntimeToLLVM

  DEPENDS
  JitCalcExperimentsConversionPassIncGen

  LINK_COMPONENTS
  Core

  LINK_LIBS PUBLIC
  MLIRTowerRuntimeDialect
  MLIRLLVMCommonConversion
  MLIRLLVMDialect
  TowerRuntimeUtils
  )
