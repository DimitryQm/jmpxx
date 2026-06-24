// SPDX-License-Identifier: MIT
// Static tier for the representation policies. Building this file is the test. It is
// built twice, once in each personality, so the assertions check both that every
// policy keeps the transport trivially copyable and that the rich policy's release
// representation is exactly the minimal policy's while its debug representation
// carries the out-of-band handle.
#include "jmpxx/core.hpp"
#include "jmpxx/diagnostics.hpp"
#include "jmpxx/erased.hpp"

#include <type_traits>

using namespace jmpxx;

// Every policy keeps the transport register-passable and memcpy-able, in every
// build, because the diagnostic context never enters the error.
static_assert(std::is_trivially_copyable_v<rich_error>);
static_assert(std::is_trivially_copyable_v<erased_error>);
static_assert(std::is_trivially_copyable_v<result<int, rich_error>>);
static_assert(std::is_trivially_copyable_v<result<int, erased_error>>);
static_assert(std::is_trivially_copyable_v<result<void, rich_error>>);

// The type-erased policy is two scalars: a value and a domain pointer.
static_assert(sizeof(erased_error) == sizeof(int) + sizeof(void*) ||
              sizeof(erased_error) == 2 * sizeof(void*));

#if JMPXX_DIAGNOSTICS_ENABLED
// Debug personality: the rich error carries one extra handle word beyond the
// minimal error, the only place the diagnostic layer touches the representation.
static_assert(sizeof(rich_error) > sizeof(error),
              "the debug rich error carries an out-of-band handle");
#else
// Release personality: the rich error is exactly the minimal error, which is the
// layout half of the rich-free-in-release guarantee. The codegen half is the
// release-diff gate.
static_assert(sizeof(rich_error) == sizeof(error),
              "the release rich error must equal the minimal error");
static_assert(alignof(rich_error) == alignof(error));
static_assert(sizeof(result<int, rich_error>) == sizeof(result<int, error>),
              "the release rich transport must equal the minimal transport");
static_assert(std::is_standard_layout_v<rich_error>);
#endif

int main() { return 0; }
