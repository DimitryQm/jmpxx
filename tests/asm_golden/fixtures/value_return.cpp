// SPDX-License-Identifier: MIT
// Fixtures: constructing and returning a value or an error transport for a
// trivial payload. The transport is eight bytes and returned in a register.
#include "jmpxx/core.hpp"

using namespace jmpxx;

extern "C" result<int, int> jx_value_return(int x) { return x; }
extern "C" result<int, int> jx_error_return(int c) { return fail(c); }
