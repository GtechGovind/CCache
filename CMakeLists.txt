# Specify the minimum required version of CMake
cmake_minimum_required(VERSION 3.22)

# Define the project name and its version
project(CCache VERSION 0.0.6)

# Set the C++ standard for the project
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED YES)

# Create an interface library for the header-only library
add_library(CCache INTERFACE)

# Specify include directories for the header file
target_include_directories(CCache INTERFACE "${CMAKE_SOURCE_DIR}/src")

# Install rules for the header file (optional, for deployment)
install(TARGETS CCache EXPORT CCacheTargets)

install(DIRECTORY src/
        DESTINATION include
        FILES_MATCHING PATTERN "*.h")
