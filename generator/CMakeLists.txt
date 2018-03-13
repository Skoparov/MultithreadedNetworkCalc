cmake_minimum_required(VERSION 2.8)
set( PROJECT expr_generator )
project( ${PROJECT} )

set( CMAKE_CXX_FLAGS ${CMAKE_CXX_FLAGS} "-std=c++11" )
find_package(Boost COMPONENTS program_options REQUIRED)

add_executable( ${PROJECT}
                main.cpp
                generator.h
                generator.cpp )

include_directories( ${Boost_INCLUDE_DIRS} )
link_directories( ${Boost_LIBRARY_DIRS} )
target_link_libraries( ${PROJECT} ${Boost_LIBRARIES} )

