add_library(placeholder_editor STATIC
    src/editor.cpp
)
add_library(Placeholder::Editor ALIAS placeholder_editor)

target_include_directories(placeholder_editor PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(placeholder_editor PUBLIC
    Placeholder::Core
    Placeholder::Renderer
)
