add_executable(json_test_tests
  json_test_test.cpp
)

target_link_libraries(json_test_tests
  PRIVATE
    json_test
    GTest::gtest_main
)

include(GoogleTest)
gtest_discover_tests(json_test_tests)
