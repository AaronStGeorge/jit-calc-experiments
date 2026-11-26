# Build the standalone coroutine library and its native regression check.

add_library(HomeRolledCoroutines STATIC
  HomeRolledCoroutines.c
  home_rolled_coroutines_${JIT_CALC_ARCH_SUFFIX}.S)
target_include_directories(HomeRolledCoroutines PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/../../include")

add_executable(HomeRolledCoroutinesTest EXCLUDE_FROM_ALL coroutine_test.c)
target_link_libraries(HomeRolledCoroutinesTest PRIVATE HomeRolledCoroutines m)
add_custom_target(check-home-rolled-coroutines
  COMMAND HomeRolledCoroutinesTest
  DEPENDS HomeRolledCoroutinesTest
  VERBATIM)
