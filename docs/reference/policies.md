<!-- SPDX-License-Identifier: MIT -->
# Error representation policies

jmpxx carries a failure in one of three representations selected by the error type
of `result<T, E>`. One transport and one set of propagation sites serve all three,
so the same source compiles under any policy and the policy is the only difference.
The `policies` command of `jmpxx-verify` exercises one body under all three and
reports that their observable behavior is identical.

## Minimal: `jmpxx::error`

A bare integer `code` and `domain`, trivially copyable and register sized, carrying
no out-of-band context. It is the default error type of `result<T>`, it pulls in
nothing outside the freestanding subset of the standard library, and it is usable
where there is no heap, no exceptions, and no RTTI.

Use it where binary size and the freestanding promise matter more than context, and
where the failure site and the call code are enough to diagnose a failure. It is the
smallest and cheapest representation and is a first-class supported path, not a
degraded mode.

Cost: the transport for a trivial value under this policy is a value-or-error union
plus a discriminant, within the size budget the `size` command reports. No path
allocates, which the `alloc` command confirms over construction, inspection,
extraction, assignment, and propagation.

## Rich: `jmpxx::rich_error`

The default for hosted application code. It carries the same in-band `code` and
`domain` as the minimal error, and in a debug build it also tags the failure with a
handle to out-of-band diagnostic context: the failure's origin and the causal chain
it accumulates as it propagates. In a release build the handle and every diagnostic
facility are compiled out, and `rich_error` becomes layout- and codegen-identical to
`jmpxx::error`.

Use it where a developer benefits from learning where a failure began and how it
travelled, and where the cost of that context in debug is acceptable and its absence
in release is required. Selecting it changes no call site that the minimal policy
uses; see [diagnostics.md](diagnostics.md) for the context surface and its bounds.

Cost: in release, identical to the minimal policy. The `jmpxx-verify release-diff`
command compiles the same operation under both policies in a release configuration
and requires their machine code to match, and it fails if a diagnostic call or a
source-location string survives into release. In debug, `rich_error` carries one
extra handle word and its construction records a context entry in a fixed per-thread
arena, which allocates nothing.

## Type-erased: `jmpxx::erased_error`

A boundary-safe error that carries a domain-tagged value usable by code that neither
knows nor cares about the originating error category. It is a value and a pointer to
a static domain descriptor, trivially copyable, register sized, free of RTTI, and
free of heap allocation. The descriptor's virtual `name()` and `message(int)` carry
the type identity that distinguishes one error family from another; virtual dispatch
works with RTTI disabled because only `typeid` and `dynamic_cast` need it.

Use it at a component boundary where the caller should read a failure uniformly
without depending on the callee's error enum. A component defines one
`jmpxx::error_domain` subclass with a static instance and constructs
`erased_error(value, domain)`; the caller reads `value()`, `domain().name()`, and
`message()`. A bare `erased_error(code, domain_tag)` constructs into a built-in
generic domain, so the same call-site source that builds a minimal error builds an
erased one.

Identity: two erased errors are equal when they name the same domain descriptor and
the same value, compared by descriptor address. Across a shared-library boundary a
domain defined in a header may have one descriptor instance per module, so within a
single program the comparison is exact and across modules it is not; a program that
needs cross-module identity should compare on a value the domain exposes.

Cost: the erased policy adds no allocation, which the `alloc` command confirms. The
`policy.erased_nortti` test builds and runs the policy with `-fno-rtti
-fno-exceptions`.

## Selecting a policy

A program selects a policy by the error type it names in `result<T, E>`, commonly
through one alias. The reference application defines `app::error` and
`template <class T> using result = jmpxx::result<T, app::error>;` in one header and
selects the underlying type with a build flag, so every call site below the alias is
identical across policies. Switching the flag rebuilds the same source under a
different policy with no source change, verified by building one file twice.
