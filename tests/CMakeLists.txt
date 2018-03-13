cmake_minimum_required(VERSION 2.8)

set(TEST_PROJECT "tests")
PROJECT(${TEST_PROJECT})

add_definitions("-std=c++11")

find_library(PTHREAD pthread)
find_package(Boost COMPONENTS unit_test_framework system thread REQUIRED)

set( SOURCE_DIR ../ )
file( GLOB SOURCES "tests.cpp"
                    "mocks.h"
                    "${SOURCE_DIR}/generator/*.h"
                    "${SOURCE_DIR}/generator/generator.cpp"
                   "${SOURCE_DIR}/calculator/*.h"
                   "${SOURCE_DIR}/calculator/server.cpp"
                   "${SOURCE_DIR}/calculator/calculator.cpp"
                   "${SOURCE_DIR}/calculator/big_int/*.hh"
                   "${SOURCE_DIR}/calculator/big_int/*.cc" )

include_directories( ../calculator )
include_directories( ../generator )

include_directories(${SOURCE_DIR} ${Boost_INCLUDE_DIRS})
add_definitions(-DBOOST_TEST_DYN_LINK)

add_executable(${TEST_PROJECT} ${SOURCES})
link_directories (${CMAKE_RUNTIME_OUTPUT_DIRECTORY})
target_link_libraries(${TEST_PROJECT} ${PTHREAD} ${Boost_LIBRARIES} ${Boost_UNIT_TEST_FRAMEWORK_LIBRARY} )
