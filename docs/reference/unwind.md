<!-- SPDX-License-Identifier: MIT -->
# Experimental non-local unwind arm

The unwind arm returns a failure from arbitrary call depth to a single landing boundary
while the platform unwinder runs the destructor of every automatic object on the path.
The intermediate frames carry no propagation construct in their source, which the
portable `JMPXX_TRY` cannot offer. It is the closest standard-C++ approach to a non-local
jump that still preserves RAII, and it is experimental and platform-specific. It is opt-in
and never on a default path: a program reaches it only by including `jmpxx/unwind.hpp`.

The arm is not a replacement for the portable surface. The portable `result` and
`JMPXX_TRY` are the exception-free, table-free default and remain the right choice for
the embedded and low-latency niche. The arm is for code that can afford unwind cleanup
tables and wants the source-oblivious middle frames a deep recursion or a deep call chain
would otherwise have to thread by hand.

## Surface (`jmpxx/unwind.hpp`)

`jmpxx::unwind::escape_scope<E>(body)` runs `body` within a landing boundary and returns
a `result<T, E>`, with `T` deduced from `body`'s return type and `E` defaulting to
`jmpxx::error`. If `body` returns normally its value becomes the result's value; if any
frame it transitively calls invokes `eject<E>`, the stack unwinds back to this scope
running every destructor on the path, and the result holds that error. Scopes nest: an
eject lands at the nearest enclosing `escape_scope`. The landing allocates nothing.

`jmpxx::unwind::eject(err)` performs the escape to the innermost `escape_scope` on the
current thread, carrying `err`. The error type must match the receiving scope and must be
small and trivially copyable, which every error representation the library ships
satisfies; richer context travels out of band, as the resolver in the reference
application does by stashing the offending key before it ejects. `eject` does not return
at run time, but it is deliberately not `[[noreturn]]`, because that attribute would let
the optimizer prove the eject site neither returns nor throws and elide the cleanup
landing pads the forced unwind depends on. It returns a type convertible to anything, so
`return jmpxx::unwind::eject(err);` type-checks in a function of any return type; a
helper that wraps `eject` returns normally and is not itself `noreturn`.

`jmpxx::unwind::available()` is a `constexpr bool` that is false on a target with no
backend, where `escape_scope` and `eject` refuse instantiation with a stated
precondition. A program can branch on it to fall back to the portable surface.

## The unwind-tables precondition

The arm runs destructors only where the compiler has emitted unwind cleanup tables, which
is the effect of compiling with exceptions enabled. A translation unit that includes
`jmpxx/unwind.hpp` with exceptions disabled is refused at compile time rather than
silently producing an arm that skips destructors. The refusal covers only the translation
unit that opts in; every frame on the escape path must likewise be compiled with cleanup
tables, a runtime precondition the header cannot check. A frame compiled `-fno-exceptions`
on the escape path has its destructors skipped, which is the undefined-RAII outcome the
arm exists to avoid, so the path must be built with tables throughout.

## Caveats

A typed catch on the escape path transits the escape without entering it on every runtime,
so a frame that catches its own typed exceptions is unaffected. A catch-all is not
portable. How a `catch (...)` interacts with the escape depends on the C++ runtime: it
transits the escape on libcxxrt, terminates the program on libc++abi, and on libstdc++ and
WebAssembly transits when it rethrows with the idiom
`catch (const abi::__forced_unwind&) { throw; }` and otherwise consumes the escape, which
the arm turns into a defined termination. The outcome is loud in every case, never a silent
mis-land. Keep a catch-all off the escape path, or use a typed catch.

A frame on the escape path marked `noexcept` terminates the unwind at that frame, because
an empty exception specification is a barrier the forced unwind cannot cross. Functions on
the path between an eject and its landing must not be `noexcept`.

The sad path costs an unwinder walk and is not for the hottest escape path. The
`jmpxx-verify unwind` command measures the forced-unwind escape distribution beside a C++
throw at the same depth and reports both: the sad path is bounded, and on the machines
measured it is comparable to a throw rather than uniformly faster. The gate the command
enforces is a determinism bound, the ratio of the arm's 99th-percentile latency to the
throw's at the same depth, which a non-deterministic cleanup is shown to exceed. The two
paths are timed back to back in one loop so that scheduling noise on a shared runner
inflates both tails together and cancels in the ratio rather than failing the gate
spuriously.

## Platform support

The arm drives platform machinery whose behavior differs by ABI, so its guarantees are
stated per ABI. The unwind-execution tier checks them, and continuous integration runs
that tier on each reachable ABI.

On the Itanium and DWARF interface, on x86-64, ARM64, and RISC-V, the arm drives
`_Unwind_ForcedUnwind` through libgcc or libunwind. An escape from nine frames runs every
destructor exactly once across GCC and Clang at every optimization level, on x86-64
natively and on ARM64 and RISC-V under emulation. On 32-bit ARM the exception-handling ABI
uses the same interface with an 8-character exception class. The landing identifies its
frame by the carrier's address during the unwind rather than by a frame address read
outside one, because the EHABI unwinder does not provide the latter.

On WebAssembly there is no forced-unwind primitive, so the escape is a typed throw the
virtual machine unwinds to the landing's catch, with the intermediate destructors run as
the compiler's catch-all-and-rethrow clauses. Destructor counts, nesting, the payload
boundary, and typed and cooperative catch-all transit all hold. Two guarantees differ. The
escape is an ordinary catchable exception with no forced-unwind exemption, so a
non-cooperative catch-all that swallows it is caught by the termination backstop rather
than by the platform unwinder. The sad path is whatever the engine charges for a throw
rather than a library-bounded walk.

On Windows the arm is reached two ways. A MinGW toolchain exposes the libgcc
`_Unwind_ForcedUnwind` interface and uses the same backend as the forced-unwind ABIs.
Native MSVC drives the cleanup through an unwinding `longjmp`, which under the C++
exception model performs a termination unwind that runs the destructors of the frames it
passes. The unwind-execution tier runs on MSVC and the catch-all transit holds.

On a bare-metal ARM target the arm runs on a freestanding Cortex-M3 with no operating
system, built against newlib and run under a system-mode emulator, where a depth-eight
escape runs every destructor through the EABI forced unwind. Two bare-metal preconditions
follow. The arm uses thread-local storage for its per-thread state, which a bare-metal
runtime must provide, read on the EABI through `__aeabi_read_tp`; a single-threaded target
can answer it with one fixed block. The `.ARM.exidx` unwind index must be kept by the
linker script, or the unwinder finds no tables and the arm cannot run.
