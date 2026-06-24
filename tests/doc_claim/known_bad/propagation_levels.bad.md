<!-- SPDX-License-Identifier: MIT -->
# Teeth for the doc-claim tier

This is a deliberately wrong propagation-levels document. It states a cost the harness
does not report, so the doc-claim check, which requires every cost the harness reports
to appear verbatim in the document, must fail against it. A doc-claim tier that passed
this would have no teeth: a documented cost could drift from the measured one unnoticed.

## Checked propagation

Cost: two conditional branches and a heap allocation per use.

## Landing scope

Cost: three call frames that never inline.
