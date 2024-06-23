get_property(dialect_libs GLOBAL PROPERTY MLIR_DIALECT_LIBS)
set(LLVM_LINK_COMPONENTS
  Core
  Support
  )

add_llvm_executable(test-stuff-out main.cpp)
target_link_libraries(test-stuff-out PRIVATE
  ${dialect_libs}
  MLIRSupport
  MLIRIR
  )

mlir_check_all_link_libraries(test-stuff-out)