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

# Assimp — 3D model importing (glTF, OBJ, FBX)
set(BUILD_SHARED_LIBS OFF CACHE INTERNAL "")
set(ASSIMP_BUILD_TESTS OFF CACHE INTERNAL "")
set(ASSIMP_BUILD_SAMPLES OFF CACHE INTERNAL "")
set(ASSIMP_BUILD_ASSIMP_TOOLS OFF CACHE INTERNAL "")
set(ASSIMP_INSTALL OFF CACHE INTERNAL "")
set(ASSIMP_NO_EXPORT ON CACHE INTERNAL "")
set(ASSIMP_WARNINGS_AS_ERRORS OFF CACHE INTERNAL "")
set(ASSIMP_INJECT_DEBUG_POSTFIX OFF CACHE INTERNAL "")
set(ASSIMP_BUILD_ZLIB ON CACHE INTERNAL "")
add_subdirectory(third_party/assimp SYSTEM)

# libwebp — WebP image decoding/encoding
set(WEBP_BUILD_ANIM_UTILS OFF CACHE INTERNAL "")
set(WEBP_BUILD_CWEBP OFF CACHE INTERNAL "")
set(WEBP_BUILD_DWEBP OFF CACHE INTERNAL "")
set(WEBP_BUILD_GIF2WEBP OFF CACHE INTERNAL "")
set(WEBP_BUILD_IMG2WEBP OFF CACHE INTERNAL "")
set(WEBP_BUILD_VWEBP OFF CACHE INTERNAL "")
set(WEBP_BUILD_WEBPINFO OFF CACHE INTERNAL "")
set(WEBP_BUILD_LIBWEBPMUX OFF CACHE INTERNAL "")
set(WEBP_BUILD_WEBPMUX OFF CACHE INTERNAL "")
set(WEBP_BUILD_EXTRAS OFF CACHE INTERNAL "")
add_subdirectory(third_party/libwebp SYSTEM)

# stb — image loading/writing (header-only, needs one implementation TU)
add_library(stb STATIC third_party/stb_impl.cpp)
target_include_directories(stb PUBLIC third_party/stb)
target_compile_definitions(stb PRIVATE
    STBI_WINDOWS_UTF8          # Use UTF-8 file paths on Windows
    _CRT_SECURE_NO_WARNINGS    # Suppress MSVC deprecation warnings in stb
)
add_library(stb::stb ALIAS stb)

# FreeType — font glyph rasterization (used by ImGui FreeType backend)
set(FT_DISABLE_ZLIB ON CACHE INTERNAL "")
set(FT_DISABLE_BZIP2 ON CACHE INTERNAL "")
set(FT_DISABLE_PNG ON CACHE INTERNAL "")
set(FT_DISABLE_HARFBUZZ ON CACHE INTERNAL "")
set(FT_DISABLE_BROTLI ON CACHE INTERNAL "")
add_subdirectory(third_party/freetype SYSTEM)

# Dear ImGui (docking branch) — compiled as a static library
add_library(imgui STATIC
    third_party/imgui/imgui.cpp
    third_party/imgui/imgui_demo.cpp
    third_party/imgui/imgui_draw.cpp
    third_party/imgui/imgui_tables.cpp
    third_party/imgui/imgui_widgets.cpp
    third_party/imgui/backends/imgui_impl_glfw.cpp
    third_party/imgui/backends/imgui_impl_opengl3.cpp
    third_party/imgui/misc/freetype/imgui_freetype.cpp
)
target_include_directories(imgui PUBLIC
    third_party/imgui
    third_party/imgui/backends
    third_party/imgui/misc/freetype
)
target_link_libraries(imgui PUBLIC glfw glad::glad freetype)
target_compile_definitions(imgui PUBLIC
    IMGUI_ENABLE_FREETYPE            # Use FreeType for font rasterization
)
add_library(imgui::imgui ALIAS imgui)

# Tracy — real-time CPU/GPU profiler
option(TRACY_ENABLE "Enable Tracy profiling" OFF)
set(TRACY_ON_DEMAND ON CACHE INTERNAL "")
add_subdirectory(third_party/tracy SYSTEM)

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
    add_subdirectory(engine/resources/tests)
    add_subdirectory(tests/integration)
    add_subdirectory(tests/render)
endif()
