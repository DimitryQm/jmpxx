# SPDX-License-Identifier: MIT
# The public examples. Each builds as its own target, so a broken example fails the
# build, and each is the worked code the cookbook in docs/ is drawn from. They consume
# jmpxx through the jmpxx::jmpxx target, the way a program does. Their run-tests register
# when the test tiers are on (JMPXX_EXAMPLE_TESTS), set by the top-level build.
set(JMPXX_EX_WARN)
if(NOT MSVC)
  set(JMPXX_EX_WARN -Wall -Wextra)
endif()

function(jmpxx_example name std)
  add_executable(example_${name} ${name}.cpp)
  target_link_libraries(example_${name} PRIVATE jmpxx::jmpxx)
  target_compile_features(example_${name} PRIVATE cxx_std_${std})
  target_compile_options(example_${name} PRIVATE ${JMPXX_EX_WARN} ${ARGN})
  if(JMPXX_EXAMPLE_TESTS)
    add_test(NAME example.${name} COMMAND example_${name})
  endif()
endfunction()

# Portable, exception-free examples in the strict configuration. reflection uses the
# hand-written fallback on a C++20 toolchain and the reflection path on a C++26 one.
set(JMPXX_EX_NICHE)
if(NOT MSVC)
  set(JMPXX_EX_NICHE -fno-exceptions -fno-rtti)
endif()
jmpxx_example(result_basics 20 ${JMPXX_EX_NICHE})
jmpxx_example(policies 20 ${JMPXX_EX_NICHE})
jmpxx_example(reflection 20 ${JMPXX_EX_NICHE})

# Diagnostics: built with the layer on so the example shows the debug origin and chain
# regardless of build type.
jmpxx_example(diagnostics 20 ${JMPXX_EX_NICHE})
target_compile_definitions(example_diagnostics PRIVATE JMPXX_DIAGNOSTICS_ENABLED=1)

# Interop: C++23 for the std::expected bridge, still exception-free.
jmpxx_example(interop 23 ${JMPXX_EX_NICHE})

# The unwind arm requires unwind tables, so it is built with exceptions enabled, unlike
# the portable examples above.
jmpxx_example(unwind_arm 20)
