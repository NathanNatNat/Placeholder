add_library(placeholder_audio STATIC
    src/audio.cpp
)

target_include_directories(placeholder_audio PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(placeholder_audio PUBLIC
    placeholder_core
)
