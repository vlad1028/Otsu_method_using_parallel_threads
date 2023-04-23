cmake_minimum_required(VERSION 3.24)
project(ompParallel)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_FLAGS "-fopenmp")

add_executable(ompParallel main.cpp)
