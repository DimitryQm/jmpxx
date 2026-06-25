<!-- SPDX-License-Identifier: MIT -->
# Changelog

All notable user-facing changes to jmpxx are recorded here. The format follows
Keep a Changelog, and jmpxx follows semantic versioning. While jmpxx
is pre-1.0, the public surface may change between minor versions, and each change
is recorded here with its migration impact.

## [Unreleased]

## [0.1.2] - 2026-06-25

### Added
- Graded portable hardening through `JMPXX_HARDENING_MODE`, with
  `JMPXX_HARDENING_NONE`, `JMPXX_HARDENING_FAST`, and
  `JMPXX_HARDENING_EXTENSIVE`. The previous `JMPXX_HARDENED` switch remains
  source-compatible: `0` maps to none and any nonzero value maps to fast.
- A hardening reference page that documents the modes, their defaults, and the
  difference between recoverable errors and contract violations.
- Adversarial verification gates over the portable surface: structured fuzz,
  libFuzzer, AFL++, a differential oracle, exception-safety throw matrices, a
  configuration cross-product, MemorySanitizer with an instrumented libc++, Clang
  static analyzer coverage, source mutation testing, an adversarial region and
  branch coverage floor, and a model-based lifecycle tier. Each gate has an
  inverted known-bad case.
- `diagnostic::context` now exposes a raw `platform::trace` and `trace_captured`
  when stack-trace capture is enabled, so code using `inspect` can read the same
  trace information that `diagnostic::print` reports.

### Changed
- Extensive hardening validates the type-erased error domain before dispatch, so a
  corrupted erased boundary fails fast instead of dereferencing a null domain.
- The public platform detection surface now reports `JMPXX_HAS_RTTI`, allowing the
  configuration matrix and consumers to state RTTI status without reading compiler
  macros directly.

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

Initial release of the core transport, propagation surface, policy set, verification
tools, lint companion, packaging channels, and reference documentation. The release
freezes the documented layout subset from v0.1.0 and keeps the experimental unwind arm
explicitly opt-in.

### Added
- The value-or-failure transport, including exactly-one state, constexpr use,
  trivial-copyability preservation for trivial payloads, and `[[nodiscard]]`
  diagnostics for discarded failures.
- The minimal, rich, and type-erased error policies over one transport and one
  propagation form. The rich policy carries debug context out of band, is identical
  to the minimal policy in release, and exposes `diagnostic::inspect` and
  `diagnostic::print`; the type-erased policy gives component boundaries a domain-tagged
  error without RTTI or heap allocation.
- Single-construct propagation with `JMPXX_TRY`, from arbitrary call depth to one typed
  landing boundary, with destructor execution and propagation-level costs covered by
  the verification reference.
- Interop bridges for `std::expected`, `std::error_code`, optional-like boundaries,
  boolean-plus-value boundaries, and opt-in exception crossings. The `std::expected`
  bridge stays usable in a freestanding, no-exceptions build; the exception bridge is
  present only when exceptions are enabled. See
  [docs/reference/interop.md](docs/reference/interop.md).
- Boundary-input validation checks, sanitizer tiers, and fuzz targets for malformed
  bridge input.
- The experimental `jmpxx/unwind.hpp` arm: an opt-in non-local escape that requires
  unwind cleanup tables, refuses `-fno-exceptions`, runs destructors through the
  platform unwinder, measures and gates the sad path, and documents the catch-all,
  `noexcept`, and table caveats across the supported ABIs. See
  [docs/reference/unwind.md](docs/reference/unwind.md).
- The platform abstraction and static platform-fence scan for compiler, operating
  system, architecture, and ABI-specific constructs. See
  [docs/reference/platform.md](docs/reference/platform.md).
- `jmpxx-verify`, `jmpxx-bench`, the binary-size, compile-cost, perf, codegen,
  allocation, ABI-layout, doc-claim, and acceptance gates, each paired with a
  known-bad input where it is a gate. See
  [docs/reference/verification.md](docs/reference/verification.md) and
  [docs/comparison.md](docs/comparison.md).
- `jmpxx-lint`, an out-of-tree Clang companion that flags discarded results,
  unchecked narrow access, and manual failure forwarding. See
  [docs/reference/lint.md](docs/reference/lint.md).
- The optional reflection forward layer, `jmpxx/reflect.hpp`, deriving enum names,
  value casts, failure sets, and enum-backed error domains through C++26 reflection
  where available and a C++20 fallback otherwise. See
  [docs/reference/reflect.md](docs/reference/reflect.md).
- Packaging through `find_package`, FetchContent, `add_subdirectory`, CPM, Conan, a
  vcpkg overlay port, and generated `jmpxx-core.hpp` and `jmpxx.hpp` single headers.
  The CMake version file uses matching major and minor compatibility while jmpxx is
  below 1.0. See [docs/reference/packaging.md](docs/reference/packaging.md).
- ABI layout checks for the frozen public layout subset, including size, alignment,
  field offsets, and trivial copyability, with the experimental unwind arm exempt until
  it graduates. See [docs/reference/abi.md](docs/reference/abi.md).
- Documentation for adoption: the no-exceptions orientation, cookbook, migration
  guides, public examples, and the reference application.
- `JMPXX_VERSION`, `JMPXX_VERSION_STRING`, and the major, minor, and patch component
  macros for consumer version checks.
- SPDX license tags on source files and the MIT license text in `LICENSE`.

[Unreleased]: https://github.com/DimitryQm/jmpxx/compare/v0.1.2...HEAD
[0.1.2]: https://github.com/DimitryQm/jmpxx/compare/v0.1.1...v0.1.2
[0.1.1]: https://github.com/DimitryQm/jmpxx/compare/v0.1.0...v0.1.1
[0.1.0]: https://github.com/DimitryQm/jmpxx/releases/tag/v0.1.0
