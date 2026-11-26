# Configure the jit-calc experiments, vendored dependencies, and test targets.

cmake_minimum_required(VERSION 3.25)
project(jit-calc-experiments LANGUAGES CXX C ASM)

include(cmake/JitCalcArch.cmake)

set(CMAKE_BUILD_WITH_INSTALL_NAME_DIR ON)

set(CMAKE_CXX_STANDARD 20 CACHE STRING "C++ standard to conform to")
set(CMAKE_C_STANDARD 11 CACHE STRING "C standard to conform to")

# MLIR uses FindPython3 and FindPython; keep nanobind on the same interpreter.
if(Python3_EXECUTABLE AND NOT Python_EXECUTABLE)
  set(Python_EXECUTABLE "${Python3_EXECUTABLE}")
endif()

set(JIT_CALC_EXPERIMENTS_WASM_CLANG "" CACHE FILEPATH
    "Path to a prebuilt clang with WebAssembly support")
if(NOT EXISTS "${JIT_CALC_EXPERIMENTS_WASM_CLANG}")
  message(FATAL_ERROR "Set JIT_CALC_EXPERIMENTS_WASM_CLANG to a prebuilt clang with WebAssembly and wasm-ld support.")
endif()
add_executable(WASM_CLANG IMPORTED)
set_target_properties(WASM_CLANG PROPERTIES IMPORTED_LOCATION "${JIT_CALC_EXPERIMENTS_WASM_CLANG}")

include(cmake/JitCalcDependencies.cmake)

jit_calc_add_bundled_llvm()
jit_calc_add_bundled_raylib()

set(LLVM_RUNTIME_OUTPUT_INTDIR ${CMAKE_BINARY_DIR}/bin)
set(LLVM_LIBRARY_OUTPUT_INTDIR ${CMAKE_BINARY_DIR}/lib)
set(MLIR_BINARY_DIR ${CMAKE_BINARY_DIR})

list(APPEND CMAKE_MODULE_PATH "${MLIR_CMAKE_DIR}")
list(APPEND CMAKE_MODULE_PATH "${LLVM_CMAKE_DIR}")

include(LLVM-Config)
include(TableGen)
include(AddLLVM)
include(AddMLIR)
include(HandleLLVMOptions)
# HandleLLVMOptions runs in both directories and appends the same Ninja pools.
get_property(jit_calc_job_pools GLOBAL PROPERTY JOB_POOLS)
if(jit_calc_job_pools)
  list(REMOVE_DUPLICATES jit_calc_job_pools)
  set_property(GLOBAL PROPERTY JOB_POOLS "${jit_calc_job_pools}")
endif()
include(MLIRDetectPythonEnv)
include(cmake/JitCalcUtils.cmake)

# find whatever python you configured with -DPython3_EXECUTABLE
mlir_configure_python_dev_packages()

include_directories(${LLVM_INCLUDE_DIRS})
include_directories(${MLIR_INCLUDE_DIRS})
include_directories(${PROJECT_SOURCE_DIR}/include)
include_directories(${PROJECT_BINARY_DIR}/include)
add_definitions(${LLVM_DEFINITIONS})

add_subdirectory(include)
add_subdirectory(lib)
add_subdirectory(python)
add_subdirectory(tools)
add_subdirectory(experiments)

# JIT_CALC_EXPERIMENTS_(SOURCE_DIR|BINARY_DIR) are required by testing build
set(JIT_CALC_EXPERIMENTS_SOURCE_DIR ${PROJECT_SOURCE_DIR})
set(JIT_CALC_EXPERIMENTS_BINARY_DIR ${PROJECT_BINARY_DIR})

add_custom_target(JitCalcUnitTests)
add_subdirectory(unittests)
add_subdirectory(test)
