# SPDX-License-Identifier: MIT
# A consumer that uses the single-header amalgamation: it adds single_include to the
# include path and includes <jmpxx.hpp>, with no other jmpxx wiring. JMPXX_ROOT points
# at the jmpxx checkout.
cmake_minimum_required(VERSION 3.20)
project(jmpxx_amalgamation_consumer CXX)

if(NOT DEFINED JMPXX_ROOT)
  message(FATAL_ERROR "set -DJMPXX_ROOT=<path to the jmpxx checkout>")
endif()

add_executable(consumer main.cpp)
target_include_directories(consumer PRIVATE ${JMPXX_ROOT}/single_include)
target_compile_features(consumer PRIVATE cxx_std_23)

enable_testing()
add_test(NAME consumer COMMAND consumer)
