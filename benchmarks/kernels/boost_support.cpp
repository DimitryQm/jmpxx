// SPDX-License-Identifier: MIT
// Boost requires the program to define boost::throw_exception when exceptions are disabled, which
// is the niche the benchmark builds in. The Boost.Outcome and Boost.LEAF kernels reference it from
// an error path, and whether the optimizer keeps that reference is not portable, so this one
// translation unit supplies the definition unconditionally when Boost is present. It aborts: a
// throw from inside Boost during the benchmark is a logic error in the benchmark, not a recoverable
// failure, and the kernels are written never to reach it. Defined here, in a single unit, so the
// Outcome and LEAF kernels both resolve to one definition with no duplicate-symbol conflict.
#if defined(__has_include) && __has_include(<boost/throw_exception.hpp>)

#include <boost/throw_exception.hpp>

#if defined(BOOST_NO_EXCEPTIONS)

#include <cstdlib>
#include <exception>

namespace boost {

void throw_exception(std::exception const&) { std::abort(); }
void throw_exception(std::exception const&, boost::source_location const&) { std::abort(); }

}  // namespace boost

#endif  // BOOST_NO_EXCEPTIONS
#endif  // __has_include(<boost/throw_exception.hpp>)
