# Specify the minimum required version of CMake
cmake_minimum_required(VERSION 3.22)

# Define the project name and its version
project(CCache VERSION 0.0.1)

# Set the C++ standard for the project. In this case, C++17 is used.
set(CMAKE_CXX_STANDARD 17)

# Define 'CCache' as a shared library (you can use STATIC if needed)
add_library(CCache SHARED
        src/CCache.h     # Header file
        src/main.cpp     # Source file with the 'main' function
)

# Optionally export symbols if you're building a shared library
set_target_properties(CCache PROPERTIES ENABLE_EXPORTS YES)

# Include directories for CCache library (PUBLIC makes them available to other projects linking this library)
target_include_directories(CCache PUBLIC "${CMAKE_SOURCE_DIR}/src")

# If you want to link this library to other projects, use the following (in the other project's CMakeLists.txt)
# target_link_libraries(OtherProject PRIVATE CCache)

# Optionally, you can specify a directory for the library if needed
# target_link_directories(OtherProject PRIVATE /path/to/CCache/library)

# If CCache is built in a separate location, you may need to define the CMAKE_PREFIX_PATH
# set(CMAKE_PREFIX_PATH "/path/to/CCache/lib/cmake")
