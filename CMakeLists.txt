cmake_minimum_required(VERSION 3.22)

project(CCache VERSION 0.0.1)

set(CMAKE_CXX_STANDARD 17)

add_executable(
        CCache
        src/CCache.h
        src/main.cpp
)

target_include_directories(CCache PRIVATE "${CMAKE_BINARY_DIR}")
