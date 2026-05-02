add_executable(placeholder_resources_tests
    test_resource_pool.cpp
)

target_link_libraries(placeholder_resources_tests PRIVATE
    Placeholder::Resources
    GTest::gtest
    GTest::gtest_main
)

include(GoogleTest)
gtest_discover_tests(placeholder_resources_tests)
