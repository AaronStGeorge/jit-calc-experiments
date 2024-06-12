get_property(dialect_libs GLOBAL PROPERTY MLIR_DIALECT_LIBS)
get_property(conversion_libs GLOBAL PROPERTY MLIR_CONVERSION_LIBS)

add_llvm_executable(srf2-opt srf2-opt.cpp)
llvm_update_compile_flags(srf2-opt)
target_link_libraries(srf2-opt PRIVATE
  ${dialect_libs}
  ${conversion_libs}
  MLIROptLib
  Wasm
  WasmTransforms
  )

mlir_check_all_link_libraries(srf2-opt)