# SPDX-License-Identifier: MIT
cmake_minimum_required(VERSION 3.20)
project(jmpxx_test_package CXX)

find_package(jmpxx CONFIG REQUIRED)

add_executable(example example.cpp)
target_link_libraries(example PRIVATE jmpxx::jmpxx)
# jmpxx requires C++20; a consumer declares its own standard.
target_compile_features(example PRIVATE cxx_std_20)
