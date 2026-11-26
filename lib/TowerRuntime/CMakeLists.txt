# Build the same target-specific runtime objects into static and shared libraries.

add_library(TowerRuntimeObjects OBJECT tower_runtime_${JIT_CALC_ARCH_SUFFIX}.S TowerRuntime.cpp)
set_target_properties(TowerRuntimeObjects PROPERTIES POSITION_INDEPENDENT_CODE ON)

add_library(TowerRuntimeShared SHARED $<TARGET_OBJECTS:TowerRuntimeObjects>)
add_library(TowerRuntimeStatic STATIC $<TARGET_OBJECTS:TowerRuntimeObjects>)
