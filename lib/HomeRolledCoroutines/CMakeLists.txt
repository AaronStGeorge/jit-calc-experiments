add_library(HomeRolledCoroutines STATIC home_rolled_coroutines_${JIT_CALC_ARCH_SUFFIX}.S)
# Use CMake's C archive rule to recreate the archive when its members change.
# The ASM archive rule can retain obsolete objects on macOS (e.g. after rebasing).
set_target_properties(HomeRolledCoroutines PROPERTIES LINKER_LANGUAGE C)
target_include_directories(HomeRolledCoroutines PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/../../include")

add_executable(HomeRolledCoroutinesTest EXCLUDE_FROM_ALL coroutine_test.c)
target_link_libraries(HomeRolledCoroutinesTest PRIVATE HomeRolledCoroutines m)
add_custom_target(check-home-rolled-coroutines
  COMMAND HomeRolledCoroutinesTest
  DEPENDS HomeRolledCoroutinesTest
  VERBATIM)
