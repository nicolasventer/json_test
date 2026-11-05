add_executable(json_test_tests
  json_test_test.cpp
  suite_test.cpp
)

target_link_libraries(json_test_tests
  PRIVATE
    json_test
    GTest::gtest_main
)

file(TO_CMAKE_PATH "${CMAKE_SOURCE_DIR}/example1.json" JSON_TEST_SUITE_PATH)
target_compile_definitions(json_test_tests
  PRIVATE
    JSON_TEST_SUITE_PATH="${JSON_TEST_SUITE_PATH}"
)

include(GoogleTest)
if(CMAKE_VERSION VERSION_GREATER_EQUAL "3.18")
  gtest_discover_tests(json_test_tests DISCOVERY_MODE PRE_TEST)
else()
  gtest_discover_tests(json_test_tests)
endif()
