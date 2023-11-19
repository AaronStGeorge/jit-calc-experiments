configure_lit_site_cfg(
        ${CMAKE_CURRENT_SOURCE_DIR}/lit.site.cfg.py.in
        ${CMAKE_CURRENT_BINARY_DIR}/lit.site.cfg.py
        MAIN_CONFIG
        ${CMAKE_CURRENT_SOURCE_DIR}/lit.cfg.py
)

set(JIT_CALC_EXPERIMENTS_TEST_DEPENDS
        FileCheck count not
        )

add_lit_testsuite(check-jit-calc-experiments "Running the jit-calc-experiments regression tests"
        ${CMAKE_CURRENT_BINARY_DIR}
        DEPENDS ${JIT_CALC_EXPERIMENTS_TEST_DEPENDS}
        )
set_target_properties(check-jit-calc-experiments PROPERTIES FOLDER "Tests")

add_lit_testsuites(JIT_CALC_EXPERIMENTS ${CMAKE_CURRENT_SOURCE_DIR} DEPENDS ${JIT_CALC_EXPERIMENTS_TEST_DEPENDS})
