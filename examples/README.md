<!-- SPDX-License-Identifier: MIT -->
# Examples

Small, self-contained programs, each showing one capability. They build as part of the
project (`-DJMPXX_BUILD_EXAMPLES=ON`, on by default for a standalone build) and run as
tests, so they cannot drift from the library. The cookbook in
[../docs/](../docs/README.md) is drawn from this code.

| File | Shows |
|------|-------|
| [result_basics.cpp](result_basics.cpp) | `result<T>`, `fail`, `JMPXX_TRY`, and one landing point |
| [policies.cpp](policies.cpp) | one body over the minimal, rich, and type-erased error policies |
| [diagnostics.cpp](diagnostics.cpp) | the debug-only origin and causal chain, free in release |
| [interop.cpp](interop.cpp) | the `std::expected`, `std::error_code`, and optional-like bridges |
| [reflection.cpp](reflection.cpp) | the optional reflection layer: enum names, the failure set, a derived domain |
| [unwind_arm.cpp](unwind_arm.cpp) | the experimental, opt-in non-local escape |

## Building and running

```sh
cmake -S . -B build -G Ninja
cmake --build build
./build/examples/example_result_basics
```

The portable examples build with `-fno-exceptions -fno-rtti`, the strict configuration.
`unwind_arm` is built with exceptions enabled, because the experimental arm requires
unwind tables; see [../docs/reference/unwind.md](../docs/reference/unwind.md).
`reflection` builds on a C++20 toolchain through the hand-written fallback and on a
C++26 reflection-capable toolchain through reflection, with identical output.
