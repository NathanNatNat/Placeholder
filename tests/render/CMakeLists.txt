add_executable(placeholder_render_tests
    render_test_scaffold.cpp
)

target_link_libraries(placeholder_render_tests PRIVATE
    Placeholder::Core
    Placeholder::Renderer
    Placeholder::Platform
    GTest::gtest_main
)

include(GoogleTest)
gtest_discover_tests(placeholder_render_tests)
