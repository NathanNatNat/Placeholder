add_executable(placeholder_integration_tests
    placeholder_integration.cpp
)

target_link_libraries(placeholder_integration_tests PRIVATE
    Placeholder::Core
    GTest::gtest_main
)

include(GoogleTest)
gtest_discover_tests(placeholder_integration_tests)
