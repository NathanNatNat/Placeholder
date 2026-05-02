add_library(placeholder_renderer STATIC
    src/opengl_render_device.cpp
    src/render_state.cpp
    src/shader_program.cpp
    src/shader_preprocessor.cpp
    src/shader_manager.cpp
    src/vertex_array.cpp
    src/uniform_buffer.cpp
    src/forward_pipeline.cpp
)

target_include_directories(placeholder_renderer PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(placeholder_renderer PUBLIC
    placeholder_core
    placeholder_platform
    placeholder_resources
    glad
)
