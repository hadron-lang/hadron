cmake_minimum_required(VERSION 3.10)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

project(Hadron)

# Use Clang as the compiler
set(CMAKE_C_COMPILER clang)
set(CMAKE_CXX_COMPILER clang++)

# Set C and C++ standards
set(CMAKE_C_STANDARD 17)
set(CMAKE_CXX_STANDARD 17)


# Enable static linking and custom flags for Windows release builds
if (CMAKE_SYSTEM_NAME STREQUAL "Windows")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -static -static-libgcc -static-libstdc++ -O3 -Wall -Wpedantic -Wno-format-security")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -static -O3 -Wall -Wpedantic -Wno-format-security")
endif ()

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g3 -O3 -Wall -Wpedantic -Wno-format-security")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -g3 -O3 -Wall -Wpedantic -Wno-format-security")

# Include directories
include_directories(include)

# Collect source files
file(GLOB_RECURSE SOURCES "src/*.cpp")

# Add the executable
add_executable(hadron ${SOURCES})

# Link libraries
target_link_libraries(hadron m readline)

