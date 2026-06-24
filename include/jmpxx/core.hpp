// SPDX-License-Identifier: MIT
// The minimal, freestanding core of jmpxx: the value-or-error transport, the
// minimal error representation, and single-construct propagation. Including this
// header pulls in nothing outside the freestanding subset of the standard
// library, so it is usable where there is no heap, no exceptions, and no RTTI.
//
// Hosted extensions (diagnostics, interop bridges, and the experimental
// non-local escape) live under separate headers and are never reached by
// including this one.
#ifndef JMPXX_CORE_HPP
#define JMPXX_CORE_HPP

#include "jmpxx/core/config.hpp"
#include "jmpxx/core/error.hpp"
#include "jmpxx/core/propagation.hpp"
#include "jmpxx/core/transport.hpp"

#endif  // JMPXX_CORE_HPP
