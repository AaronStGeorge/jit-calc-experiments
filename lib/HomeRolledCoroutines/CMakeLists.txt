# Build the standalone coroutine library for the target architecture.

add_library(HomeRolledCoroutines STATIC
  HomeRolledCoroutines.c
  home_rolled_coroutines_${JIT_CALC_ARCH_SUFFIX}.S)
target_include_directories(HomeRolledCoroutines PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/../../include")
