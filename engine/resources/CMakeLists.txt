add_library(placeholder_resources STATIC
    src/resources.cpp
)

target_include_directories(placeholder_resources PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(placeholder_resources PUBLIC
    placeholder_core
)
