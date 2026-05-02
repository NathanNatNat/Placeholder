add_library(placeholder_core STATIC
    src/core.cpp
    src/logging.cpp
    src/config.cpp
    src/file_io.cpp
    src/file_watcher.cpp
    src/allocators.cpp
    src/job_system.cpp
    src/console.cpp
)

target_include_directories(placeholder_core PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(placeholder_core PUBLIC
    spdlog::spdlog
    nlohmann_json::nlohmann_json
)
