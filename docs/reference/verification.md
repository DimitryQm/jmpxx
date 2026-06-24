<!-- SPDX-License-Identifier: MIT -->
# Verification surface

A header library cannot be run on its own, so jmpxx ships a command-line surface that compiles
fixtures, measures the result, and gates it. Every performance, size, and codegen claim the project
makes is produced here and fails the build on a regression, and every claim in
[../comparison.md](../comparison.md) is reproducible with the commands below. Two tools make up the
surface: `jmpxx-verify` compiles a fixture and measures it, and `jmpxx-bench` runs a workload and
times it. Both take `--format=json` for one machine-readable object per probe.

## `jmpxx-verify`

Each subcommand reports named metrics and a pass or fail. The behavioral commands run the engine and
report what it did. The codegen and size commands compile a fixture and inspect the output.

- `semantics`, `destructors`, `policies`, `diagnostics`, `interop`, `reflect`, `platform` exercise
  the transport contract, the destructor counts across a deep propagation, the three policies over
  one body, the diagnostic context, the bridges, the reflection-derived error metadata, and the
  build's matrix cell.
- `abi-layout` compiles a descriptor of the frozen public types in the ship configuration, reports
  each type's size, alignment, field offsets, and trait bits, and diffs the layout against a
  committed golden, so an unversioned layout change fails the build. See [abi.md](abi.md).
- `unwind` drives the experimental arm and reports its destructor count and sad-path distribution,
  detailed in [unwind.md](unwind.md).
- `codegen` compiles a fixture at `-O2` for a target, slices one function's assembly, and diffs it
  against a committed golden, reporting instruction count and stack-spill presence. `release-diff`
  compiles the same operation under the minimal and the rich policy in a release configuration and
  requires their machine code to match, which proves the rich policy is free in release.
- `size` reports the transport size per policy against a no-overhead budget. `alloc` counts heap
  allocations over the paths declared allocation-free, through a replaced global `operator new`.
- `size-delta` compiles a jmpxx fixture and a hand-written baseline to objects in the ship
  configuration and reports the difference in section bytes, which is the library's size cost.
- `compile-cost` compiles a jmpxx fixture and a baseline and reports the translation cost as the
  template-instantiation count, a deterministic metric, and the wall-clock ratio alongside it.
- `levels` reports each propagation level's cost, listed in [propagation-levels.md](propagation-levels.md).
- `all` runs the behavioral and size commands together.

## `jmpxx-bench`

`jmpxx-bench` drives one error-propagation kernel implemented identically for jmpxx, a hand-written
branch, `std::expected`, a threaded `std::error_code`, language exceptions, Boost.Outcome,
Boost.LEAF, and tl::expected, so a measured difference is the mechanism and not the work. The results
and the honest comparison are in [../comparison.md](../comparison.md).

- `run` sweeps a failure ratio (0, 1, 50, and 100 percent) and a depth (8 and 32) and reports each
  mechanism's per-call latency by median, 90th, and 99th percentile, with the ratio against the
  hand-written baseline.
- `gate` co-measures jmpxx and the hand-written baseline on the happy path and bounds jmpxx's median
  to a multiple of the baseline's, the perf gate.
- `callgrind`, run under `valgrind --tool=callgrind --collect-atstart=no`, counts the instructions a
  region executes. The count is identical on every machine, so it is the deterministic perf gate.

The measurement controls follow the established practice for tiny operations. The kernel frames are
non-inlined so the propagation crosses real stack frames rather than collapsing to a few
instructions. The inputs are a shuffled mix at the target failure rate, so the branch is taken in an
order the predictor cannot learn, with a fixed seed for reproducibility. Each result passes through
an optimization barrier. Each cell calibrates its iteration count to a target time so the happy path
and the far slower exception sad path are both measured well. The reported statistic is the median
over epochs with the high percentiles, not the mean, because the distribution is right-skewed.

## Gates

A gate is a measurement with a committed bound that fails the build when crossed, and each gate is
checked against a known-bad input so it cannot pass silently.

| gate | command | bound | known-bad input |
|------|---------|-------|-----------------|
| codegen | `codegen` | optimized assembly matches the golden | a wrong golden and a spilling fixture |
| release diff | `release-diff` | rich and minimal policies match in release | a fixture whose diagnostic call survives release |
| size | `size` | transport within the no-overhead budget | a transport over budget |
| size delta | `size-delta` | zero bytes over the hand-written baseline | a bloated fixture |
| no-alloc | `alloc` | zero allocations on the declared paths | an allocating path |
| compile cost | `compile-cost` | instantiation count within budget | an instantiation-heavy fixture |
| perf | `bench gate` and the callgrind script | jmpxx within a multiple of the baseline | a deliberately slowed kernel |
| unwind determinism | `unwind` | sad-path tail within a multiple of a throw's | an injected non-deterministic cleanup |
| abi layout | `abi-layout` | frozen type layout matches the committed golden | a golden claiming a changed layout |
| doc claim | `doc_claim` script | a documented cost matches the harness | a doc stating a cost the harness does not report |

## Structured output

With `--format=json` each probe prints one object: `probe` names the command, `ok` is the pass or
fail boolean, `metrics` maps each metric name to a number, string, or boolean, and `notes` carries
any messages. Continuous integration and the gate scripts consume this rather than parsing prose.
The schema is stable within a major version.

## Acceptance sweep

`verify/acceptance.py` runs the whole suite over a built tree and reduces it to one release
verdict. It runs every tier and every gate through CTest, pairs each gate with its inverted
self-test (the `.teeth` cases), and reports a gate green only when both the gate and its teeth
pass. A gate with no passing inverted self-test is reported `unteethed` and fails the verdict, so
the report cannot read green while a gate is unproven. It records the cell's compiler,
architecture, and standard and the headline metrics from `jmpxx-verify`.

```sh
python3 verify/acceptance.py --build-dir build --format json --out acceptance.json
```

The report object carries `cell`, `summary`, `gates` (each with its status and its gate and teeth
members), `tiers`, `metrics`, and a top-level `verdict`, and the schema is stable within a major
version. `--self-test` proves the verdict logic itself, that it fails a failed test and a gate
with no inverted self-test, and it runs as a tier in the suite.

## Reproducing the comparison

```sh
cmake -S . -B build -G Ninja && cmake --build build

# Identical happy-path codegen and zero size delta against the hand-written baseline.
./build/verify/jmpxx-verify size-delta \
  --fixture benchmarks/fixtures/ship_jmpxx.cpp \
  --baseline benchmarks/fixtures/ship_handwritten.cpp --max-total-delta 0

# Deterministic instruction counts per mechanism (requires valgrind).
valgrind --tool=callgrind --collect-atstart=no --callgrind-out-file=cg.out \
  ./build/benchmarks/jmpxx-bench callgrind --mech jmpxx --fail 0 --iters 100000
grep '^summary:' cg.out

# The wall-clock distribution sweep across mechanisms, ratios, and depths.
./build/benchmarks/jmpxx-bench run

# The compile-cost tax as a deterministic instantiation count.
./build/verify/jmpxx-verify compile-cost --cxx clang++ \
  --fixture benchmarks/fixtures/ship_jmpxx.cpp \
  --baseline benchmarks/fixtures/ship_handwritten.cpp --max-instantiations 320
```
