get_property(dialect_libs GLOBAL PROPERTY MLIR_DIALECT_LIBS)
get_property(translation_libs GLOBAL PROPERTY MLIR_TRANSLATION_LIBS)

set(LLVM_LINK_COMPONENTS
  Core
  Support
  )

add_llvm_executable(srf3-translate
  srf3-translate.cpp
  )

llvm_update_compile_flags(srf3-translate)

target_link_libraries(srf3-translate PRIVATE
  ${dialect_libs}
  ${translation_libs}
  )

mlir_check_all_link_libraries(srf3-translate)
