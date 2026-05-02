add_library(placeholder_input STATIC
    src/input_manager.cpp
    src/fly_camera.cpp
    src/orbit_camera.cpp
)
add_library(Placeholder::Input ALIAS placeholder_input)

target_sources(placeholder_input
    PUBLIC
        FILE_SET HEADERS
        BASE_DIRS include
        FILES
            include/input/input_types.h
            include/input/input_manager.h
            include/input/camera.h
            include/input/fly_camera.h
            include/input/orbit_camera.h
)

target_link_libraries(placeholder_input PUBLIC
    Placeholder::Core
    Placeholder::Platform
    glm::glm
)
