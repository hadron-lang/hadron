cmake_minimum_required(VERSION 3.25)

project(Hadron VERSION 0.0.1 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

option(ENABLE_SANITIZERS "Enable Sanitizers" ON)
option(BUILD_TESTS "Build unit tests" ON)

find_package(LLVM REQUIRED CONFIG)

if (TARGET LLVM)
    set(llvm_libs LLVM)
else ()
    llvm_map_components_to_libnames(llvm_libs core support
            native nativecodegen demangle targetparser
            scalaropts instcombine ipo passes)
endif ()

if (MSVC)
    set(HADRON_FLAGS /W4 /WX /permissive-)
else ()
    set(HADRON_FLAGS -Wall -Wextra -Wpedantic -Wconversion -Wshadow -Werror -Wno-unused-parameter)
    if (ENABLE_SANITIZERS AND CMAKE_BUILD_TYPE MATCHES "Debug")
        list(APPEND HADRON_FLAGS -fsanitize=address,undefined -fno-omit-frame-pointer)
        add_link_options(-fsanitize=address,undefined)
    endif ()
endif ()

file(GLOB_RECURSE LIB_SOURCES "src/frontend/*.cpp" "src/backend/*.cpp" "src/common/*.cpp")

add_library(hadron_core STATIC ${LIB_SOURCES})
target_include_directories(hadron_core PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include")
target_include_directories(hadron_core SYSTEM PUBLIC ${LLVM_INCLUDE_DIRS})
target_compile_definitions(hadron_core PUBLIC ${LLVM_DEFINITIONS})
target_compile_options(hadron_core PRIVATE ${HADRON_FLAGS})
target_link_libraries(hadron_core PUBLIC ${llvm_libs})

add_executable(hadronc "src/main.cpp")
target_link_libraries(hadronc PRIVATE hadron_core ${llvm_libs})
target_compile_options(hadronc PRIVATE ${HADRON_FLAGS})

if (BUILD_TESTS)
    include(FetchContent)
    FetchContent_Declare(googletest URL https://github.com/google/googletest/archive/refs/tags/v1.14.0.zip)
    set(INSTALL_GTEST OFF CACHE BOOL "" FORCE)
    set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
    FetchContent_MakeAvailable(googletest)

    if(TARGET gtest)
        target_compile_options(gtest PRIVATE -w)
    endif()
    if(TARGET gtest_main)
        target_compile_options(gtest_main PRIVATE -w)
    endif()

    include_directories(SYSTEM ${googletest_SOURCE_DIR}/googletest/include)

    enable_testing()
    file(GLOB_RECURSE TEST_SOURCES "test/*.cpp")
    add_executable(hadron_tests ${TEST_SOURCES})
    target_link_libraries(hadron_tests PRIVATE hadron_core GTest::gtest_main ${llvm_libs})
    target_compile_options(hadron_tests PRIVATE ${HADRON_FLAGS})
    include(GoogleTest)
    gtest_discover_tests(hadron_tests)
endif ()
