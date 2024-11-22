cmake_minimum_required(VERSION 3.22)

# Define project name and version
project(CCache VERSION 0.0.1)

# Set the C++ standard to C++17
set(CMAKE_CXX_STANDARD 17)

# Include version information in the executable
add_executable(
        CCache
        src/CCache.h
        src/main.cpp
)

# Include the generated version header in your source files
target_include_directories(CCache PRIVATE "${CMAKE_BINARY_DIR}")
