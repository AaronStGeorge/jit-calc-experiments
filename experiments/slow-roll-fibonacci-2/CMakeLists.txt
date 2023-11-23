set(LLVM_LINK_COMPONENTS
  Core
  Support
  nativecodegen
  native
  )

add_llvm_executable(slow-roll-fibonacci-2 slow-roll-fibonacci-2.cpp)
llvm_update_compile_flags(slow-roll-fibonacci-2)
target_link_libraries(slow-roll-fibonacci-2 PRIVATE
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
  HomeRolledCoroutines
  LLVMLineEditor
  )

mlir_check_all_link_libraries(slow-roll-fibonacci-2)
