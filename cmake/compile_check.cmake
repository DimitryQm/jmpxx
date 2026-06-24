# SPDX-License-Identifier: MIT
# Compile SRC with CXX and assert the outcome matches EXPECT (fail|succeed).
# The compile-fail tier uses this to prove that misuse does not compile and that
# the corresponding correct use does. -Werror turns the [[nodiscard]] diagnostic
# into a hard error so a discarded result fails the build.
# Required -D arguments: CXX, STD, INC, SRC, EXPECT. Optional: FLAGS.
separate_arguments(EXTRA UNIX_COMMAND "${FLAGS}")
execute_process(
  COMMAND ${CXX} -std=${STD} -fsyntax-only -Werror -I ${INC} ${EXTRA} ${SRC}
  RESULT_VARIABLE rc
  OUTPUT_VARIABLE out
  ERROR_VARIABLE err)
if(EXPECT STREQUAL "fail")
  if(rc EQUAL 0)
    message(FATAL_ERROR "compile-fail case unexpectedly compiled: ${SRC}")
  endif()
else()
  if(NOT rc EQUAL 0)
    message(FATAL_ERROR "expected-success case failed to compile: ${SRC}\n${err}")
  endif()
endif()
