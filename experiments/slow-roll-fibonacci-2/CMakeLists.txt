set(LLVM_LINK_COMPONENTS
  Core
  OrcJIT
  Support
  nativecodegen
  native
  )
add_llvm_executable(srf2-driver srf2-driver.cpp)
llvm_update_compile_flags(srf2-driver)
target_link_libraries(srf2-driver PRIVATE
  MLIRJitRunner
  MLIRTargetLLVMIRExport
  MLIRRegisterAllDialects
  MLIRRegisterAllExtensions
  MLIRRegisterAllPasses
  MLIRSupport
  Wasm
  WasmTransforms
  TowerRuntimeStatic
  )
mlir_check_all_link_libraries(srf2-driver)
# Don't ask me what the fuck this witchcraft
# https://github.com/llvm/llvm-project/issues/61712
export_executable_symbols(srf2-driver)