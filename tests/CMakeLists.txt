add_executable(json_test_tests
  gen/example1_test.cpp
  gen/test_module.cpp
)

target_include_directories(json_test_tests
  PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/gen
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
