# SPDX-License-Identifier: MIT
cmake_minimum_required(VERSION 3.20)
project(jmpxx
  VERSION 0.1.0
  DESCRIPTION "Non-local, RAII-correct, exception-free failure propagation for C++"
  HOMEPAGE_URL "https://github.com/DimitryQm/jmpxx"
  LANGUAGES CXX)

# Top-level detection without requiring CMake 3.21 (PROJECT_IS_TOP_LEVEL).
set(JMPXX_TOP OFF)
if(CMAKE_CURRENT_SOURCE_DIR STREQUAL CMAKE_SOURCE_DIR)
  set(JMPXX_TOP ON)
endif()

option(JMPXX_BUILD_VERIFY "Build the jmpxx-verify verification harness" ${JMPXX_TOP})
option(JMPXX_BUILD_TESTS "Build the test tiers" ${JMPXX_TOP})
option(JMPXX_BUILD_REFERENCE_APP "Build the field-use reference application" OFF)
option(JMPXX_INSTALL "Generate install and package-config rules" ON)

# The shipped library: header-only, zero runtime dependencies.
add_library(jmpxx INTERFACE)
add_library(jmpxx::jmpxx ALIAS jmpxx)
target_compile_features(jmpxx INTERFACE cxx_std_20)
target_include_directories(jmpxx INTERFACE
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
  $<INSTALL_INTERFACE:include>)

if(JMPXX_TOP)
  set(CMAKE_CXX_STANDARD_REQUIRED ON)
  set(CMAKE_CXX_EXTENSIONS OFF)

  if(JMPXX_BUILD_VERIFY AND EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/verify/CMakeLists.txt)
    add_subdirectory(verify)
  endif()
  if(JMPXX_BUILD_TESTS AND EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/tests/CMakeLists.txt)
    enable_testing()
    add_subdirectory(tests)
  endif()
  if(JMPXX_BUILD_REFERENCE_APP AND EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/reference_app/CMakeLists.txt)
    add_subdirectory(reference_app)
  endif()
endif()

if(JMPXX_INSTALL)
  include(GNUInstallDirs)
  include(CMakePackageConfigHelpers)
  set(JMPXX_CMAKE_DIR ${CMAKE_INSTALL_LIBDIR}/cmake/jmpxx)

  install(TARGETS jmpxx EXPORT jmpxxTargets)
  install(DIRECTORY include/ DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})
  install(EXPORT jmpxxTargets
    NAMESPACE jmpxx::
    FILE jmpxxTargets.cmake
    DESTINATION ${JMPXX_CMAKE_DIR})

  configure_package_config_file(cmake/jmpxxConfig.cmake.in
    ${CMAKE_CURRENT_BINARY_DIR}/jmpxxConfig.cmake
    INSTALL_DESTINATION ${JMPXX_CMAKE_DIR})
  write_basic_package_version_file(
    ${CMAKE_CURRENT_BINARY_DIR}/jmpxxConfigVersion.cmake
    VERSION ${PROJECT_VERSION}
    COMPATIBILITY SameMajorVersion
    ARCH_INDEPENDENT)
  install(FILES
    ${CMAKE_CURRENT_BINARY_DIR}/jmpxxConfig.cmake
    ${CMAKE_CURRENT_BINARY_DIR}/jmpxxConfigVersion.cmake
    DESTINATION ${JMPXX_CMAKE_DIR})
endif()
