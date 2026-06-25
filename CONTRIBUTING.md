<!-- SPDX-License-Identifier: MIT -->
# Contributing to jmpxx

Use this guide to build jmpxx, run its checks, and keep a change inside the
project's conventions.

## Prerequisites

- A C++20 compiler. The CI matrix lists the supported minimum versions; anything
  at or above them works.
- CMake 3.20 or newer and a generator such as Ninja.
- For the verification tool only: an LLVM install that provides `FileCheck` and
  `lit`, used by the codegen and compile-fail tests. These are build-time tools,
  not a dependency of the library.

jmpxx has no runtime dependencies, and its public headers include nothing beyond
a small subset of the standard library. Please keep it that way. Changes that add
a runtime dependency, or pull a new header onto the public surface, will not be
accepted: zero dependencies and freestanding support are core features.

## Building and testing

```sh
cmake --preset dev
cmake --build --preset dev
ctest --preset dev
```

Each test tier can also be run on its own. The `jmpxx-verify` tool exposes every
observable library capability and every gate; pass `--format=json` for structured
output.

## Conventions

- Measure, don't assert. Any statement about size, speed, or generated code must
  have a passing gate behind it, and each gate is checked against a known-bad
  input so it cannot pass silently.
- Land tests with the code. A user-facing change includes its tests in the same
  pull request.
- Keep the code correct, readable, and free of undefined behavior. Keep
  documentation accurate and state limitations plainly, including any axis where
  jmpxx loses to an alternative.
- Build the strict configurations too: exceptions off, RTTI off, and the minimal
  core in a freestanding build. CI runs these so the guarantees stay honest.

## Commit messages

Use a short imperative subject ("Add ...", "Fix ...") and explain in the body why
the change is needed and any trade-offs it makes. Keep each commit focused on one
thing.

## Reporting a vulnerability

See [SECURITY.md](SECURITY.md). Please do not open a public issue for a security
report.
