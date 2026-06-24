// SPDX-License-Identifier: MIT
// Fenced stack-trace capture for the diagnostic layer.
//
// Capturing a return-address trace is platform-specific, so the whole mechanism
// lives behind this one boundary and nothing outside it names a platform unwinder.
// The capture records raw return addresses into a fixed buffer; it takes no lock,
// allocates nothing, and does not symbolize. Turning addresses into file and
// function names is a separate, offline concern, because symbolization is the part
// that allocates and is slow.
//
// The capability is optional and off unless JMPXX_STACKTRACE is set, and it is
// reached only through the diagnostic layer, which is itself debug-only. When the
// switch is off, capture() is a no-op that returns an empty trace and no platform
// header is included. A platform without a fenced capturer here returns an empty
// trace rather than guessing, so the absence is honest rather than silent.
#ifndef JMPXX_PLATFORM_TRACE_HPP
#define JMPXX_PLATFORM_TRACE_HPP

#include "jmpxx/core/config.hpp"

#include <cstddef>

#if JMPXX_STACKTRACE
// The capturer is selected here, once, by platform. glibc's execinfo backtrace is
// used on Linux; it needs frame pointers to be reliable, which a debug build keeps.
// Other platforms route to the empty capture below until a fenced capturer for
// them is added, rather than emitting an unverified trace.
#if defined(__GLIBC__) && defined(__has_include)
#if __has_include(<execinfo.h>)
#include <execinfo.h>
#define JMPXX_TRACE_BACKEND_EXECINFO 1
#endif
#endif
#endif  // JMPXX_STACKTRACE

namespace jmpxx {
namespace platform {

// A captured trace: up to max_frames return addresses, innermost first. Trivially
// copyable so a diagnostic record can hold one by value with no allocation.
struct trace {
  static constexpr int max_frames = 24;
  void* frames[max_frames];
  int count;

  [[nodiscard]] constexpr bool empty() const noexcept { return count == 0; }
};

// Capture the current call stack, skipping the innermost `skip` frames so the
// diagnostic machinery's own frames do not appear. Returns an empty trace when the
// capability is off or no fenced capturer exists for this platform. Never throws,
// never allocates, takes no lock.
[[nodiscard]] inline trace capture(int skip) noexcept {
  trace t{};
  t.count = 0;
#if defined(JMPXX_TRACE_BACKEND_EXECINFO)
  void* raw[trace::max_frames + 8];
  int want = trace::max_frames + (skip > 0 ? skip : 0);
  if (want > static_cast<int>(sizeof(raw) / sizeof(raw[0])))
    want = static_cast<int>(sizeof(raw) / sizeof(raw[0]));
  int n = ::backtrace(raw, want);
  int first = skip > 0 ? skip : 0;
  for (int i = first; i < n && t.count < trace::max_frames; ++i)
    t.frames[t.count++] = raw[i];
#else
  (void)skip;
#endif
  return t;
}

// True when a fenced capturer is compiled in, so a caller or a report can state
// plainly whether a trace is available on this build rather than implying one.
[[nodiscard]] constexpr bool trace_available() noexcept {
#if defined(JMPXX_TRACE_BACKEND_EXECINFO)
  return true;
#else
  return false;
#endif
}

}  // namespace platform
}  // namespace jmpxx

#endif  // JMPXX_PLATFORM_TRACE_HPP
