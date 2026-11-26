# Build the hover-game demo and link its compiled player code.

add_llvm_executable(hover-game main.cpp)
add_player_code_library(hover player-code/simple.c)
target_link_libraries(hover-game
    hover
    raylib
    TowerRuntime
)

# Checks if OSX and links appropriate frameworks (only required on MacOS)
if (APPLE)
    target_link_libraries(hover-game "-framework IOKit")
    target_link_libraries(hover-game "-framework Cocoa")
    target_link_libraries(hover-game "-framework OpenGL")
endif ()
