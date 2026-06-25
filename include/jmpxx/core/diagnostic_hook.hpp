// SPDX-License-Identifier: MIT
// The propagation diagnostic hook. Single-construct propagation calls
// note_propagation on the failure path so a hosted diagnostic layer can record a
// causal hop as a failure travels toward its landing boundary. The core ships
// only the no-op default here: it compiles to nothing, pulls in no hosted header,
// and is selected by overload resolution for any error representation that
// carries no out-of-band context, so the freestanding minimal core stays pure and
// the zero-overhead codegen is unchanged. The rich policy (jmpxx/diagnostics.hpp)
// adds an overload for its error type that captures the propagation-site location.
//
// The hook is reached only through the propagation macros, and only when
// JMPXX_DIAGNOSTICS_ENABLED is on; the macros expand it to nothing otherwise. It
// lives in detail because it is an internal hook, not a caller-facing entry point.
#ifndef JMPXX_CORE_DIAGNOSTIC_HOOK_HPP
#define JMPXX_CORE_DIAGNOSTIC_HOOK_HPP

#include "jmpxx/core/config.hpp"

namespace jmpxx {
namespace detail {

// An error representation with no out-of-band context records no hop. A more
// specialized non-template overload, contributed by a richer policy, wins over
// this template for that policy's error type.
template <class E>
JMPXX_ALWAYS_INLINE constexpr void note_propagation(const E&) noexcept {}

}  // namespace detail
}  // namespace jmpxx

#endif  // JMPXX_CORE_DIAGNOSTIC_HOOK_HPP
