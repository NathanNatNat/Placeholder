add_executable(placeholder_core_tests
    test_config.cpp
    test_allocators.cpp
    test_job_system.cpp
    test_console.cpp
    test_file_io.cpp
)

target_link_libraries(placeholder_core_tests PRIVATE
    Placeholder::Core
    GTest::gtest_main
)

include(GoogleTest)
gtest_discover_tests(placeholder_core_tests)
