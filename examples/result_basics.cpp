// SPDX-License-Identifier: MIT
// The starting point: a function that can fail returns result<T>, a caller propagates
// the failure with JMPXX_TRY, and one landing point handles it. There is no per-call
// `if (!r) return r;`, and the intermediate function carries no error type it does not
// produce itself.
#include <jmpxx/core.hpp>

#include <cstdio>
#include <string_view>

using jmpxx::error;
using jmpxx::fail;
using jmpxx::result;

// A leaf operation that fails on bad input. The error carries a code and a coarse
// domain tag, both plain integers.
enum domain { dom_parse = 1 };
enum code { empty_input = 1, not_a_number = 2 };

result<int> parse_int(std::string_view text) {
  if (text.empty()) return fail(error(empty_input, dom_parse));
  int value = 0;
  for (char c : text) {
    if (c < '0' || c > '9') return fail(error(not_a_number, dom_parse));
    value = value * 10 + (c - '0');
  }
  return value;  // a bare value converts into a successful result
}

// A caller that does not handle the failure itself: JMPXX_TRY binds the value on
// success and returns the failure to its own caller otherwise.
result<int> doubled(std::string_view text) {
  JMPXX_TRY(value, parse_int(text));
  return value * 2;
}

int main() {
  // The single place that handles failure for this chain.
  for (std::string_view in : {"21", "", "12x"}) {
    result<int> r = doubled(in);
    if (r)
      std::printf("doubled(\"%.*s\") = %d\n", (int)in.size(), in.data(), r.value());
    else
      std::printf("doubled(\"%.*s\") failed: code=%d domain=%d\n", (int)in.size(),
                  in.data(), r.error().code, r.error().domain);
  }

  // value_or supplies a default without branching at the call site.
  std::printf("parse_int(\"x\").value_or(-1) = %d\n", parse_int("x").value_or(-1));
  return 0;
}
