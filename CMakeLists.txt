cmake_minimum_required(VERSION 3.9)

project(softwarert)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(MSVC)
    set(CMAKE_CXX_FLAGS_RELEASE "/arch:AVX2 /O2 /Ob2 /Oi /MP /Ot /GL")
endif()

add_executable(softwarert "source/main.cpp")
target_include_directories(softwarert PUBLIC "include")
