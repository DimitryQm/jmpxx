<!-- SPDX-License-Identifier: MIT -->
# Security policy

jmpxx is a header-only library compiled into your program, so it runs in your
program's trust domain.

## Reporting a vulnerability

Please report security issues privately using GitHub's "Report a vulnerability"
button on this repository rather than opening a public issue. Include the
affected version, the compiler and configuration, and a minimal reproducer. We
will acknowledge the report, work on a fix, and credit you on disclosure unless
you prefer to stay anonymous.

## In scope

A defect in jmpxx that introduces undefined behavior, a memory error, or an
unchecked overflow into a program that uses the documented API as documented.

jmpxx is written to keep memory in bounds, to check overflowing arithmetic, to
avoid undefined behavior (the tests run under AddressSanitizer and
UndefinedBehaviorSanitizer), and to validate untrusted input at the boundary so
malformed data produces a defined failure instead of undefined behavior.

## Out of scope

- A program that modifies jmpxx's own headers. The library is compiled inside
  your trust domain.
- Translation of diagnostic text. Diagnostics are technical and English-only.
- Async-signal-safety of the diagnostics layer. The minimal error representation
  is usable from a signal handler; the diagnostics layer is not, and that is
  documented where it applies.

## Supported versions

While jmpxx is pre-1.0, security fixes target the latest released minor version.
