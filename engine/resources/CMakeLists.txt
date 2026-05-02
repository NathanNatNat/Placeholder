add_library(placeholder_resources STATIC
    src/resources.cpp
)
add_library(Placeholder::Resources ALIAS placeholder_resources)

target_sources(placeholder_resources
    PUBLIC
        FILE_SET HEADERS
        BASE_DIRS include
        FILES
            include/resources/resource_handle.h
            include/resources/resource_pool.h
)

target_link_libraries(placeholder_resources PUBLIC
    Placeholder::Core
)
