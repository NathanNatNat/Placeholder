# WoW Library — self-contained, no engine dependencies.
# This module must be extractable into its own repository at any time.
# Dependencies: standard C++, zlib, cpp-httplib, OpenSSL, stb, libwebp

add_library(placeholder_wowlib STATIC
    src/data_buffer.cpp
    src/jenkins96.cpp
    src/salsa20.cpp
    src/tact_keys.cpp
    src/blte_reader.cpp
    src/casc_source.cpp
    src/casc_local.cpp
    src/casc_remote.cpp
    src/cdn_config.cpp
    src/cdn_resolver.cpp
    src/build_cache.cpp
    src/blp_image.cpp
    src/db2_reader.cpp
    src/m2_loader.cpp
    src/wmo_loader.cpp
)
add_library(Placeholder::WoWLib ALIAS placeholder_wowlib)

target_sources(placeholder_wowlib PUBLIC
    FILE_SET HEADERS
    BASE_DIRS ${CMAKE_CURRENT_SOURCE_DIR}/include
    FILES
        include/wowlib/data_buffer.h
        include/wowlib/jenkins96.h
        include/wowlib/salsa20.h
        include/wowlib/tact_keys.h
        include/wowlib/blte_reader.h
        include/wowlib/casc_source.h
        include/wowlib/casc_local.h
        include/wowlib/casc_remote.h
        include/wowlib/cdn_config.h
        include/wowlib/cdn_resolver.h
        include/wowlib/build_cache.h
        include/wowlib/blp_image.h
        include/wowlib/db2_reader.h
        include/wowlib/m2_types.h
        include/wowlib/m2_loader.h
        include/wowlib/wmo_types.h
        include/wowlib/wmo_loader.h
)

target_link_libraries(placeholder_wowlib PUBLIC
    zlibstatic
    OpenSSL::Crypto
    httplib::httplib
    stb::stb
    webp
    nlohmann_json::nlohmann_json
)
