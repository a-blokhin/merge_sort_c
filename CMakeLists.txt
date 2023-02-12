cmake_minimum_required(VERSION 3.16)
project(merge_sort_c CXX)

set(CMAKE_CXX_STANDARD 17)

add_executable(merge_sort_c main.cpp options.h options.cpp generator.h generator.cpp check.h check.cpp sort.h sort.cpp)
