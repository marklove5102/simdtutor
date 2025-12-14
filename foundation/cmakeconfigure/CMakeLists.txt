cmake_minimum_required(VERSION 3.18)
set(CMAKE_CXX_STANDARD 20)
project(main LANGUAGES CXX)

option(USE_BABY "Enable baby mode" OFF)
option(USE_THREE "Enable three body" OFF)

add_executable(main main.cpp foo.cpp)

target_include_directories(main PRIVATE ${CMAKE_BINARY_DIR})

configure_file(config.h.in config.h)
