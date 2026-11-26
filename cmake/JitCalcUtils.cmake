# Gather up all python files used in wasm_to_mlir. We mark the resulting pile of
# python code a dependency for the step using wasm_to_mlir. The dependency
# ensures that if wasm_to_mlir changes the downstream dep graph is marked dirty
# and re-run.
file(GLOB_RECURSE WASM_TO_MLIR_PY_FILES "${CMAKE_SOURCE_DIR}/python/wasm/*.py")
list(APPEND WASM_TO_MLIR_PY_FILES "${CMAKE_SOURCE_DIR}/python/wasm_to_mlir.py")

function(c_to_wasm INPUT_C_FILE OUT_OUTPUT)
    get_filename_component(FILE_NAME ${INPUT_C_FILE} NAME_WE)
    set(OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/${FILE_NAME}.wasm")
    file(RELATIVE_PATH REL_OUTPUT "${CMAKE_BINARY_DIR}" "${OUTPUT}")

    add_custom_command(
            OUTPUT ${OUTPUT}

            COMMAND $<TARGET_FILE:WASM_CLANG>
            -O1
            --target=wasm32
            --no-standard-libraries
            -Wl,--allow-undefined
            -Wl,--export-all
            -Wl,--no-entry
            -o ${OUTPUT}
            ${CMAKE_CURRENT_SOURCE_DIR}/${INPUT_C_FILE}

            DEPENDS
            ${CMAKE_CURRENT_SOURCE_DIR}/${INPUT_C_FILE}
            WASM_CLANG

            WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}

            COMMENT "Building WebAssembly module ${REL_OUTPUT}"
            VERBATIM
    )

    # https://stackoverflow.com/a/29894437
    # OUTPUT_FILE is limited to the function's scope, but you can set any
    # "return values" you want on the parents scope by setting a variable in
    # PARENT_SCOPE. To make things a little more reasonable you can pass the
    # name that you would lke to set as a function argument. This is all a sack
    # of boiled dorito farts, but this is CMake c'est la vie
    set(${OUT_OUTPUT} ${OUTPUT} PARENT_SCOPE)
endfunction()

function(wasm_to_mlir INPUT_WASM_FILE OUT_OUTPUT)
    get_filename_component(FILE_NAME ${INPUT_WASM_FILE} NAME_WE)
    set(OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/${FILE_NAME}.mlir")
    file(RELATIVE_PATH REL_OUTPUT "${CMAKE_BINARY_DIR}" "${OUTPUT}")

    add_custom_command(
            OUTPUT ${OUTPUT}

            COMMAND ${CMAKE_COMMAND} -E env PYTHONPATH=${PROJECT_BINARY_DIR}/python_packages/jit_calc
            ${Python3_EXECUTABLE} ${CMAKE_SOURCE_DIR}/python/wasm_to_mlir.py
            ${INPUT_WASM_FILE}
            --output_path=${OUTPUT}

            DEPENDS
            ${INPUT_WASM_FILE}
            ${WASM_TO_MLIR_PY_FILES}
            JitCalcPythonModules

            WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}

            COMMENT "Building MLIR representation ${REL_OUTPUT}"
    )

    set(${OUT_OUTPUT} ${OUTPUT} PARENT_SCOPE)
endfunction()

function(mlir_to_llvm INPUT_MLIR_FILE OUT_OUTPUT PREFIX)
    get_filename_component(FILE_NAME ${INPUT_MLIR_FILE} NAME_WE)
    set(OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/${FILE_NAME}.ll")
    set(LOWERED_MLIR "${CMAKE_CURRENT_BINARY_DIR}/${FILE_NAME}.llvm.mlir")
    file(RELATIVE_PATH REL_OUTPUT "${CMAKE_BINARY_DIR}" "${OUTPUT}")

    add_custom_command(
            OUTPUT ${OUTPUT}
            BYPRODUCTS ${LOWERED_MLIR}

            COMMAND
            $<TARGET_FILE:jit-calc-opt>
            ${INPUT_MLIR_FILE}
            --shim-function='function-to-shim=tick prefix=${PREFIX}'
            --convert-tower-runtime-to-llvm
            --convert-vector-to-scf
            --convert-scf-to-cf
            --convert-cf-to-llvm
            --finalize-memref-to-llvm
            --convert-arith-to-llvm
            --convert-func-to-llvm
            --reconcile-unrealized-casts
            -o ${LOWERED_MLIR}

            COMMAND $<TARGET_FILE:mlir-translate>
            ${LOWERED_MLIR} --mlir-to-llvmir
            -o ${OUTPUT}

            DEPENDS
            ${INPUT_MLIR_FILE}
            jit-calc-opt
            mlir-translate

            WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
            COMMENT "Building LLVM representation ${REL_OUTPUT}"
    )

    set(${OUT_OUTPUT} ${OUTPUT} PARENT_SCOPE)
endfunction()

function(llvm_to_object_file INPUT_LLVM_FILE OUT_OUTPUT)
    get_filename_component(FILE_NAME ${INPUT_LLVM_FILE} NAME_WE)
    set(OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/${FILE_NAME}.o")
    file(RELATIVE_PATH REL_OUTPUT "${CMAKE_BINARY_DIR}" "${OUTPUT}")

    add_custom_command(
            OUTPUT ${OUTPUT}

            COMMAND $<TARGET_FILE:llc> -filetype=obj ${INPUT_LLVM_FILE} -o ${OUTPUT}

            DEPENDS
            ${INPUT_LLVM_FILE}
            llc

            WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
            COMMENT "Building object file ${REL_OUTPUT}"
    )

    set(${OUT_OUTPUT} ${OUTPUT} PARENT_SCOPE)
endfunction()

function(compile_player_code INPUT_C_FILE OUT_OBJECT_FILE PREFIX)
    c_to_wasm(${INPUT_C_FILE} WASM_FILE)
    wasm_to_mlir(${WASM_FILE} MLIR_FILE)
    mlir_to_llvm(${MLIR_FILE} LL_FILE "${PREFIX}")
    llvm_to_object_file(${LL_FILE} OBJECT_FILE)

    set(${OUT_OBJECT_FILE} ${OBJECT_FILE} PARENT_SCOPE)
endfunction()

macro(add_player_code_library LIBRARY_NAME INPUT_C_FILE)
    # Parse arguments after LIBRARY_NAME and INPUT_C_FILE
    cmake_parse_arguments(
        ARG         # prefix
        ""          # options
        "PREFIX"    # one value keywords
        ""          # multi-value keywords
        ${ARGN}
    )

    compile_player_code(${INPUT_C_FILE} OBJECT_FILE "${ARG_PREFIX}")
    add_library(${LIBRARY_NAME} INTERFACE)
    set_target_properties(${LIBRARY_NAME} PROPERTIES
            INTERFACE_SOURCES "${OBJECT_FILE}")
endmacro()
