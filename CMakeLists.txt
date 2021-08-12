cmake_minimum_required(VERSION 3.9)

project(softwarert)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(softwarert "source/main.cpp")
target_include_directories(softwarert PUBLIC "include")
