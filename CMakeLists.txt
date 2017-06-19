project (test)
cmake_minimum_required (VERSION 2.6)
find_package(CxxTest)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

enable_testing()

include_directories ("${PROJECT_SOURCE_DIR}/include")

CXXTEST_ADD_TEST(tests test/runner_test.cpp test/test.hpp)

