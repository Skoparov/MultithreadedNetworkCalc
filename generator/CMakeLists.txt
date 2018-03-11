cmake_minimum_required(VERSION 2.6)
set( PROJECT expr_generator )
project( ${PROJECT} )

SET( CMAKE_CXX_FLAGS ${CMAKE_CXX_FLAGS} "-std=c++11" )

ADD_EXECUTABLE( ${PROJECT}
                main.cpp
                generator.h
                generator.cpp )
