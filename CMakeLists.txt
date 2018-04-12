# CMake version check
cmake_minimum_required (VERSION 2.8)

# project name
project (ProtobufAutoGenerator)

# C++11
SET(CMAKE_C_COMPILER g++)
add_compile_options(-std=c++11)

# build target
add_executable(ProtobufAutoGenerator main.cc)
