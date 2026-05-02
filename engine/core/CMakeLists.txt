add_library(placeholder_core STATIC
    src/core.cpp
)

target_include_directories(placeholder_core PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)
