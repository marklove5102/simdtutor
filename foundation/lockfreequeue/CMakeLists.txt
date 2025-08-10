cmake_minimum_required(VERSION 3.10)

set(CMAKE_CXX_STANDARD 20)

project(main CXX)

add_executable(main main.cpp)

find_package(fmt REQUIRED)
target_link_libraries(main fmt::fmt)
