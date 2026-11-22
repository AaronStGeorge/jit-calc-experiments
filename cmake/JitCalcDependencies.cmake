# Add the vendored LLVM/MLIR and raylib submodules to jit-calc's CMake build.
# Check that the submodules are initialized and expose the LLVM/MLIR settings
# needed by jit-calc without importing a separate LLVM installation.

include_guard(GLOBAL)

function(jit_calc_require_submodule path)
  if(NOT EXISTS "${path}/CMakeLists.txt")
    message(FATAL_ERROR "Missing ${path}. Run git submodule update --init --recursive.")
  endif()
endfunction()

function(jit_calc_add_bundled_llvm)
  set(llvm_src "${PROJECT_SOURCE_DIR}/third_party/llvm-project/llvm")
  set(mlir_src "${PROJECT_SOURCE_DIR}/third_party/llvm-project/mlir")
  set(llvm_build "${PROJECT_BINARY_DIR}/third_party/llvm-project")
  jit_calc_require_submodule("${llvm_src}")

  # LLVM/MLIR targets share our build graph, but LLVM's own default build,
  # examples, and regression suites are not dependencies of our default build.
  # LLVM's own minimum CMake version predates CMP0126; keep the normal
  # variables below when its set(... CACHE ...) calls initialize the cache.
  set(CMAKE_POLICY_DEFAULT_CMP0126 NEW)
  # Keep LLVM/MLIR at the pinned revision's default C++ standard. In C++20
  # mode libstdc++ 15 rejects MLIR's Dominance.cpp iterator instantiation.
  set(CMAKE_CXX_STANDARD 17)
  set(LLVM_ENABLE_PROJECTS mlir)
  set(LLVM_TARGETS_TO_BUILD host)
  set(LLVM_RUNTIME_OUTPUT_INTDIR "${llvm_build}/${CMAKE_CFG_INTDIR}/bin")
  set(LLVM_LIBRARY_OUTPUT_INTDIR "${llvm_build}/${CMAKE_CFG_INTDIR}/lib${LLVM_LIBDIR_SUFFIX}")
  set(LLVM_INCLUDE_UTILS ON)
  set(LLVM_INCLUDE_TESTS ON) # LLVM's gtest targets are gated on this.
  set(LLVM_INCLUDE_EXAMPLES OFF)
  set(LLVM_INCLUDE_BENCHMARKS OFF)
  set(MLIR_INCLUDE_TESTS ON) # jit-calc-lsp-server registers MLIR test dialects.
  set(MLIR_ENABLE_BINDINGS_PYTHON ON)
  set(LLVM_ENABLE_LIBEDIT OFF CACHE BOOL "Build LLVM with libedit")
  set(LLVM_ENABLE_LIBXML2 OFF CACHE BOOL "Build LLVM with libxml2")
  set(LLVM_ENABLE_ZLIB OFF CACHE BOOL "Build LLVM with zlib")
  set(LLVM_ENABLE_ZSTD OFF CACHE BOOL "Build LLVM with zstd")

  message(STATUS "Adding bundled LLVM projects: ${LLVM_ENABLE_PROJECTS}")
  add_subdirectory("${llvm_src}" "${llvm_build}" EXCLUDE_FROM_ALL)

  # In-tree targets already exist. Export only the variables that the LLVM
  # CMake helpers and jit-calc need, rather than importing a second LLVM.
  foreach(var IN ITEMS LLVM_ABI_BREAKING_CHECKS LLVM_AVAILABLE_LIBS
      LLVM_BINARY_DIR LLVM_CMAKE_DIR LLVM_COMMON_DEPENDS LLVM_DEFINITIONS
      LLVM_ENABLE_ASSERTIONS LLVM_ENABLE_EH LLVM_ENABLE_PIC LLVM_ENABLE_RTTI
      LLVM_LIBRARY_DIR LLVM_MAIN_INCLUDE_DIR LLVM_NATIVE_ARCH LLVM_TARGETS_TO_BUILD
      LLVM_TOOLS_BINARY_DIR LLVM_USE_SPLIT_DWARF)
    get_directory_property(value DIRECTORY "${llvm_src}" DEFINITION "${var}")
    set(${var} "${value}" PARENT_SCOPE)
  endforeach()
  foreach(var IN ITEMS MLIR_ENABLE_BINDINGS_PYTHON MLIR_ENABLE_EXECUTION_ENGINE)
    get_directory_property(value DIRECTORY "${mlir_src}" DEFINITION "${var}")
    set(${var} "${value}" PARENT_SCOPE)
  endforeach()
  set(LLVM_INCLUDE_DIRS "${llvm_src}/include;${llvm_build}/include" PARENT_SCOPE)
  set(MLIR_INCLUDE_DIRS "${mlir_src}/include;${llvm_build}/tools/mlir/include" PARENT_SCOPE)
  set(MLIR_CMAKE_DIR "${mlir_src}/cmake/modules" PARENT_SCOPE)
  set(LLVM_EXTERNAL_LIT "${llvm_src}/utils/lit/lit.py" PARENT_SCOPE)
endfunction()

function(jit_calc_add_bundled_raylib)
  set(raylib_src "${PROJECT_SOURCE_DIR}/third_party/raylib")
  jit_calc_require_submodule("${raylib_src}")
  set(BUILD_EXAMPLES OFF)
  set(BUILD_SHARED_LIBS OFF)
  add_subdirectory("${raylib_src}" "${PROJECT_BINARY_DIR}/third_party/raylib" EXCLUDE_FROM_ALL)
endfunction()
