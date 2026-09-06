#!/usr/bin/env bash
# CLV-lite — Cross-Layer Validation, mechanical tier.
#
# WHAT THIS FILE IS: a thin runner. Checks 0, 1, 1b, 2 and 6 are small enough to live here;
# Check 5 is NOT implemented here at all — it invokes scripts/clv_findings.py, which is the one
# canonical implementation. See "ONE MECHANISM" below.
#
# ── 2026-08-30, pass 1 (documentation reconciliation) ────────────────────────────────────────
# The revision before that one had drifted in four ways, each repaired and each named here so a
# reader can audit that the check was CORRECTED rather than weakened:
#   1. CHECK 1 WAS SILENTLY VACUOUS — it read a `state.md` that DEC-015 deleted on 2026-07-13.
#      `awk` on a missing file yields zero rows, zero rows means "nothing missing", so it printed
#      PASS without looking at anything. Check 0 now makes a missing input a hard FAIL and Check 1
#      reads the DEC-015 sources of truth (prd.md, traceability.md).
#   2. CHECK 6 HAD A REGEX FALSE POSITIVE — `DEC-[0-9]+` matched the `DEC-0` inside the
#      metasyntactic placeholder `DEC-0NN` and reported a nonexistent `DEC-0`. Removes a FALSE
#      finding, no TRUE one.
#   3. CHECK 2 COUNTED A LABELLED PROVENANCE ROW AS A DEFECT — `REQ-027-PRIOR` is a superseded row
#      kept deliberately, and a superseded row legitimately carries no DEC. `-PRIOR` rows, and ONLY
#      those, are now skipped, and the exclusion is printed so it cannot hide a live row.
#
# ── 2026-08-30, pass 2 (gate repair) — THE TWO REPAIRS THAT MATTER ───────────────────────────
#   4. ONE MECHANISM, NOT TWO. Check 5 used to be an awk one-liner here that was a hand-copy of a
#      second awk one-liner pasted into findings.md, kept in step by a comment asking the next
#      editor to re-sync them by hand. Both copies are gone. `scripts/clv_findings.py` is the sole
#      implementation, `findings.md` DESCRIBES it, and this file RUNS it.
#   5. CHECK 5 FAILED OPEN AND NOW FAILS SAFE. The predecessor reported 28 unparseable rows as
#      "reported, not blocking" and passed over them — several of those rows literally begin
#      `**BLOCKING — …**`. It also SILENTLY SKIPPED any row whose severity cell held prose rather
#      than a severity, which is not a judgement that the row is harmless but the absence of any
#      judgement. An unreadable row, an unreadable severity, an unreadable disposition and a
#      deliberately-unset severity are now each a hard FAIL, because each one means the mechanism
#      CANNOT prove the absence of a blocker.
#   6. CHECK 1b NOW SEES THE NON-FUNCTIONAL REQUIREMENTS. It only ever matched `REQ-NNN`, so all 16
#      `REQ-NF-*` ids sat outside spine-completeness entirely. Grouped rows are expanded to the ids
#      they name.
#
# WHAT WAS DELIBERATELY *NOT* SOFTENED: every genuinely open item still FAILS this script, and the
# repairs of pass 2 make MORE things fail, not fewer. Nothing here grants a waiver.
#
# Runs from anywhere. Exit code: 0 = PASS/WARN, 1 = FAIL (escalate to CLV-full).

set -u

WF_DIR="$(cd "$(dirname "$0")/.." && pwd)"
cd "$WF_DIR" || exit 1
REPO_ROOT="$(cd "$WF_DIR/../.." && pwd)"

PROJECT="$(basename "$WF_DIR")"
echo "CLV-lite — $PROJECT"
echo "workflow root: $WF_DIR"
echo

fail=0
warn=0

# ── Check 0: inputs exist ────────────────────────────────────────────────────
# A check whose input is missing must FAIL, never pass. This check exists because the
# previous revision's Check 1 passed for six weeks by reading a file DEC-015 had deleted.
missing_inputs=""
for f in prd.md traceability.md findings.md decisions.md; do
  [ -r "$WF_DIR/$f" ] || missing_inputs="$missing_inputs $f"
done
[ -r "$REPO_ROOT/STATE.md" ] || missing_inputs="$missing_inputs STATE.md"
if [ -n "$missing_inputs" ]; then
  printf "Check 0 (inputs present):     FAIL — missing/unreadable:%s\n" "$missing_inputs"
  echo
  echo "Overall: FAIL — cannot validate against absent sources of truth."
  exit 1
fi
printf "Check 0 (inputs present):     PASS — prd/traceability/findings/decisions + STATE.md all readable\n"

# ── Check 1: Coverage ────────────────────────────────────────────────────────
# Every must/should REQ should have a TEST. An uncovered REQ is only tolerable when it is
# EXPLICITLY carried by a named future stage in STATE.md (## NEXT_GATE '### Stage' blocks) or
# by an explicit deferral recorded in its own traceability row. Anything else is UNACCOUNTED:
# a requirement nothing in the plan will ever pick up. That distinction is the point of the
# check — "not built yet" is a plan item, "not built and in no plan" is a defect.
awk -F'|' '/^\| REQ-[0-9]+ \| (must|should) /{id=$2;gsub(/^ +| +$/,"",id);print id}' prd.md \
  | sort -u > /tmp/clv_mustshould.$$

: > /tmp/clv_notest.$$
while read -r rid; do
  [ -n "$rid" ] || continue
  row=$(grep -m1 "^| $rid |" traceability.md 2>/dev/null)
  [ -n "$row" ] || continue          # missing rows are Check 1b's job, not this one
  # NOTE: column indices follow the 2026-08-30 schema, which inserted `Status (current)`
  # as column 3. DEC is now $5 and TEST is now $7.
  tcell=$(printf '%s' "$row" | sed 's/\\|/\x01/g' | awk -F'|' '{print $7}' | sed 's/^ *//; s/ *$//')
  if [ -z "$tcell" ] || [ "$tcell" = "—" ] || [ "$tcell" = "-" ]; then
    echo "$rid" >> /tmp/clv_notest.$$
  fi
done < /tmp/clv_mustshould.$$

accounted=""; unaccounted=""
while read -r rid; do
  [ -n "$rid" ] || continue
  # Accounted if a STATE.md future-stage block names it, or its own row records a deferral.
  # The "future stages" region is STATE.md's whole `## NEXT_GATE` section (the gate block plus
  # the stage table). Anchoring on a heading PREFIX rather than a specific heading text is
  # deliberate: the previous anchor was `### Stage `, and when the stage blocks became a table
  # the detector silently matched nothing and reported every uncovered REQ as unaccounted.
  if awk '/^## NEXT_GATE/{s=1;next} /^## /{s=0} s' "$REPO_ROOT/STATE.md" | grep -q "$rid"; then
    accounted="$accounted $rid"
  elif grep -m1 "^| $rid |" traceability.md | grep -qiE 'DEFERRED|accepted-with-rationale'; then
    accounted="$accounted $rid"
  else
    unaccounted="$unaccounted $rid"
  fi
done < /tmp/clv_notest.$$

if [ -z "$unaccounted" ] && [ -z "$accounted" ]; then
  printf "Check 1 (coverage):           PASS — every must/should REQ carries a TEST\n"
elif [ -z "$unaccounted" ]; then
  printf "Check 1 (coverage):           WARN — %d must/should REQ(s) without TEST, ALL carried by a named stage/deferral:%s\n" \
    "$(printf '%s\n' $accounted | grep -c .)" "$accounted"
  warn=$((warn+1))
else
  printf "Check 1 (coverage):           FAIL — %d must/should REQ(s) without TEST and NOT named by any future stage or deferral:%s\n" \
    "$(printf '%s\n' $unaccounted | grep -c .)" "$unaccounted"
  [ -n "$accounted" ] && printf "                              (additionally carried by a named stage/deferral:%s)\n" "$accounted"
  fail=$((fail+1))
fi
rm -f /tmp/clv_mustshould.$$ /tmp/clv_notest.$$

# ── Check 1b: Spine completeness ─────────────────────────────────────────────
# A requirement with NO traceability row is invisible to the per-id SSOT: no status, no tests,
# no commit, and no way for any later stage to notice it was skipped. This is the shape that
# let REQ-024 and REQ-025 sit outside the spine entirely.
#
# REPAIRED 2026-08-30: THE CHECK ONLY EVER LOOKED AT `REQ-NNN`. Every one of the 16
# `REQ-NF-*` ids was outside it, so a non-functional requirement could vanish from the spine
# and this check would still say PASS. They are now included, and the three GROUPED rows
# (`REQ-NF-Perf-001..003`, `REQ-NF-Sec-001..004`, `REQ-NF-Reliab-001..002`) are EXPANDED to the
# individual ids they stand for, so a group row covers exactly the ids it names and no more.
# The expansion is mechanical: `REQ-NF-<Family>-<lo>..<hi>` covers <lo> through <hi> inclusive,
# preserving the zero-padding. Anything else is taken literally.
no_row=$(python3 - "$WF_DIR" <<'PYEOF'
import re, sys, os
wf = sys.argv[1]

def ids_in(path, pattern):
    out = set()
    with open(path, encoding="utf-8") as fh:
        for line in fh:
            m = pattern.match(line)
            if m:
                out.add(m.group(1))
    return out

# prd.md declares one id per row: REQ-NNN or REQ-NF-Family-NNN.
prd = ids_in(os.path.join(wf, "prd.md"),
             re.compile(r"^\|\s*(REQ-(?:NF-[A-Za-z0-9]+-)?[0-9]+)\s*\|"))

# traceability.md may declare a RANGE in one row; expand it to the ids it covers.
trace = set()
row = re.compile(r"^\|\s*(REQ-(?:NF-[A-Za-z0-9]+-)?[0-9]+(?:\.\.[0-9]+)?)\s*\|")
with open(os.path.join(wf, "traceability.md"), encoding="utf-8") as fh:
    for line in fh:
        m = row.match(line)
        if not m:
            continue
        rid = m.group(1)
        if ".." not in rid:
            trace.add(rid)
            continue
        head, hi = rid.split("..")
        stem, lo = head.rsplit("-", 1)
        width = len(lo)
        for n in range(int(lo), int(hi) + 1):
            trace.add("%s-%0*d" % (stem, width, n))

missing = sorted(prd - trace)
print(" ".join(missing))
PYEOF
)
if [ -z "$(printf '%s' "$no_row" | tr -d ' ')" ]; then
  printf "Check 1b (spine complete):    PASS — every prd.md REQ (numeric AND REQ-NF-*, grouped rows expanded) has a traceability.md row\n"
else
  printf "Check 1b (spine complete):    FAIL — REQ(s) in prd.md with NO traceability row: %s\n" "$no_row"
  fail=$((fail+1))
fi

# ── Check 2: Provenance ──────────────────────────────────────────────────────
# Every LIVE REQ row's DEC cell is non-empty. `-PRIOR` rows are superseded provenance kept
# under the retention rule and are excluded BY NAME, reported below so the exclusion is visible.
orphans=$(sed 's/\\|/\x01/g' traceability.md | awk -F'|' '
  /^\| REQ-/ {
    id=$2; gsub(/^ +| +$/,"",id)
    if (id ~ /-PRIOR$/) next
    decs=$5; gsub(/^ +| +$/,"",decs)   # $5 since the Status column was inserted at 3
    if (decs == "" || decs == "—" || decs == "-") print id
  }')
excluded=$(grep -oE '^\| REQ-[A-Za-z0-9-]+-PRIOR' traceability.md | sed 's/^| //' | tr '\n' ' ')
if [ -z "$orphans" ]; then
  printf "Check 2 (provenance):         PASS — every live REQ row has a DEC"
  [ -n "$excluded" ] && printf " (excluded superseded provenance row(s): %s)" "$excluded"
  printf "\n"
else
  printf "Check 2 (provenance):         FAIL — live REQ(s) with empty DEC cell: %s\n" "$(echo $orphans)"
  fail=$((fail+1))
fi

# ── Check 5: Cycle finding closure ───────────────────────────────────────────
# THIS CHECK NO LONGER CONTAINS AN ALGORITHM. It RUNS the canonical one.
#
# It used to hold an awk one-liner that was a hand-copy of a second awk one-liner pasted into
# findings.md, and the header of this file carried a note asking the next editor to keep them in
# sync by hand. Two copies of a predicate are two predicates. Both are gone: the sole
# implementation is scripts/clv_findings.py, findings.md now DESCRIBES it instead of restating
# it, and this block invokes it and forwards its verdict.
#
# The canonical implementation FAILS SAFE where its predecessor failed OPEN. A row it cannot
# parse, a severity it cannot read, a disposition it cannot read, and a severity nobody has set
# are each a hard failure now, because each is the ABSENCE of a verdict rather than a clean one.
# The predecessor counted 28 unparseable rows as "reported, not blocking" and passed over them —
# several of those rows begin `**BLOCKING — …**`.
c5_out=$("$WF_DIR/scripts/clv_findings.py" --findings "$WF_DIR/findings.md" --quiet 2>&1)
c5_rc=$?
if [ "$c5_rc" -eq 0 ]; then
  printf "Check 5 (finding closure):    PASS — every row readable, nothing outstanding\n"
else
  printf "Check 5 (finding closure):    FAIL — %s\n" "$(printf '%s' "$c5_out" | sed -n 's/^RESULT: FAIL — //p')"
  printf '%s\n' "$c5_out" | sed -n '/^  MALFORMED/,/^  OK /p' | sed 's/^ */                              /'
  printf "                              per-row detail: scripts/clv_findings.py --limit 0\n"
  fail=$((fail+1))
fi

# ── Check 6: Ledger integrity ────────────────────────────────────────────────
# Every DEC referenced in the ledger must be reachable. Two tiers, because they are different
# defects: a DEC that appears NOWHERE is drift (FAIL); a DEC that is declared in the decision
# INDEX but has no entry section is a KNOWN, deliberately-unresearched allocation (WARN, named
# — it stays visible and it is owed work, but a reader following the citation does find its
# status). DEC-033 is the live example of the second tier.
referenced_decs=$(grep -rhoP '(?<![:a-zA-Z])DEC-[0-9]+(?![0-9A-Za-z])' --include='*.md' . 2>/dev/null | sort -u)
entry_decs=$(grep -oE '^## DEC-[0-9]+' decisions.md | sed 's/^## //' | sort -u)
index_decs=$(grep -oE '^\| DEC-[0-9]+' decisions.md | sed 's/^| //' | sort -u)
declared_decs=$(printf '%s\n%s\n' "$entry_decs" "$index_decs" | grep -c . >/dev/null; printf '%s\n%s\n' "$entry_decs" "$index_decs" | sort -u)
undeclared=$(comm -23 <(printf '%s\n' "$referenced_decs") <(printf '%s\n' "$declared_decs") | tr '\n' ' ')
index_only=$(comm -23 <(printf '%s\n' "$index_decs") <(printf '%s\n' "$entry_decs") | tr '\n' ' ')
if [ -n "$(printf '%s' "$undeclared" | tr -d ' ')" ]; then
  printf "Check 6 (ledger integrity):   FAIL — DEC(s) referenced but declared NOWHERE in decisions.md: %s\n" "$undeclared"
  fail=$((fail+1))
elif [ -n "$(printf '%s' "$index_only" | tr -d ' ')" ]; then
  printf "Check 6 (ledger integrity):   WARN — DEC(s) in the decision index with NO entry section (allocated, not researched): %s\n" "$index_only"
  warn=$((warn+1))
else
  printf "Check 6 (ledger integrity):   PASS — every referenced DEC has an entry section\n"
fi

# ── Overall ──────────────────────────────────────────────────────────────────
echo
if [ "$fail" -gt 0 ]; then
  echo "Overall: FAIL ($fail check(s) failed, $warn warned) — escalate to CLV-full and drill named artifacts."
  exit 1
elif [ "$warn" -gt 0 ]; then
  echo "Overall: WARN ($warn) — informational; CLV-full recommended at next phase exit."
  exit 0
else
  echo "Overall: PASS"
  exit 0
fi
