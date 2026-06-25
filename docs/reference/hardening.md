<!-- SPDX-License-Identifier: MIT -->
# Hardening

jmpxx has a graded portable hardening mode for checks that turn misuse or an
internal invariant violation into a defined fail-fast event. The mode is selected
per translation unit before including jmpxx headers.

## Modes

`JMPXX_HARDENING_MODE` accepts these values:

- `JMPXX_HARDENING_NONE`: no portable hardening checks are required to remain in
  optimized generated code.
- `JMPXX_HARDENING_FAST`: checks narrow value and error accessors, so asking for a
  value from a failure or an error from a value terminates through the platform
  fail-fast boundary instead of becoming undefined behavior.
- `JMPXX_HARDENING_EXTENSIVE`: includes the fast checks and adds deeper defensive
  boundary checks, such as validating a type-erased error domain before dispatch.

When neither macro is set, release builds default to `JMPXX_HARDENING_NONE` and
debug builds default to `JMPXX_HARDENING_FAST`. The old binary
`JMPXX_HARDENED` switch is still accepted: `0` maps to none and any nonzero value
maps to fast. Code that reads `JMPXX_HARDENED` still sees `0` or `1`, derived from
the graded mode. If both switches are defined and disagree, the header rejects the
translation unit rather than letting two public configuration macros report
different postures.

## What is checked

Hardening is for contract violations, not recoverable errors. A malformed file, a
failed syscall, or a rejected parse should travel as a `result` failure. A value
accessed through the wrong narrow accessor or a corrupted type-erased boundary is
a broken program state, so the hardened behavior is fail-fast.

Use `has_value()`, `operator bool`, `value_or`, or `error_or` when a branch is part
of normal control flow. The narrow `value()`, `error()`, `assume_value()`, and
`assume_error()` accessors are for code whose preceding control flow has already
established the state.

## Verification

The `hardening.mode` CTest tier checks both sides of the contract:

- In fast and extensive modes, a live program that uses a narrow accessor in the
  wrong state terminates through the fail-fast boundary.
- In extensive mode, a live corrupted type-erased boundary terminates through the
  fail-fast boundary.
- In lower modes, optimized generated code for the checks not promised by that
  mode does not contain the trap instruction on the gated x86-64 Clang cell.
- A compatible legacy/new macro pair compiles, while a contradictory pair fails
  compilation.
- `hardening.mode.teeth` deliberately asks the gate to accept the wrong absence
  expectation, and must fail as the gate's inverted self-test.

The configuration matrix also compiles and runs hardening with diagnostics,
stack-trace capture, exceptions disabled, RTTI disabled, release defaults, and
debug defaults so the switches do not silently rot apart.
