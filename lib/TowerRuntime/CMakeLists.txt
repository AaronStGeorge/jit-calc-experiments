add_library(TowerRuntimeObjects OBJECT ${JIT_CALC_CONTEXT_ASM} TowerRuntime.cpp)
set_target_properties(TowerRuntimeObjects PROPERTIES POSITION_INDEPENDENT_CODE ON)
target_compile_definitions(TowerRuntimeObjects PRIVATE JIT_CALC_TOWER_RUNTIME)

add_library(TowerRuntimeShared SHARED $<TARGET_OBJECTS:TowerRuntimeObjects>)
add_library(TowerRuntimeStatic STATIC $<TARGET_OBJECTS:TowerRuntimeObjects>)
