add_executable(placeholder_game
    src/main.cpp
)

target_link_libraries(placeholder_game PRIVATE
    placeholder_core
)

target_compile_definitions(placeholder_game PRIVATE
    PLACEHOLDER_ROOT_DIR="${CMAKE_SOURCE_DIR}"
)
