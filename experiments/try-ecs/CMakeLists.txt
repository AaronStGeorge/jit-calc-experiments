add_llvm_executable(try-ecs try-ecs.cpp)

target_compile_options(try-ecs PRIVATE -fsanitize=address -fno-omit-frame-pointer)
target_link_options(try-ecs PRIVATE -fsanitize=address)