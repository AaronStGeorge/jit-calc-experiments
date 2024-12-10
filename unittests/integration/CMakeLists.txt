
function(fixture LIB_NAME FILE)
    add_player_code_library(${LIB_NAME} ${FILE} PREFIX ${LIB_NAME})
endfunction()


add_jit_calc_unittest(IntegrationTests IntegrationTests.cpp)
fixture(simple fixtures/simple.c)
fixture(add fixtures/add.c)
fixture(simple_cond fixtures/simple_cond.c)
fixture(fib fixtures/fib.c)
target_link_libraries(IntegrationTests PRIVATE
    simple
    add
    simple_cond
    fib
    TowerRuntimeStatic
)
