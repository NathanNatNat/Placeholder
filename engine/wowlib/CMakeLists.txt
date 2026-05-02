# WoW Library — self-contained, no engine dependencies.
# This module must be extractable into its own repository at any time.

add_library(placeholder_wowlib STATIC
    src/wowlib.cpp
)

target_include_directories(placeholder_wowlib PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

# wowlib depends only on: standard C++, zlib, cpp-httplib, OpenSSL, stb, libwebp
# These dependencies will be wired in when added as submodules in later phases.
