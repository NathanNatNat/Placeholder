add_library(placeholder_platform STATIC
    src/window.cpp
)
add_library(Placeholder::Platform ALIAS placeholder_platform)

target_sources(placeholder_platform
    PUBLIC
        FILE_SET HEADERS
        BASE_DIRS include
        FILES
            include/platform/window.h
)

target_link_libraries(placeholder_platform PUBLIC
    Placeholder::Core
    glfw
    glad::glad
)
