set(LLVM_LINK_COMPONENTS
  Core
  OrcJIT
  Support
  nativecodegen
  native
  )
add_llvm_executable(srf2-test-thing srf2-test-thing.cpp)
llvm_update_compile_flags(srf2-test-thing)
target_link_libraries(srf2-test-thing PRIVATE
  MLIRAnalysis
  MLIRBuiltinToLLVMIRTranslation
  MLIRExecutionEngine
  MLIRIR
  MLIRJitRunner
  MLIRLLVMDialect
  MLIRLLVMToLLVMIRTranslation
  MLIRToLLVMIRTranslationRegistration
  MLIRParser
  MLIRTargetLLVMIRExport
  MLIRSupport
  Wasm
  )
mlir_check_all_link_libraries(srf2-test-thing)
# Don't ask me what the fuck this witchcraft
# https://github.com/llvm/llvm-project/issues/61712
export_executable_symbols(srf2-test-thing)