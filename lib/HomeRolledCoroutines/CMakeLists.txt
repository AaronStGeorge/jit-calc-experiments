add_library(HomeRolledCoroutines ${JIT_CALC_CONTEXT_ASM})
target_include_directories(HomeRolledCoroutines PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/../../include")

add_executable(HomeRolledCoroutinesTest EXCLUDE_FROM_ALL coroutine_test.c)
target_link_libraries(HomeRolledCoroutinesTest PRIVATE HomeRolledCoroutines m)
add_custom_target(check-home-rolled-coroutines
  COMMAND HomeRolledCoroutinesTest
  DEPENDS HomeRolledCoroutinesTest
  VERBATIM)
