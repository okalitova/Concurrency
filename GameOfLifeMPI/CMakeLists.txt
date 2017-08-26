cmake_minimum_required(VERSION 3.3)
project(GameOfLifeMPI)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")

SET(CMAKE_EXE_LINKER_FLAGS "-pthread")

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -fopenmp")

set(SOURCE_FILES main.cpp main2.cpp)
add_executable(GameOfLifeMPI ${SOURCE_FILES})