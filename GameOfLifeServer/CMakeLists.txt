cmake_minimum_required(VERSION 3.3)
project(GameOfLifeServer)

SET(CMAKE_EXE_LINKER_FLAGS "-pthread")

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")

set(SOURCE_FILES server.cpp main.cpp main.h)
add_executable(GameOfLifeServer ${SOURCE_FILES})