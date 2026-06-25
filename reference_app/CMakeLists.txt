# SPDX-License-Identifier: MIT
# The cumulative reference application. Built standalone, it consumes
# jmpxx through find_package (a real packaging channel) and pulls two third-party
# libraries through FetchContent: toml++ for the TOML configuration and glaze for the
# JSON access policy. When configured inside the jmpxx tree it reuses the in-tree
# target instead.
#
# The same source is built three times: minimal, rich diagnostics, and rich
# diagnostics under extensive hardening with trace capture. Selecting a policy or
# a hardening posture changes neither the call sites nor the behavior, only what a
# failure can tell you and which contract violations are fail-fast checked. All
# builds adopt glaze's std::expected through the interop bridge, so the application
# exercises the std::expected, type-erased, and minimal-or-rich representations
# together over one transport.
cmake_minimum_required(VERSION 3.20)
project(jmpxx_reference_app CXX)

if(NOT TARGET jmpxx::jmpxx)
  find_package(jmpxx REQUIRED)
endif()

include(FetchContent)
FetchContent_Declare(tomlplusplus
  GIT_REPOSITORY https://github.com/marzer/tomlplusplus.git
  GIT_TAG v3.4.0)
FetchContent_Declare(glaze
  GIT_REPOSITORY https://github.com/stephenberry/glaze.git
  GIT_TAG v7.8.3)
FetchContent_MakeAvailable(tomlplusplus glaze)

# Three builds from one source. The minimal build uses the default error policy; the
# rich build defines APP_POLICY_RICH and turns the diagnostic layer on; the hardened
# build also enables trace capture and extensive portable hardening.
add_executable(config_validate_minimal src/config_validate.cpp)
add_executable(config_validate_rich src/config_validate.cpp)
add_executable(config_validate_hardened src/config_validate.cpp)
target_compile_definitions(config_validate_rich PRIVATE
  APP_POLICY_RICH JMPXX_DIAGNOSTICS_ENABLED=1)
target_compile_definitions(config_validate_hardened PRIVATE
  APP_POLICY_RICH JMPXX_DIAGNOSTICS_ENABLED=1 JMPXX_STACKTRACE=1
  JMPXX_HARDENING_MODE=JMPXX_HARDENING_EXTENSIVE)

foreach(app config_validate_minimal config_validate_rich config_validate_hardened)
  target_link_libraries(${app} PRIVATE
    jmpxx::jmpxx tomlplusplus::tomlplusplus glaze::glaze)
  # C++23 for the std::expected bridge the glaze policy boundary uses, and for glaze.
  target_compile_features(${app} PRIVATE cxx_std_23)
  # Build the strict configuration the library targets: exceptions and RTTI disabled, toml++ in its
  # no-exceptions mode. glaze returns std::expected and runs with exceptions off.
  target_compile_definitions(${app} PRIVATE TOML_EXCEPTIONS=0)
  if(NOT MSVC)
    target_compile_options(${app} PRIVATE -fno-exceptions -fno-rtti)
  endif()
endforeach()

# config_resolve opts into the experimental unwind arm for a recursive reference
# resolver. The arm runs destructors during a forced unwind only where exception cleanup
# tables are present, so this program, unlike config_validate, is built with exceptions
# enabled. toml++ is built in its no-exceptions mode regardless, so a parse error is a
# value rather than a throw; only the arm's own translation units need the tables.
add_executable(config_resolve src/config_resolve.cpp)
target_link_libraries(config_resolve PRIVATE jmpxx::jmpxx tomlplusplus::tomlplusplus)
target_compile_features(config_resolve PRIVATE cxx_std_23)
target_compile_definitions(config_resolve PRIVATE TOML_EXCEPTIONS=0)

# config_bench is the application-level head-to-head: the application's [server] validation implemented
# with jmpxx and again with std::expected, timed on a real loaded config. Built optimized in the
# strict configuration so the measurement reflects what ships.
add_executable(config_bench src/config_bench.cpp)
target_link_libraries(config_bench PRIVATE jmpxx::jmpxx tomlplusplus::tomlplusplus)
target_compile_features(config_bench PRIVATE cxx_std_23)
target_compile_definitions(config_bench PRIVATE TOML_EXCEPTIONS=0)
if(NOT MSVC)
  target_compile_options(config_bench PRIVATE -O2 -fno-exceptions -fno-rtti)
endif()

enable_testing()
set(CFG ${CMAKE_CURRENT_SOURCE_DIR}/configs)
foreach(app config_validate_minimal config_validate_rich config_validate_hardened)
  add_test(NAME ${app}.valid COMMAND ${app} ${CFG}/valid.toml)
  add_test(NAME ${app}.invalid_port COMMAND ${app} ${CFG}/invalid_port.toml)
  add_test(NAME ${app}.missing_host COMMAND ${app} ${CFG}/missing_host.toml)
  add_test(NAME ${app}.invalid_limits COMMAND ${app} ${CFG}/invalid_limits.toml)
  add_test(NAME ${app}.invalid_policy COMMAND ${app} ${CFG}/invalid_policy.toml)
  set_tests_properties(
    ${app}.invalid_port ${app}.missing_host ${app}.invalid_limits
    ${app}.invalid_policy
    PROPERTIES WILL_FAIL TRUE)
endforeach()

# The resolver resolves a valid config, and escapes-and-reports on a reference cycle and
# a missing reference, with the resolver staying usable across the escape.
add_test(NAME config_resolve.valid COMMAND config_resolve ${CFG}/values_valid.toml)
add_test(NAME config_resolve.cycle COMMAND config_resolve ${CFG}/values_cycle.toml)
add_test(NAME config_resolve.missing COMMAND config_resolve ${CFG}/values_missing.toml)
set_tests_properties(config_resolve.cycle config_resolve.missing
  PROPERTIES WILL_FAIL TRUE)

# The head-to-head runs as a smoke test; its measured numbers are read from its output, not
# gated, because wall-clock latency is machine-specific.
add_test(NAME config_bench.runs COMMAND config_bench ${CFG}/valid.toml)
set_tests_properties(config_bench.runs PROPERTIES TIMEOUT 120)
