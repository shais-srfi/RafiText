cmake_minimum_required(VERSION 3.16)
project(RafiText LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_executable(RafiText
    src/main.cpp
    src/terminal.cpp
)

target_include_directories(RafiText PRIVATE src)

target_compile_options(RafiText PRIVATE -Wall -Wextra -Wpedantic)