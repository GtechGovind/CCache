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

# Optionally, you can also add version information to your project
# so that it can be accessed programmatically from the code.
# You can create a header file with version info like this:

configure_file(
        "${CMAKE_SOURCE_DIR}/src/version.h.in"
        "${CMAKE_BINARY_DIR}/version.h"
)

# Include the generated version header in your source files
target_include_directories(CCache PRIVATE "${CMAKE_BINARY_DIR}")
