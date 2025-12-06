cmake_minimum_required(VERSION 4.0)
project(PureTest)

set(CMAKE_CXX_STANDARD 20)

add_executable(PureTest test.cpp
        Array.h)

target_include_directories(PureTest PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
