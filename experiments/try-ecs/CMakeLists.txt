add_llvm_executable(try-ecs ecs.h ecs.cpp try-ecs.cpp)

target_link_libraries(try-ecs
    raylib
    MLIRSupport
)

# Checks if OSX and links appropriate frameworks (only required on MacOS)
if (APPLE)
    target_link_libraries(try-ecs "-framework IOKit")
    target_link_libraries(try-ecs "-framework Cocoa")
    target_link_libraries(try-ecs "-framework OpenGL")
endif ()