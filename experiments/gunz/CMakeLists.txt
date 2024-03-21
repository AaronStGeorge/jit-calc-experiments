set(LLVM_LINK_COMPONENTS
  Core
  OrcJIT
  Support
  nativecodegen
  native
  )

add_llvm_executable(gunz asteroids.c)

llvm_update_compile_flags(gunz)
target_link_libraries(gunz PRIVATE
	# mlir stuff
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
  LLVMLineEditor
	# raylib stuff
	raylib
	# this is only needed on apple platforms
 	"-framework IOKit"
 	"-framework Cocoa"
 	"-framework OpenGL"
	)
mlir_check_all_link_libraries(gunz)
# Don't ask me what the fuck this witchcraft
# https://github.com/llvm/llvm-project/issues/61712
export_executable_symbols(gunz)