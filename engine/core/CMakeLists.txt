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
add_library(Placeholder::Core ALIAS placeholder_core)

target_sources(placeholder_core
    PUBLIC
        FILE_SET HEADERS
        BASE_DIRS include
        FILES
            include/core/logging.h
            include/core/config.h
            include/core/file_io.h
            include/core/file_watcher.h
            include/core/allocators.h
            include/core/job_system.h
            include/core/console.h
)

target_link_libraries(placeholder_core PUBLIC
    spdlog::spdlog
    nlohmann_json::nlohmann_json
)
