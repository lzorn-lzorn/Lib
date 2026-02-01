cmake_minimum_required(VERSION 3.3)
project(PureTest)

set(CMAKE_CXX_STANDARD 23)

add_executable(PureTest test_full.cpp)

target_include_directories(PureTest PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
