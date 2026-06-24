<!-- SPDX-License-Identifier: MIT -->
# jmpxx documentation

jmpxx is a header-only C++20 library for non-local, RAII-correct, exception-free error propagation.
The [project README](../README.md) is the introduction and the first example. This directory holds
the performance comparison, the API reference, and the tooling reference.

## Performance

[comparison.md](comparison.md) measures jmpxx against a hand-written branch, `std::expected`,
`std::error_code`, language exceptions, Boost.Outcome, Boost.LEAF, and tl::expected, and states
where jmpxx wins and where it does not. The happy path is the same machine code as a hand-written
branch, and the failure path is bounded and deterministic where a thrown exception is neither.

## API reference

- [propagation-levels.md](reference/propagation-levels.md): `JMPXX_TRY` and the landing scope, and
  the cost of each.
- [policies.md](reference/policies.md): the minimal, rich, and type-erased error representations
  over one transport.
- [diagnostics.md](reference/diagnostics.md): the debug-only origin and causal chain, free in
  release.
- [interop.md](reference/interop.md): the `std::expected`, `std::error_code`, exception, and
  optional-like bridges.
- [platform.md](reference/platform.md): the fenced platform and ABI abstraction.
- [unwind.md](reference/unwind.md): the experimental, opt-in non-local unwind arm.

## Tooling

- [verification.md](reference/verification.md): `jmpxx-verify` and `jmpxx-bench`, the gates that
  back every cost claim, and how to reproduce the comparison.
- [lint.md](reference/lint.md): `jmpxx-lint`, the companion check set that enforces the usage
  discipline in consumer code.

The [reference application](../reference_app/README.md) consumes jmpxx through `find_package`
alongside toml++ and glaze, and is the worked example of using the library in a real program.
