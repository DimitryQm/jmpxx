// SPDX-License-Identifier: MIT
// The kernel entry points the benchmark harness drives. Each mechanism compiles to
// its own translation unit and exposes one C-linkage entry that runs the chain in
// spec.hpp at the requested depth and lands: it returns the produced value on success
// and the negated failure code on failure, so the harness drives every mechanism
// through one uniform signature and sinks one int per call. The depth argument selects
// the deep cell of the sweep when it is at least jxb_depth_deep and the canonical
// depth otherwise, both compile-time instantiations so no runtime loop changes the
// chain's character. C linkage keeps the symbols stable for the size and codegen
// probes that inspect them by name.
#ifndef JMPXX_BENCH_KERNELS_HPP
#define JMPXX_BENCH_KERNELS_HPP

extern "C" {

// The mechanisms always present: jmpxx, the hand-written branch jmpxx claims to
// match, std::expected with hand-threaded checks, a std::error_code threaded by
// hand, and language exceptions. Each is built from the one kernel spec.
int jmpxx_chain(int x, int depth);
int handwritten_chain(int x, int depth);
int expected_chain(int x, int depth);
int errorcode_chain(int x, int depth);
int exceptions_chain(int x, int depth);

// The third-party incumbents, present only where the suite was configured with the
// library available. The harness queries the JXB_HAVE_* flags below before driving them.
int outcome_chain(int x, int depth);
int leaf_chain(int x, int depth);
int tlexpected_chain(int x, int depth);

// The deliberately slowed jmpxx kernel that gives the perf gate its teeth.
int jmpxx_slow_chain(int x, int depth);

}  // extern "C"

// Compile-time presence flags for the optional incumbents, set by the build when the
// corresponding kernel translation unit is included. A mechanism whose flag is 0 is
// reported as unavailable rather than driven, so the suite runs wherever it is built
// and reports honestly which incumbents it could measure.
#ifndef JXB_HAVE_OUTCOME
#define JXB_HAVE_OUTCOME 0
#endif
#ifndef JXB_HAVE_LEAF
#define JXB_HAVE_LEAF 0
#endif
#ifndef JXB_HAVE_TLEXPECTED
#define JXB_HAVE_TLEXPECTED 0
#endif

#endif  // JMPXX_BENCH_KERNELS_HPP
