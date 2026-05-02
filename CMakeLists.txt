cmake_minimum_required(VERSION 3.31)
project(Placeholder VERSION 0.1.0 LANGUAGES C CXX)

# Enforce Windows x64 only
if(NOT CMAKE_SIZEOF_VOID_P EQUAL 8)
    message(FATAL_ERROR
        "Placeholder requires a 64-bit (x64) build.\n"
        "  Ninja: run from an x64 Developer Command Prompt / PowerShell.\n"
        "  Visual Studio: configure with -A x64 or use the x64 preset.\n"
        "  Detected pointer size: ${CMAKE_SIZEOF_VOID_P} bytes (expected 8)."
    )
endif()

if(MSVC AND NOT CMAKE_CXX_COMPILER_ARCHITECTURE_ID STREQUAL "x64"
        AND NOT CMAKE_CXX_COMPILER_ARCHITECTURE_ID STREQUAL "X64")
    message(FATAL_ERROR
        "Placeholder requires the x64 MSVC toolchain.\n"
        "  Detected compiler architecture: ${CMAKE_CXX_COMPILER_ARCHITECTURE_ID}\n"
        "  Use an x64 Developer Command Prompt or select the x64 host/target toolset."
    )
endif()

if(NOT MSVC)
    message(FATAL_ERROR "Placeholder requires MSVC. Use Visual Studio Build Tools or Visual Studio.")
endif()

include(CTest)

# Testing (only when this is the top-level project)
option(PLACEHOLDER_BUILD_TESTS "Build unit and integration tests" ON)

# Third-party dependencies (SYSTEM suppresses warnings from these targets)
add_subdirectory(third_party/spdlog SYSTEM)

set(JSON_BuildTests OFF CACHE INTERNAL "")
set(JSON_Install OFF CACHE INTERNAL "")
add_subdirectory(third_party/json SYSTEM)

set(GLFW_BUILD_DOCS OFF CACHE INTERNAL "")
set(GLFW_BUILD_TESTS OFF CACHE INTERNAL "")
set(GLFW_BUILD_EXAMPLES OFF CACHE INTERNAL "")
set(GLFW_INSTALL OFF CACHE INTERNAL "")
add_subdirectory(third_party/glfw SYSTEM)

set(GLM_BUILD_TESTS OFF CACHE INTERNAL "")
set(GLM_BUILD_INSTALL OFF CACHE INTERNAL "")
add_subdirectory(third_party/glm SYSTEM)

if(PLACEHOLDER_BUILD_TESTS)
    set(gtest_force_shared_crt ON CACHE INTERNAL "")
    set(BUILD_GMOCK OFF CACHE INTERNAL "")
    set(INSTALL_GTEST OFF CACHE INTERNAL "")
    add_subdirectory(third_party/googletest SYSTEM)
endif()

# GLAD2 — OpenGL 4.6 Core loader (generated)
add_library(glad STATIC third_party/glad/src/gl.c)
target_include_directories(glad PUBLIC third_party/glad/include)
add_library(glad::glad ALIAS glad)

# Engine modules (static libraries)
add_subdirectory(engine/core)
add_subdirectory(engine/platform)
add_subdirectory(engine/resources)
add_subdirectory(engine/renderer)
add_subdirectory(engine/input)
add_subdirectory(engine/audio)
add_subdirectory(engine/animation)
add_subdirectory(engine/editor)
add_subdirectory(engine/wowlib)

# Game executable
add_subdirectory(game)

# Tests
if(PLACEHOLDER_BUILD_TESTS)
    add_subdirectory(engine/core/tests)
    add_subdirectory(tests/integration)
    add_subdirectory(tests/render)
endif()
