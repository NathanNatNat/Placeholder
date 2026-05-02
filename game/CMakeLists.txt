add_executable(placeholder_game
    src/main.cpp
)

target_link_libraries(placeholder_game PRIVATE
    placeholder_core
)
