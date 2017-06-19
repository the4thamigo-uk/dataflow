project (test)
cmake_minimum_required (VERSION 2.6)
set(CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR}/CMakeModules)

find_package(CxxTest)
find_program( MEMORYCHECK_COMMAND valgrind )

set( CMAKE_EXPORT_COMPILE_COMMANDS ON )
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set( MEMORYCHECK_COMMAND_OPTIONS "--trace-children=yes --leak-check=full" )

if(CMAKE_COMPILER_IS_GNUCXX)
    include(CodeCoverage)
    APPEND_COVERAGE_COMPILER_FLAGS()
    SETUP_TARGET_FOR_COVERAGE(i
      NAME coverage
      EXECUTABLE ${CMAKE_BINARY_DIR}/bin/tests
      )
endif()

enable_testing()

include_directories ("${PROJECT_SOURCE_DIR}/include")

CXXTEST_ADD_TEST(tests test/runner_test.cpp test/test.hpp)

