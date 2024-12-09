function(add_jit_calc_unittest test_dirname)
  add_unittest(JitCalcUnitTests ${test_dirname} ${ARGN})
endfunction()

add_subdirectory(integration)
