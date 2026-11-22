configure_lit_site_cfg(
        ${CMAKE_CURRENT_SOURCE_DIR}/lit.site.cfg.py.in
        ${CMAKE_CURRENT_BINARY_DIR}/lit.site.cfg.py
        MAIN_CONFIG
        ${CMAKE_CURRENT_SOURCE_DIR}/lit.cfg.py
)
# The LLVM testing machinery is really weird. Instead of just using CTest they
# run unit tests through their gold-file testing lit program. I guess the goal
# is to keep the build infrastructure consistent.
configure_lit_site_cfg(
        ${CMAKE_CURRENT_SOURCE_DIR}/Unit/lit.site.cfg.py.in
        ${CMAKE_CURRENT_BINARY_DIR}/Unit/lit.site.cfg.py
        MAIN_CONFIG
        ${CMAKE_CURRENT_SOURCE_DIR}/Unit/lit.cfg.py
        )

set(JIT_CALC_EXPERIMENTS_TEST_DEPENDS
        FileCheck count not jit-calc-opt mlir-opt mlir-runner
        )

list(APPEND JIT_CALC_EXPERIMENTS_TEST_DEPENDS JitCalcUnitTests check-home-rolled-coroutines)

add_lit_testsuite(check-jit-calc-experiments "Running the jit-calc-experiments regression tests"
        ${CMAKE_CURRENT_BINARY_DIR}
        DEPENDS ${JIT_CALC_EXPERIMENTS_TEST_DEPENDS}
        )

add_lit_testsuites(JIT_CALC_EXPERIMENTS ${CMAKE_CURRENT_SOURCE_DIR} DEPENDS ${JIT_CALC_EXPERIMENTS_TEST_DEPENDS})
