cmake_minimum_required(VERSION 3.20.0)
project(jit-calc-experiments LANGUAGES CXX C ASM_NASM)

set(CMAKE_BUILD_WITH_INSTALL_NAME_DIR ON)

set(CMAKE_CXX_STANDARD 17 CACHE STRING "C++ standard to conform to")

find_package(MLIR REQUIRED CONFIG)

message(STATUS "Using MLIRConfig.cmake in: ${MLIR_DIR}")
message(STATUS "Using LLVMConfig.cmake in: ${LLVM_DIR}")

set(LLVM_RUNTIME_OUTPUT_INTDIR ${CMAKE_BINARY_DIR}/bin)
set(LLVM_LIBRARY_OUTPUT_INTDIR ${CMAKE_BINARY_DIR}/lib)
set(MLIR_BINARY_DIR ${CMAKE_BINARY_DIR})

list(APPEND CMAKE_MODULE_PATH "${MLIR_CMAKE_DIR}")
list(APPEND CMAKE_MODULE_PATH "${LLVM_CMAKE_DIR}")

include(TableGen)
include(AddLLVM)
include(AddMLIR)
include(HandleLLVMOptions)

include_directories(${LLVM_INCLUDE_DIRS})
include_directories(${MLIR_INCLUDE_DIRS})
include_directories(${PROJECT_SOURCE_DIR}/include)
include_directories(${PROJECT_BINARY_DIR}/include)
link_directories(${LLVM_BUILD_LIBRARY_DIR})
add_definitions(${LLVM_DEFINITIONS})

add_subdirectory(count-to-n)
add_subdirectory(jit-calc)
add_subdirectory(slow-roll-fibonacci-1)
add_subdirectory(slow-roll-fibonacci-2)

# JIT_CALC_EXPERIMENTS_(SOURCE_DIR|BINARY_DIR) are required by testing build
set(JIT_CALC_EXPERIMENTS_SOURCE_DIR ${PROJECT_SOURCE_DIR})
set(JIT_CALC_EXPERIMENTS_BINARY_DIR ${PROJECT_BINARY_DIR})
add_subdirectory(test)