add_library(placeholder_input STATIC
    src/input.cpp
)

target_include_directories(placeholder_input PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(placeholder_input PUBLIC
    placeholder_core
    placeholder_platform
)
