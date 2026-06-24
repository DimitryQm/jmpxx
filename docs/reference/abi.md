<!-- SPDX-License-Identifier: MIT -->
# ABI and layout stability

jmpxx is header-only and ships no compiled object, so its ABI is the observable
memory layout of its public types: the size, the alignment, the public field
offsets, and the trivial copyability a consumer's register passing and any persisted
or shared-memory representation depend on. This page states which layouts are frozen
within a major version, which are free to change, and how the promise is enforced.

## What is frozen

Within a major version the observable layout of these types does not change. A change
to one of them is a major-version break, recorded with a migration note.

- `jmpxx::error`: eight bytes, four-byte aligned, `code` at offset 0 and `domain` at
  offset 4, standard-layout and trivially copyable. This is the representation the
  freestanding and embedded audience stores and threads, so its layout is fixed.
- `jmpxx::erased_error`: a value and a pointer to a static domain descriptor,
  trivially copyable. On a 64-bit target it is sixteen bytes, eight-byte aligned.
- `jmpxx::rich_error` in a release build: identical to `jmpxx::error` in size,
  alignment, and the `code` and `domain` offsets, which is the layout half of the
  rich-free-in-release guarantee. The debug representation carries one extra handle
  word and is **not** part of this promise; see below.
- `jmpxx::result<T, E>` for a trivially copyable `T` over each frozen `E`: its size,
  alignment, and trivial copyability are fixed, because that is what a caller passing
  it in registers relies on. For example `result<int, error>` is twelve bytes.

## What is not frozen

- The debug representation of `jmpxx::rich_error`. The diagnostic handle it carries
  in a debug build is a build-configuration artifact that never ships in release, so
  its size is free to change. The frozen rich layout is the release layout.
- Anything under a `detail` namespace, including the diagnostic store and the
  layout of `result` over a non-trivial `T`, whose size follows the contained type.
- The experimental unwind arm and every type it uses are exempt from the stability
  promise until a later release graduates the arm, and the arm states its
  experimental status at its surface.

## Pre-1.0 scope

While the project is below 1.0 the public API surface may still gain additions
between minor versions, each recorded in the change history. The frozen layouts above
are the exception: they hold from v0.1.0 across the 0.x series, so an embedded
consumer that stores a `jmpxx::error` or passes a `result<int, error>` across a
compilation boundary can rely on the layout now rather than waiting for 1.0. A change
to a frozen layout before 1.0 still requires a version bump and a migration note; it
is not made silently between patches.

## What the promise does not cover

Layout stability is not binary compatibility across mismatched toolchains. A
header-only library instantiates its templates in the consumer's own translation
unit, with the consumer's compiler, flags, and standard library, so two objects built
with incompatible toolchains are incompatible regardless of this promise. What jmpxx
fixes is the layout its own types take for a given target ABI; combining objects built
with different toolchains or standard-library versions is the consumer's
compatibility concern, the same one every C++ library leaves to the consumer.

## How the promise is enforced

The frozen layout is gated, not asserted in prose.

- A static tier pins the architecture-independent facts at translation time, so a
  reorder or an inserted field fails the build on every cell, not only where the
  layout gate runs. `offsetof` is well defined for these types because the public
  error types are standard-layout.
- `jmpxx-verify abi-layout` compiles a descriptor of the frozen types in the
  configuration the niche ships, release with exceptions and RTTI disabled, runs it
  to emit each type's size, alignment, field offsets, and trait bits, and diffs the
  result against a committed golden under `goldens/abi/`. A divergence is an
  unversioned layout change and fails the build. The gate has teeth: pointed at a
  golden that claims a different layout, it fails, which is the unversioned-change
  case it exists to catch. The golden is regenerated with `--update` only after a
  justified, version-bumped change.
- The installed CMake package version file declares `SameMajorVersion` compatibility,
  so `find_package(jmpxx 0)` accepts any 0.x and a consumer pins the major version it
  built against.
