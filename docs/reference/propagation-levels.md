<!-- SPDX-License-Identifier: MIT -->
# Propagation levels

jmpxx offers failure propagation at two levels. Each states its cost, and the
`jmpxx-verify levels` command reports the same cost so the documentation cannot
drift from what the harness measures.

## Checked propagation (`JMPXX_TRY`, `JMPXX_TRYV`, `JMPXX_TRYX`)

The default, zero-overhead level. `JMPXX_TRY` evaluates a `result`, binds its
value on success, and returns its failure to the caller otherwise; `JMPXX_TRYV`
does the same without binding a value, and `JMPXX_TRYX` is the value-yielding
form usable inside an expression (available on GCC and Clang).

Cost: one conditional branch; no allocation; no frame. The committed codegen
golden shows an eight-frame success path inlining to a branchless sequence with
no stack spill.

Reach: propagates one frame per use and composes to any depth. Each frame opts
in with a single construct; there is no hidden control flow.

## Landing scope (`jmpxx::try_scope`)

A function-style boundary that runs a callable and returns its result, marking
the single typed point a region's propagation lands at. Later layers attach
failure handling and diagnostic capture here.

Cost: one call frame unless inlined; no allocation. When the call inlines the
boundary disappears entirely.

Reach: one typed landing boundary for a region.
