cmake_minimum_required(VERSION 3.10)

project(timestamp CXX)

add_executable(timestamp timestamp.cpp)

find_package(fmt REQUIRED)
target_link_libraries(timestamp fmt::fmt)

find_package(OpenMP REQUIRED)
target_link_libraries(timestamp OpenMP::OpenMP_CXX) # -fopenmp

find_package(TBB REQUIRED)
target_link_libraries(timestamp TBB::tbb)

target_compile_options(timestamp PRIVATE -march=native)
