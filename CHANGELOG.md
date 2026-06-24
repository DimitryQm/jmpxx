<!-- SPDX-License-Identifier: MIT -->
# Changelog

All notable user-facing changes to jmpxx are recorded here. The format follows
Keep a Changelog, and the project follows semantic versioning. While the project
is pre-1.0, the public surface may change between minor versions, and each change
is recorded here with its migration impact.

## [Unreleased]

## [0.1.1] - 2026-06-24

### Added
- Monadic combinators on `result<T, E>`: `and_then`, `or_else`, `transform`, and
  `transform_error`, matching `std::expected` (P2505) across the value and error states,
  with lvalue, const, and rvalue overloads and `void` value support. They stay in the
  freestanding core: the callable is invoked directly, so they pull in no `<functional>`.

### Changed
- The experimental unwind arm reads an ejected error through `__builtin_bit_cast` rather
  than a `reinterpret_cast`, which is defined by construction for the trivially copyable
  error type and removes an object-lifetime subtlety. The builtin is used in place of
  `std::bit_cast` so the arm needs no `<bit>`, which an older WebAssembly toolchain's
  standard library does not provide.

### Fixed
- The `std::error_code` interop documentation no longer implies an implicit conversion:
  jmpxx does not specialize `std::is_error_code_enum`, so `std::error_code ec = err;` does
  not compile, and a `jmpxx::error` is converted explicitly with `to_error_code` or
  `make_error_code`.
- The `erased_error(code, domain_tag)` documentation now describes its fold honestly: it
  round-trips a code and a tag that each fit in 16 bits, and is otherwise coarse and can
  collide, so a boundary that needs full fidelity names a domain descriptor.

## [0.1.0] - 2026-06-24

The first release. It delivers the value-or-error transport, single-construct
propagation to a typed landing, the three representation policies over one transport,
the debug-only diagnostic layer that is free in release, the interop bridges, the
experimental opt-in unwind arm across four ABIs, the optional reflection layer, the
verification harness with its gates, the lint companion, and distribution through every
common channel, each property backed by a gate checked against a known-bad input.

### Added
- The value-or-failure transport: a return type that carries either a produced
  value or a failure, holds exactly one of the two across construction,
  assignment, move, and copy, and never presents an observable valueless state.
  A trivially copyable value and error yield a trivially copyable transport, the
  transport is usable in constant evaluation, and a failure cannot be discarded
  without a compile-time diagnostic.
- The minimal representation policy: a freestanding, heap-free error
  representation that collapses toward a bare code and compiles with exceptions
  and RTTI disabled.
- The rich representation policy and the diagnostic layer: an error that carries
  a failure's origin and the causal chain it accumulates as it propagates, held
  out of band. The context is present in a debug build and compiled out of a
  release build, where the rich policy's generated code is identical to the
  minimal policy's, proven by a release codegen diff with teeth. The store is
  per-thread and allocates nothing, and a landing scope bounds the lifetime of
  the context it owns. Reading the context uses `jmpxx::diagnostic::inspect` and
  `jmpxx::diagnostic::print`.
- The type-erased representation policy: a domain-tagged error usable at a
  component boundary by code that does not know the originating error category,
  with no RTTI and no heap allocation.
- One transport and one set of propagation sites serve all three policies, so a
  program selects a policy at its error type with no change at the call sites. An
  optional, off-by-default stack-trace capture is fenced behind the platform
  abstraction. Each policy header is self-contained: including `jmpxx/diagnostics.hpp`
  or `jmpxx/erased.hpp` pulls in the core transport, so a policy is usable from its
  own header.
- Single-construct propagation: a failure propagates from arbitrary call depth to
  a single typed landing boundary with one source-level construct, running every
  destructor on the path exactly once, with the cost of each propagation level
  stated and proven.
- Interoperability bridges so a codebase already written against the standard
  error vocabularies adopts jmpxx at a seam. The `std::expected` bridge converts
  `result` to and from `std::expected` losslessly and stays usable in a
  freestanding, no-exceptions build. The `std::error_code` bridge carries a foreign
  code verbatim and exposes a jmpxx error as a code in a jmpxx category that
  round-trips. The opt-in exception bridge converts between a failure and a thrown
  exception where exceptions are enabled and is absent otherwise. The `from_optional`
  and `from_condition` adapters collapse the optional-like and boolean-plus-value
  boundaries of other libraries into one call. See
  [docs/reference/interop.md](docs/reference/interop.md).
- Validation of untrusted input at a bridge boundary, so malformed input yields a
  defined failure rather than undefined behavior, checked by a behavioral tier under
  the sanitizers and a bounded libFuzzer tier.
- An experimental, opt-in non-local unwind arm that returns a failure from arbitrary
  call depth to a single landing boundary while the platform unwinder runs every
  destructor on the path, so the intermediate frames carry no propagation construct.
  It requires unwind cleanup tables and refuses a no-exceptions configuration at
  compile time, is reached only through `jmpxx/unwind.hpp`, and is never on a default
  path. Its sad path is measured and gated for determinism. It runs on the Itanium and
  DWARF interface, the ARM exception-handling ABI including a bare-metal target,
  WebAssembly, and the Windows structured-exception ABI. The catch-all, `noexcept`, and
  unwind-tables caveats are stated at its surface. See
  [docs/reference/unwind.md](docs/reference/unwind.md).
- A platform abstraction that fences every target-specific and ABI-specific
  construct behind one unit and exposes the build's compiler, operating system, and
  architecture, enforced by a static scan. See
  [docs/reference/platform.md](docs/reference/platform.md).
- `jmpxx-verify`: the verification harness that compiles fixtures, emits and
  diffs optimized machine code against committed goldens, and reports transport
  size and allocation counts in human-readable and machine-readable form.
- The distribution benchmark suite, `jmpxx-bench`, which drives one error-propagation
  kernel implemented identically for jmpxx, a hand-written branch, `std::expected`,
  a threaded `std::error_code`, language exceptions, Boost.Outcome, Boost.LEAF, and
  tl::expected, and reports each mechanism's per-call latency by median and high
  percentile across a failure-ratio and a depth sweep. The honest comparison, which
  states where jmpxx loses as plainly as where it wins, is in
  [docs/comparison.md](docs/comparison.md).
- The binary-size-delta, compile-cost, and perf gates as continuous gates with teeth.
  The size gate proves the library adds zero bytes over a hand-written branch in the
  ship configuration; the compile-cost gate bounds the template-instantiation count,
  a deterministic metric; the perf gate bounds jmpxx against a co-measured baseline,
  and a deterministic callgrind instruction count proves the happy path executes the
  same instructions as the hand-written branch. Each gate fails a known-bad input. See
  [docs/reference/verification.md](docs/reference/verification.md).
- The companion check set, `jmpxx-lint`, an out-of-tree Clang tool that flags a
  discarded result, a value taken through a narrow accessor without a success check,
  and a hand-written failure forward that `JMPXX_TRY` would express. It is a developer
  tool, never a runtime or include dependency. See
  [docs/reference/lint.md](docs/reference/lint.md).
- The optional reflection forward layer, `jmpxx/reflect.hpp`, which derives error
  metadata from an enum: the enumerator name from a value and back, the static set of
  failures an error enum declares, and an `error_domain` whose per-value message is the
  enumerator name. Where C++26 static reflection is available it uses reflection. Where
  it is not, a hand-written C++20 path parses the compiler signature, and the two
  produce identical results. Nothing in the core requires it, and the full core builds
  with it absent on a C++20 toolchain. See
  [docs/reference/reflect.md](docs/reference/reflect.md).
- Distribution through every common channel: a CMake package for `find_package`, the
  same definition usable through FetchContent, `add_subdirectory`, and CPM, an in-repo
  Conan recipe, a vcpkg overlay port, and two generated single-header amalgamations, a
  freestanding `jmpxx-core.hpp` and a full hosted `jmpxx.hpp`. The single headers are
  regenerated and diffed by a gate so they cannot drift, and a consumer is built through
  each channel. See [docs/reference/packaging.md](docs/reference/packaging.md).
- An ABI layout gate that freezes the observable layout of the public types, their size,
  alignment, field offsets, and trivial copyability, within a major version, with the
  experimental arm exempt until graduated. A static tier pins the layout on every cell
  and a golden gate catches an unversioned change, with teeth. See
  [docs/reference/abi.md](docs/reference/abi.md).
- Documentation for adoption: the orientation on why the audience disables exceptions,
  a task-oriented cookbook drawn from the examples, migration guides from
  `std::expected`, `std::error_code`, and exceptions, and a set of public examples that
  build and run as tests.
- An `SPDX-License-Identifier: MIT` tag on every source file; the project is
  MIT-licensed with the full text in `LICENSE`.
- `JMPXX_VERSION` and `JMPXX_VERSION_STRING`: a combined integer and a string version
  for a one-line consumer check such as `#if JMPXX_VERSION >= 100`, alongside the
  existing `JMPXX_VERSION_MAJOR`, `_MINOR`, and `_PATCH` macros.
- An acceptance sweep, `verify/acceptance.py`, that runs every test tier and every gate
  over a built tree, pairs each gate with its inverted self-test, and prints one
  machine-readable release verdict. A gate with no passing inverted self-test fails the
  verdict. See [docs/reference/verification.md](docs/reference/verification.md).
- A `find_package(jmpxx)` version file that accepts a matching major and minor version,
  so a 0.1.x release satisfies a request for 0.1 and a 0.2.0 release does not, the honest
  contract for a library whose surface may change between 0.x minor versions.

[Unreleased]: https://github.com/DimitryQm/jmpxx/compare/v0.1.1...HEAD
[0.1.1]: https://github.com/DimitryQm/jmpxx/compare/v0.1.0...v0.1.1
[0.1.0]: https://github.com/DimitryQm/jmpxx/releases/tag/v0.1.0
