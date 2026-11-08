cmake_minimum_required(VERSION 3.16)

project(json_test LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

include(FetchContent)

FetchContent_Declare(
  googletest
  URL https://github.com/google/googletest/archive/refs/tags/v1.15.2.zip
  DOWNLOAD_EXTRACT_TIMESTAMP TRUE
)

# Avoid mixing the CRT with MSVC on Windows.
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)

FetchContent_MakeAvailable(googletest)

FetchContent_Declare(
  cpp-terminal
  URL https://github.com/jupyter-xeus/cpp-terminal/archive/refs/heads/master.zip
  DOWNLOAD_EXTRACT_TIMESTAMP TRUE
)

set(CPPTERMINAL_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(CPPTERMINAL_ENABLE_TESTING OFF CACHE BOOL "" FORCE)
set(CPPTERMINAL_ENABLE_DOCS OFF CACHE BOOL "" FORCE)
set(CPPTERMINAL_ENABLE_INSTALL OFF CACHE BOOL "" FORCE)

FetchContent_MakeAvailable(cpp-terminal)

enable_testing()

add_library(json_test
  src/device_logic.cpp
  src/test_module.cpp
  Timer.cpp
)

target_include_directories(json_test
  PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)

add_executable(json_test_app
  src/main.cpp
)

target_link_libraries(json_test_app
  PRIVATE
    cpp-terminal
)

add_subdirectory(tests)
