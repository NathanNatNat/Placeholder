add_library(placeholder_editor STATIC
    src/editor.cpp
    src/editor_console.cpp
    src/editor_panels.cpp
    src/debug_draw.cpp
)
add_library(Placeholder::Editor ALIAS placeholder_editor)

target_sources(placeholder_editor
    PUBLIC
        FILE_SET HEADERS
        BASE_DIRS include
        FILES
            include/editor/editor.h
            include/editor/editor_console.h
            include/editor/editor_panels.h
            include/editor/debug_draw.h
            include/editor/icon_font.h
)

target_link_libraries(placeholder_editor PUBLIC
    Placeholder::Core
    Placeholder::Platform
    Placeholder::Renderer
    Placeholder::Input
    imgui::imgui
    TracyClient
    glm::glm
)
