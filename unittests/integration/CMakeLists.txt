add_jit_calc_unittest(IntegrationTests IntegrationTests.cpp)
add_player_code_library(simple fixtures/simple.c PREFIX simple)
add_player_code_library(add fixtures/add.c PREFIX add)
target_link_libraries(IntegrationTests PRIVATE
    simple
    add
    TowerRuntimeStatic
)
