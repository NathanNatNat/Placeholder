cmake_minimum_required(VERSION 3.20)
project(Placeholder VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Enforce Windows x64 only
if(NOT CMAKE_SIZEOF_VOID_P EQUAL 8)
    message(FATAL_ERROR "Placeholder requires a 64-bit build. Configure with -A x64.")
endif()

if(NOT MSVC)
    message(FATAL_ERROR "Placeholder requires MSVC. Use Visual Studio Build Tools or Visual Studio.")
endif()

# MSVC compile options
add_compile_options(/W4 /utf-8 /permissive-)

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
