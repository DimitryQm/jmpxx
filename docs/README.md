<!-- SPDX-License-Identifier: MIT -->
# jmpxx documentation

jmpxx is a header-only C++20 library for non-local, RAII-correct,
exception-free error propagation. The [project README](../README.md) is the
introduction and first example. This directory holds the guides, reference pages,
comparison data, and tooling reference.

## Start here

- [why-no-exceptions.md](why-no-exceptions.md): why exceptions get disabled and
  where jmpxx fits.
- [cookbook.md](cookbook.md): task-oriented recipes, each drawn from a runnable example.
- Migrating from what you use now:
  [std::expected](migration/from-expected.md),
  [std::error_code](migration/from-error-code.md),
  [exceptions](migration/from-exceptions.md).

## Performance

[comparison.md](comparison.md) measures jmpxx against a hand-written branch,
`std::expected`, `std::error_code`, language exceptions, Boost.Outcome,
Boost.LEAF, and tl::expected, and states where jmpxx wins and where it does not.

## API reference

- [propagation-levels.md](reference/propagation-levels.md): `JMPXX_TRY` and the landing
  scope, and the cost of each.
- [policies.md](reference/policies.md): the minimal, rich, and type-erased error
  representations over one transport.
- [diagnostics.md](reference/diagnostics.md): the debug-only origin and causal chain,
  free in release.
- [hardening.md](reference/hardening.md): graded fail-fast checks for contract
  violations, with runtime probes and codegen absence gates.
- [interop.md](reference/interop.md): the `std::expected`, `std::error_code`, exception,
  and optional-like bridges.
- [reflect.md](reference/reflect.md): the optional reflection layer that derives error
  metadata, with a C++20 fallback.
- [platform.md](reference/platform.md): the fenced platform and ABI abstraction.
- [unwind.md](reference/unwind.md): the experimental, opt-in non-local unwind arm.

## Distribution

- [packaging.md](reference/packaging.md): installing and depending on jmpxx through
  find_package, FetchContent, CPM, Conan, vcpkg, and the single-header amalgamation.
- [abi.md](reference/abi.md): which type layouts are frozen within a major version, and
  what the promise does and does not cover.

## Tooling

- [verification.md](reference/verification.md): `jmpxx-verify` and `jmpxx-bench`, the
  gates that back every cost claim, and how to reproduce the comparison.
- [lint.md](reference/lint.md): `jmpxx-lint`, the Clang-based checks that enforce the
  usage discipline in consumer code.

The [reference application](../reference_app/README.md) consumes jmpxx through
`find_package` alongside toml++ and glaze, and the [examples](../examples/README.md) are
small programs showing each capability. Both build and run as tests.
