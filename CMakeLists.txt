cmake_minimum_required(VERSION 3.26)
project(Deque)

set(CMAKE_MODULE_PATH              "${CMAKE_SOURCE_DIR}/cmake")

set(CMAKE_CXX_STANDARD 20)

find_package(Catch REQUIRED)

add_executable(Deque deque.h
        test/test.cpp)
target_link_libraries(Deque PRIVATE contrib_catch_main)