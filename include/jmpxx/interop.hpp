// SPDX-License-Identifier: MIT
// The interoperability bridges, gathered for a hosted consumer.
//
// This umbrella pulls in every bridge: the std::expected conversion, the
// std::error_code adoption, the opt-in exception boundary, and the optional-like
// adapters. It is a convenience for a hosted program that wants the whole interop
// surface in one include; because the error_code bridge needs <system_error> and
// the expected bridge needs <expected>, including this header is a hosted choice.
//
// A freestanding or minimal consumer includes only the specific bridge it needs.
// jmpxx/interop/expected.hpp and jmpxx/interop/adapt.hpp are freestanding-capable;
// jmpxx/interop/error_code.hpp and jmpxx/interop/exception.hpp are hosted, and the
// latter is present only where exceptions are enabled. Each bridge defines a macro
// (JMPXX_INTEROP_HAS_EXPECTED, JMPXX_INTEROP_HAS_EXCEPTION_BRIDGE) a consumer can
// test to learn whether it is available in the current configuration.
#ifndef JMPXX_INTEROP_HPP
#define JMPXX_INTEROP_HPP

#include "jmpxx/interop/adapt.hpp"
#include "jmpxx/interop/error_code.hpp"
#include "jmpxx/interop/exception.hpp"
#include "jmpxx/interop/expected.hpp"

#endif  // JMPXX_INTEROP_HPP
