add_library(placeholder_renderer STATIC
    src/renderer.cpp
)

target_include_directories(placeholder_renderer PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(placeholder_renderer PUBLIC
    placeholder_core
    placeholder_platform
    placeholder_resources
)
