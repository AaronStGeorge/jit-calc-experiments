set(LLVM_OPTIONAL_SOURCES
  null.cpp
)

get_property(dialect_libs GLOBAL PROPERTY MLIR_DIALECT_LIBS)
get_property(conversion_libs GLOBAL PROPERTY MLIR_CONVERSION_LIBS)
set(LLVM_LINK_COMPONENTS
  Core
  Support
  AsmParser
  )

set(test_libs
  MLIRAffineTransformsTestPasses
  MLIRShapeTestPasses
  MLIRSPIRVTestPasses
  MLIRTestAnalysis
  MLIRTestDialect
  MLIRTestDynDialect
  MLIRTestIR
  MLIRTestPass
  MLIRTestReducer
  )
set(test_libs
  ${test_libs}
  MLIRTestTransformDialect
  MLIRTestTransforms)

set(LIBS
  ${conversion_libs}
  ${dialect_libs}
  ${extension_libs}
  ${test_libs}

  MLIRAffineAnalysis
  MLIRAnalysis
  MLIRDialect
  MLIRFuncAllExtensions
  MLIRLspServerLib
  MLIRParser
  MLIRPass
  MLIRTransforms
  MLIRTransformUtils
  MLIRSupport
  MLIRIR
  Wasm
  )

add_llvm_executable(jit-calc-lsp-server
        jit-calc-lsp-server.cpp

  DEPENDS
  ${LIBS}
  )
target_link_libraries(jit-calc-lsp-server PRIVATE ${LIBS})
llvm_update_compile_flags(jit-calc-lsp-server)

mlir_check_all_link_libraries(jit-calc-lsp-server)
