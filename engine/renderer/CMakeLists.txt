add_library(placeholder_renderer STATIC
    src/opengl_render_device.cpp
    src/render_state.cpp
    src/shader_program.cpp
    src/shader_preprocessor.cpp
    src/shader_manager.cpp
    src/vertex_array.cpp
    src/uniform_buffer.cpp
    src/forward_pipeline.cpp
    src/texture.cpp
    src/texture_loader.cpp
    src/mesh.cpp
    src/mesh_loader.cpp
    src/material.cpp
    src/skybox.cpp
)
add_library(Placeholder::Renderer ALIAS placeholder_renderer)

target_sources(placeholder_renderer
    PUBLIC
        FILE_SET HEADERS
        BASE_DIRS include
        FILES
            include/renderer/render_types.h
            include/renderer/render_device.h
            include/renderer/render_state.h
            include/renderer/opengl_render_device.h
            include/renderer/shader_program.h
            include/renderer/vertex_array.h
            include/renderer/uniform_buffer.h
            include/renderer/shader_preprocessor.h
            include/renderer/shader_manager.h
            include/renderer/forward_pipeline.h
            include/renderer/texture.h
            include/renderer/texture_loader.h
            include/renderer/mesh.h
            include/renderer/mesh_loader.h
            include/renderer/material.h
            include/renderer/skybox.h
)

target_link_libraries(placeholder_renderer PUBLIC
    Placeholder::Core
    Placeholder::Platform
    Placeholder::Resources
    glad::glad
    glm::glm
    stb::stb
    webp
    assimp::assimp
)
