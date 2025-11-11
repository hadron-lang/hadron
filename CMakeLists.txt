cmake_minimum_required(VERSION 3.10)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

project(Hadron)

set(CMAKE_C_COMPILER clang)
set(CMAKE_CXX_COMPILER clang++)

set(CMAKE_C_STANDARD 20)
set(CMAKE_CXX_STANDARD 20)


if (CMAKE_SYSTEM_NAME STREQUAL "Windows")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -static -static-libgcc -static-libstdc++ -O3 -Wall -Wpedantic -Wno-format-security")
  set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -static -O3 -Wall -Wpedantic -Wno-format-security")
endif ()

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g3 -O3 -Wall -Wpedantic -Wno-format-security")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -g3 -O3 -Wall -Wpedantic -Wno-format-security")

include_directories(include)

file(GLOB_RECURSE SOURCES "src/*.cpp")

add_executable(hadronc ${SOURCES})

target_link_libraries(hadronc m)
