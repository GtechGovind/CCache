# Specify the minimum required version of CMake
cmake_minimum_required(VERSION 3.22)

# Define the project name and its version
project(CCache VERSION 0.0.1)

# Set the C++ standard for the project. In this case, C++17 is used.
set(CMAKE_CXX_STANDARD 17)

# Add the executable target named 'CCache'. The following files will be used for the compilation:
# - src/CCache.h (Header file)
# - src/main.cpp (Source file)
add_executable(
        CCache  # Name of the output executable
        src/CCache.h  # Header file required by the project
        src/main.cpp  # Source file where the main function is located
)

# Add an include directory for the project. The "PRIVATE" keyword ensures that the specified directory
# is used only within this target and does not propagate to other targets.
# The directory being included here is the binary directory where CMake stores generated files (e.g., build/).
target_include_directories(CCache PRIVATE "${CMAKE_BINARY_DIR}")
