add_executable(count-to-n count-to-n.c asm.asm)
set_target_properties(count-to-n
				PROPERTIES
				RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
				)