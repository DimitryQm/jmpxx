#!/usr/bin/env bash
# SPDX-License-Identifier: MIT
# Every propagation-level cost the harness reports must appear verbatim in the
# reference documentation, so a documented cost cannot drift from the measured one.
set -euo pipefail
VERIFY="${1:?jmpxx-verify path}"; DOC="${2:?reference doc}"
json="$("$VERIFY" levels --format=json)"
for key in try.cost scope.cost; do
  val="$(printf '%s' "$json" | grep -oE "\"$key\":\"[^\"]*\"" | sed -E "s/\"$key\":\"//; s/\"$//")"
  if [ -z "$val" ]; then echo "FAIL: harness reported no $key"; exit 1; fi
  if ! grep -qF "$val" "$DOC"; then
    echo "FAIL: reference doc does not state the measured $key: $val"; exit 1
  fi
done
echo "doc-claim OK: documented level costs match the harness"
