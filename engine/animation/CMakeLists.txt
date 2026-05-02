add_library(placeholder_animation STATIC
    src/animation.cpp
)
add_library(Placeholder::Animation ALIAS placeholder_animation)

target_include_directories(placeholder_animation PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(placeholder_animation PUBLIC
    Placeholder::Core
)
