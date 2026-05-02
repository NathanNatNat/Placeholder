add_library(placeholder_platform STATIC
    src/platform.cpp
)

target_include_directories(placeholder_platform PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(placeholder_platform PUBLIC
    placeholder_core
)
