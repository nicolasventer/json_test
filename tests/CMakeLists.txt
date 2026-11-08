add_executable(json_test_tests
  example1_test.cpp
)

target_link_libraries(json_test_tests
  PRIVATE
    json_test
    GTest::gtest_main
)

include(GoogleTest)
if(CMAKE_VERSION VERSION_GREATER_EQUAL "3.18")
  gtest_discover_tests(json_test_tests DISCOVERY_MODE PRE_TEST)
else()
  gtest_discover_tests(json_test_tests)
endif()
