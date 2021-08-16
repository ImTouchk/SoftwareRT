cmake_minimum_required(VERSION 3.9)

project(softwarert)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(NOT CMAKE_BUILD_TYPE EQUAL "DEBUG")
    if(CMAKE_SIZEOF_VOID_P EQUAL 8 AND MSVC)
        add_compile_options(/arch:AVX2 /O2)
        message("Using /arch:AVX2 /O2 compiler options.")
    endif()
endif()

add_executable(softwarert "source/main.cpp")
target_include_directories(softwarert PUBLIC "include")
