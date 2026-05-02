add_executable(placeholder_game
    src/main.cpp
)

target_link_libraries(placeholder_game PRIVATE
    Placeholder::Core
    Placeholder::Platform
    Placeholder::Resources
    Placeholder::Renderer
    Placeholder::Input
    Placeholder::Editor
)

target_compile_definitions(placeholder_game PRIVATE
    PLACEHOLDER_ROOT_DIR="${CMAKE_SOURCE_DIR}"
)
