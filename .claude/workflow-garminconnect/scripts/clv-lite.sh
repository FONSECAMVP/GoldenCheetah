#!/usr/bin/env bash
# CLV-lite — Cross-Layer Validation, mechanical tier.
# Checks 1, 2, 5, 6 against state.md + traceability.md + findings.md.
# Runs from the workflow root: .claude/workflow-garminconnect/
# Exit code: 0 = PASS, 1 = FAIL (escalate to CLV-full and drill named artifacts).

set -u

WF_DIR="$(cd "$(dirname "$0")/.." && pwd)"
cd "$WF_DIR" || exit 1

PROJECT="$(basename "$WF_DIR")"
echo "CLV-lite — $PROJECT"

fail=0
warn=0

# ── Check 1: Coverage ─────────────────────────────────────────────────────────
# Every must/should REQ row in traceability.md should have non-empty DEC, DES,
# TEST columns. A row is `| REQ-NNN | … | DECs | DESs | TESTs | COMMIT | …`.
# We approximate "missing" by looking for cells that are just "—" or empty.
c1=$(awk -F'|' '
  /^\| REQ-/ {
    # cell 2 = REQ id, 3 = description, 4 = DECs, 5 = DESs, 6 = TESTs (per traceability schema)
    # state.md also has REQ rows but with different columns; rely on file basename
  }' traceability.md 2>/dev/null)
# Simpler: count REQ rows in state.md `## reqs` with TESTs cell == "—"
missing=$(awk '
  /^## reqs/ {in_table=1; next}
  in_table && /^## / {in_table=0}
  in_table && /^\| REQ?[- ]?/ {next}
  in_table && /^\| [0-9]/ {
    # split on pipe; cell 4 = cat, 7 = TESTs (per state.md schema: REQ | cat | DECs | DESs | TESTs | status)
    n=split($0,a,"|")
    cat=a[3]; tests=a[6]; status=a[7]
    gsub(/^ +| +$/,"",cat); gsub(/^ +| +$/,"",tests); gsub(/^ +| +$/,"",status)
    if ((cat ~ /must|should/) && (tests == "" || tests == "—") && status !~ /not started|deferred|doc-only/) {
      print a[2]
    }
  }
' state.md 2>/dev/null | wc -l)
if [ "$missing" -eq 0 ]; then
  printf "Check 1 (coverage):           PASS\n"
else
  printf "Check 1 (coverage):           WARN — %d must/should REQ(s) in-progress without TEST\n" "$missing"
  warn=$((warn+1))
fi

# ── Check 2: Provenance ──────────────────────────────────────────────────────
# Every TEST/DES/COMMIT cell in traceability.md should have a parent.
# Approximated: every REQ row's DECs cell is non-empty.
orphans=$(awk -F'|' '
  /^\| REQ-/ {
    decs=$4; gsub(/^ +| +$/,"",decs)
    if (decs == "" || decs == "—") print $2
  }' traceability.md 2>/dev/null | wc -l)
if [ "$orphans" -eq 0 ]; then
  printf "Check 2 (provenance):         PASS\n"
else
  printf "Check 2 (provenance):         FAIL — %d REQ(s) with empty DEC cell in traceability.md\n" "$orphans"
  fail=$((fail+1))
fi

# ── Check 5: Cycle finding closure ────────────────────────────────────────────
# `grep -E '\| blocking \|' findings.md | grep -vE '\| (fix-now|deferred|accepted)' ` must be empty.
open_blocking=$(grep -E '\| blocking \|' findings.md 2>/dev/null \
                | grep -vE '\| (fix-now|deferred|accepted) \|' | wc -l)
if [ "$open_blocking" -eq 0 ]; then
  printf "Check 5 (finding closure):    PASS\n"
else
  printf "Check 5 (finding closure):    FAIL — %d open blocking finding(s) in findings.md\n" "$open_blocking"
  fail=$((fail+1))
fi

# ── Check 6: Ledger integrity ────────────────────────────────────────────────
# Every DEC referenced anywhere in *.md exists as `## DEC-NNN` in decisions.md.
# Exclude cross-project references like `coach:DEC-NNN` via a negative lookbehind.
referenced_decs=$(grep -rhoP '(?<![:a-zA-Z])DEC-[0-9]+' --include='*.md' . 2>/dev/null | sort -u)
declared_decs=$(grep -oE '^## DEC-[0-9]+' decisions.md 2>/dev/null | sed 's/^## //' | sort -u)
missing_decs=$(comm -23 <(echo "$referenced_decs") <(echo "$declared_decs") | wc -l)
# state.md `## decs` row count == decisions.md H2 count
state_dec_rows=$(awk '/^## decs/{in_t=1;next} in_t && /^## /{in_t=0} in_t && /^\| 0[0-9]+/{c++} END{print c+0}' state.md)
ledger_dec_count=$(echo "$declared_decs" | grep -c .)
if [ "$missing_decs" -eq 0 ] && [ "$state_dec_rows" -eq "$ledger_dec_count" ]; then
  printf "Check 6 (ledger integrity):   PASS\n"
elif [ "$missing_decs" -gt 0 ]; then
  printf "Check 6 (ledger integrity):   FAIL — %d DEC(s) referenced but not declared in decisions.md\n" "$missing_decs"
  fail=$((fail+1))
else
  printf "Check 6 (ledger integrity):   WARN — state.md `## decs` rows (%d) != decisions.md H2 count (%d)\n" "$state_dec_rows" "$ledger_dec_count"
  warn=$((warn+1))
fi

# ── Overall ──────────────────────────────────────────────────────────────────
if [ "$fail" -gt 0 ]; then
  echo "Overall: FAIL — escalate to CLV-full and drill named artifacts."
  exit 1
elif [ "$warn" -gt 0 ]; then
  echo "Overall: WARN — informational; CLV-full recommended at next phase exit."
  exit 0
else
  echo "Overall: PASS"
  exit 0
fi
