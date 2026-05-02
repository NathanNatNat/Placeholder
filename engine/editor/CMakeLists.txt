add_library(placeholder_editor STATIC
    src/editor.cpp
)

target_include_directories(placeholder_editor PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(placeholder_editor PUBLIC
    placeholder_core
    placeholder_renderer
)
