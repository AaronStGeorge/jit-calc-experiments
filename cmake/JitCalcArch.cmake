# Validate the compiler target and select the filename suffix for coroutine assembly.

set(JIT_CALC_ARCH AUTO CACHE STRING "Target architecture (AUTO, ARM64, or X86_64)")
set_property(CACHE JIT_CALC_ARCH PROPERTY STRINGS AUTO ARM64 X86_64)

# For a native build this is the host architecture. Cross builds get their
# target from the toolchain, or from the explicit macOS -arch selection.
set(target_arch "${CMAKE_SYSTEM_PROCESSOR}")
if(APPLE AND CMAKE_OSX_ARCHITECTURES)
  list(LENGTH CMAKE_OSX_ARCHITECTURES arch_count)
  if(NOT arch_count EQUAL 1)
    message(FATAL_ERROR "jit-calc-experiments requires a single CMAKE_OSX_ARCHITECTURES target")
  endif()
  set(target_arch "${CMAKE_OSX_ARCHITECTURES}")
endif()
string(TOLOWER "${target_arch}" target_arch)
if(target_arch MATCHES "^(aarch64|arm64)$")
  set(target_arch ARM64)
elseif(target_arch MATCHES "^(x86_64|amd64)$")
  set(target_arch X86_64)
else()
  message(FATAL_ERROR "Unsupported jit-calc-experiments target architecture: ${target_arch}")
endif()
if(NOT CMAKE_SYSTEM_NAME MATCHES "^(Darwin|Linux)$")
  message(FATAL_ERROR "jit-calc-experiments coroutine backends support macOS and Linux ABIs only")
endif()
if(NOT JIT_CALC_ARCH STREQUAL "AUTO" AND NOT JIT_CALC_ARCH STREQUAL target_arch)
  message(FATAL_ERROR
    "JIT_CALC_ARCH=${JIT_CALC_ARCH} does not match target ${target_arch}; select the compiler target with your toolchain or CMAKE_OSX_ARCHITECTURES")
endif()
string(TOLOWER "${target_arch}" JIT_CALC_ARCH_SUFFIX)
message(STATUS "jit-calc-experiments target architecture: ${target_arch}")
