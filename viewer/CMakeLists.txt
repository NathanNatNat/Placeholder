add_executable(placeholder_viewer
    src/main.cpp
)

target_link_libraries(placeholder_viewer PRIVATE
    Placeholder::Core
    Placeholder::Platform
    Placeholder::Resources
    Placeholder::Renderer
    Placeholder::Input
    Placeholder::Editor
    Placeholder::WoWLib
)

target_compile_definitions(placeholder_viewer PRIVATE
    PLACEHOLDER_ROOT_DIR="${CMAKE_SOURCE_DIR}"
)

# Copy OpenSSL DLLs to executable output directory
foreach(_ssl_dll ${OPENSSL_RUNTIME_DLLS_DEBUG})
    add_custom_command(TARGET placeholder_viewer POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different "${_ssl_dll}" $<TARGET_FILE_DIR:placeholder_viewer>
    )
endforeach()
