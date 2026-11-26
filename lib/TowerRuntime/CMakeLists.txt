# Build the static TowerRuntime library for the target architecture.

add_library(TowerRuntime STATIC
  TowerRuntime.cpp
  tower_runtime_${JIT_CALC_ARCH_SUFFIX}.S)
