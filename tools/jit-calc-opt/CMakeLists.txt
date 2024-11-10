get_property(dialect_libs GLOBAL PROPERTY MLIR_DIALECT_LIBS)
get_property(conversion_libs GLOBAL PROPERTY MLIR_CONVERSION_LIBS)
get_property(extension_libs GLOBAL PROPERTY MLIR_EXTENSION_LIBS)
set(LLVM_LINK_COMPONENTS
  Core
  Support
  AsmParser
  )

add_llvm_executable(jit-calc-opt jit-calc-opt.cpp)
llvm_update_compile_flags(jit-calc-opt)
target_link_libraries(jit-calc-opt PRIVATE
  ${dialect_libs}
  ${conversion_libs}
  ${extension_libs}
  Wasm
  WasmTransforms

  MLIRAffineAnalysis
  MLIRAnalysis
  MLIRCastInterfaces
  MLIRDialect
  MLIROptLib
  MLIRParser
  MLIRPass
  MLIRTransforms
  MLIRTransformUtils
  MLIRSupport
  MLIRIR

  # TODO: Remove when registerAllGPUToLLVMIRTranslations is no longer
  # registered directly in mlir-opt.cpp.
  MLIRToLLVMIRTranslationRegistration
  )

mlir_check_all_link_libraries(jit-calc-opt)
llvm_update_compile_flags(jit-calc-opt)
