<!-- SPDX-License-Identifier: MIT -->
# Changelog

All notable user-facing changes to jmpxx are recorded here. The format follows
Keep a Changelog, and the project follows semantic versioning. While the project
is pre-1.0, the public surface may change between minor versions, and each change
is recorded here with its migration impact.

## [Unreleased]

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
  abstraction.
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
- A packaging channel through which a separate consumer builds against jmpxx.

[Unreleased]: https://github.com/DimitryQm/jmpxx
