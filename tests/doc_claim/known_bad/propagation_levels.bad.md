<!-- SPDX-License-Identifier: MIT -->
# Known-bad doc-claim fixture

Deliberately wrong propagation-levels document. It states a cost the harness does not
report, so the doc-claim check must fail against it. A doc-claim tier that passed this
fixture would let a documented cost drift from the measured one unnoticed.

## Checked propagation

Cost: two conditional branches and a heap allocation per use.

## Landing scope

Cost: three call frames that never inline.
