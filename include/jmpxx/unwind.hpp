// SPDX-License-Identifier: MIT
// The experimental non-local unwind arm: opt-in entry point.
//
// This header is the only door to the arm. Including jmpxx/core.hpp or any portable
// header does not pull it in, so the arm's cost and caveats stay off every user who
// does not opt in by naming this header.
//
// A failure ejected deep in a call chain returns to a single landing boundary while the
// platform unwinder runs the destructor of every automatic object on the path, so the
// intermediate frames carry no propagation construct in their source, which the portable
// JMPXX_TRY cannot offer. The arm is experimental and platform-specific.
//
// The arm runs destructors only where the compiler emits unwind cleanup tables, which it
// does when exceptions are enabled. A translation unit that includes this header with
// exceptions disabled is refused below at compile time rather than producing an arm that
// skips destructors silently. The refusal covers only this translation unit; every frame
// on the escape path must also be compiled with cleanup tables.
//
// docs/reference/unwind.md states the rest of the contract: the unwind-tables
// requirement, the sad-path cost, the catch-all and noexcept constraints on the escape
// path, and the per-ABI differences, notably WebAssembly where the virtual machine
// performs the unwinding.
#ifndef JMPXX_UNWIND_HPP
#define JMPXX_UNWIND_HPP

#include "jmpxx/core/config.hpp"

#if !JMPXX_HAS_EXCEPTIONS
#error "jmpxx/unwind.hpp requires C++ exceptions enabled for unwind cleanup tables; use jmpxx/core.hpp for the portable no-exceptions path."
#endif

#include "jmpxx/unwind/escape.hpp"

#endif  // JMPXX_UNWIND_HPP
