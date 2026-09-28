#!/bin/bash
# Inspector event-driven wake. Run as a BACKGROUND Bash task (run_in_background:
# true) as the LAST action of every Inspector turn. Blocks server-side at zero
# token cost until a supervised agent reaches idle/done/blocked (or the heartbeat
# fires), prints ONE compact status block, and exits -- the exit re-invokes the
# Inspector. Handle the wake, run one cycle pass, then re-arm this script.
#   $1 = active heartbeat FLOOR ms, while any agent is working (default 120000 =
#        2 min). The real wait stretches above the floor by measured headroom:
#        3 min when every working pane is under 85% of its context threshold,
#        5 min under 60%. Any unknown reading pins the wait at the floor --
#        never sample slower than you can intervene.
#   $2 = settled heartbeat ms, while all agents are settled (default 300000 = 5 min;
#        pass shorter, e.g. 120000, to recheck a wait-and-see blocked agent)
#   --stamp = maintenance mode, not a wake: compute the fingerprint and record it
#        as self-authored (KNOWN_FP), then exit. The Inspector runs this after
#        every ledger write or commit (SKILL.md steps 6-7) so its own edits read
#        as SELF, not NEW, on the next wake.
#   ARM IT WITH NO ARGUMENTS. Passing both values explicitly overrides these defaults,
#   so editing them here has no effect while the call site still spells them out --
#   that is a real trap: you change the number, nothing changes, and the script looks
#   like it ignored you.
#   SELF_PANE = Inspector's own pane id (optional; enables the self context read)
#   WAKELOG   = append every block to this log (default /tmp/insp_wake.log); the
#               Inspector reads the tail on demand instead of restating readings
#   KNOWN_FP  = self-authorship fingerprint stamp (default /tmp/insp_known_fp),
#               written by --stamp; a wake fingerprint equal to it tags SELF
#   SEC_STATE = per-section hash store for change-only printing (default
#               /tmp/insp_wake_state). Static sections (gate markers, hold/scope,
#               ledger budgets) print in full on change and collapse to one
#               UNCHANGED line otherwise; the full copy of EVERY block, changed
#               or not, is always in the wake log.
# Every block ends with a FINGERPRINT line: one hash over STATE.md + the 3 ledgers
# + repo HEAD (the dirty-set is deliberately excluded -- builder source edits don't
# change what step 2 reads), tagged SAME / SELF / NEW. SAME = identical to the
# previous block's fingerprint. SELF = different, but equal to the KNOWN_FP stamp
# (the Inspector's own writes/commit). NEW = neither -- a foreign change. SAME or
# SELF => skip the step-2 tiered ledger re-read (see
# references/herdr-polling-reference.md, "Change fingerprint").
# Checkout root. Overridable so the sandbox fixture can exercise this script
# unmodified; production leaves it unset and gets the real checkout.
GC="${GC:-/media/andy/TOSHIBA EXT/Backup2/Documents/GoldenCheetah}"
S="$GC/.claude/skills/inspector-cycle/scripts"
WAKELOG="${WAKELOG:-/tmp/insp_wake.log}"
KNOWN_FP="${KNOWN_FP:-/tmp/insp_known_fp}"
SEC_STATE="${SEC_STATE:-/tmp/insp_wake_state}"
LEDGERS=("$GC/STATE.md" "$GC/.claude/workflow-garminconnect/traceability.md" \
         "$GC/.claude/workflow-garminconnect/decisions.md" \
         "$GC/.claude/workflow-garminconnect/findings.md")

compute_fp() {
  { stat -c '%n %s %Y' "${LEDGERS[@]}" 2>/dev/null
    git -C "$GC" rev-parse HEAD 2>/dev/null; } | sha256sum | cut -c1-16
}

# --stamp: record the current fingerprint as self-authored, then exit. Not a wake:
# nothing is appended to the wake log and no roster is required.
if [ "${1:-}" = "--stamp" ]; then
  fp="$(compute_fp)"
  printf '%s\n' "$fp" > "$KNOWN_FP"
  echo "STAMPED $fp -> $KNOWN_FP"
  exit 0
fi

ACTIVE_MS="${1:-120000}"
SETTLED_MS="${2:-300000}"

# Whole block goes to stdout AND the wake log; coverage history lives in the file.
exec > >(tee -a "$WAKELOG") 2>&1

# Re-resolve panes by agent NAME every run -- topology drifts. Fail loudly on
# unresolved roles; never fall back to remembered pane ids.
LIST=$(herdr agent list) || { echo "WAKE_ERROR: herdr agent list failed"; exit 1; }
eval "$(printf '%s' "$LIST" | python3 -c '
import sys, json
d = json.load(sys.stdin)
roles = {"BUILDER": "", "REVIEWER": "", "INVESTIGATOR": ""}
for a in d["result"]["agents"]:
    n = (a.get("name") or "").lower()
    if "builder" in n: roles["BUILDER"] = a["pane_id"]
    elif "review" in n: roles["REVIEWER"] = a["pane_id"]
    elif "investigat" in n: roles["INVESTIGATOR"] = a["pane_id"]
missing = [k for k, v in roles.items() if not v]
if missing:
    print("MISSING=\"" + ",".join(missing) + "\"")
else:
    for k, v in roles.items(): print(f"{k}={v}")
')"
if [ -n "${MISSING:-}" ]; then
  echo "WAKE_ERROR: unresolved roster roles: $MISSING"
  echo "Rename the roster agents so each name contains builder/reviewer/investigator,"
  echo "then re-arm. Live agents now:"
  printf '%s' "$LIST" | python3 -c 'import sys,json
d = json.load(sys.stdin)
for a in d["result"]["agents"]:
    print("  %-26s %-7s %s" % (a.get("name") or a["pane_id"], a["pane_id"], a["agent_status"]))'
  exit 1
fi
PANES=("$BUILDER" "$REVIEWER" "$INVESTIGATOR")

# Only wait on agents currently WORKING -- a wait on an already-settled pane
# returns instantly and spins the wake loop. Restricted to the three ROSTER panes:
# the Inspector's own pane is always "working" while it runs, so counting it here
# both hides the all-settled branch and makes the wake fire on the Inspector's own
# turn-end instead of on a supervised agent.
WORKING=()
while read -r pane status; do
  [ "$status" = "working" ] || continue
  for p in "${PANES[@]}"; do
    if [ "$pane" = "$p" ]; then WORKING+=("$pane"); break; fi
  done
done < <(printf '%s' "$LIST" | python3 -c '
import sys, json
d = json.load(sys.stdin)
for a in d["result"]["agents"]:
    print(a["pane_id"], a["agent_status"])')

# Resolve the live agent process (kind + pid) behind a pane. Empty output = unknown.
agent_pid() { # $1 pane -> echoes "<kind> <pid>"
  local info
  info=$(herdr pane process-info --pane "$1" 2>/dev/null)
  printf '%s' "$info" | python3 -c 'import sys,json
try:
    d = json.load(sys.stdin)
    for f in d["result"]["process_info"].get("foreground_processes", []):
        if f["name"] in ("claude", "codex"): print(f["name"], f["pid"]); break
except Exception: pass' 2>/dev/null
}

# Headroom-scaled active heartbeat: the wait may stretch above the floor only when
# EVERY working pane reports a comfortable context reading. Any unknown reading or
# unreadable pane pins the wait at the floor. Server-side reads, zero token cost.
headroom_ms() { # $@ working panes -> echoes the wait in ms
  local worst=0 pane kind pid j r
  for pane in "$@"; do
    IFS=' ' read -r kind pid <<<"$(agent_pid "$pane")"
    [ -z "${pid:-}" ] && { echo "$ACTIVE_MS"; return; }
    if [ "$kind" = "claude" ]; then
      j=$(python3 "$S/claude_context.py" --pid "$pid" 2>/dev/null)
    else
      j=$(python3 "$S/codex_context.py" --pid "$pid" 2>/dev/null)
    fi
    r=$(printf '%s' "$j" | python3 -c 'import sys,json
try:
    d=json.load(sys.stdin); u=d.get("used_tokens"); t=d.get("threshold")
    print(u/t if u and t else "")
except Exception: print("")')
    [ -z "$r" ] && { echo "$ACTIVE_MS"; return; }
    awk -v a="$r" -v w="$worst" 'BEGIN{exit !(a>w)}' && worst=$r
  done
  awk -v w="$worst" -v f="$ACTIVE_MS" 'BEGIN{print (w<0.60?300000:(w<0.85?180000:f))}'
}

# Wait on the herdr children BY PID, never via bare `wait` / `jobs -p`. Those also
# pick up the `tee` from the stdout process substitution above, and that tee can
# never exit while this script holds the pipe open -- so a bare `wait` deadlocks the
# wake before it prints a single line. The roster-error path above exits earlier, so
# only the normal path was ever affected, which is why this stayed invisible.
wait_on() {
  local p
  wait -n "$@" 2>/dev/null
  kill "$@" 2>/dev/null
  for p in "$@"; do wait "$p" 2>/dev/null; done
}

WPIDS=()
if [ ${#WORKING[@]} -gt 0 ]; then
  # Someone is burning context: heartbeat floor is the context-sampling floor,
  # stretched above it only by measured headroom.
  WAIT_MS="$(headroom_ms "${WORKING[@]}")"
  for w in "${WORKING[@]}"; do
    herdr agent wait "$w" --until idle --until done --until blocked --timeout "$WAIT_MS" >/dev/null 2>&1 &
    WPIDS+=($!)
  done
else
  # All settled: nothing is growing. A transition to working (e.g. right after
  # the Inspector dispatches new work) wakes us early; one cheap extra wake per
  # dispatch is expected.
  for w in "${PANES[@]}"; do
    herdr agent wait "$w" --until working --timeout "$SETTLED_MS" >/dev/null 2>&1 &
    WPIDS+=($!)
  done
fi
wait_on "${WPIDS[@]}"

# Re-list AFTER the wait -- the pre-wait snapshot's statuses are stale.
echo "=== WAKE $(date '+%H:%M:%S') ==="
herdr agent list | python3 -c '
import sys, json
d = json.load(sys.stdin)
for a in d["result"]["agents"]:
    print("  %-26s %-7s %s" % (a.get("name") or a["pane_id"], a["pane_id"], a["agent_status"]))'

# Context reads: one-shot scripts against each pane's live PID. Mechanical file
# reads, never keystrokes.
ctx_for_pane() {
  local pane="$1"; shift
  local kind pid r
  IFS=' ' read -r kind pid <<<"$(agent_pid "$pane")"
  [ -z "${pid:-}" ] && { echo "  $pane unknown (no live agent process)"; return; }
  if [ "$kind" = "claude" ]; then
    r=$(python3 "$S/claude_context.py" --pid "$pid" "$@" 2>&1)
  else
    r=$(python3 "$S/codex_context.py" --pid "$pid" 2>&1)
  fi
  printf '  %s %s\n' "$pane" "$(printf '%s' "$r" | python3 -c 'import sys,json
try:
    d = json.load(sys.stdin); print(d.get("used_tokens"), d.get("status"))
except Exception: print("unknown")' 2>/dev/null)"
}

echo "--- context ---"
for pane in "${PANES[@]}"; do ctx_for_pane "$pane"; done
if [ -n "${SELF_PANE:-}" ]; then
  ctx_for_pane "$SELF_PANE" --threshold 210000
else
  echo "  self   unknown (export SELF_PANE when arming to enable)"
fi

# Change-only sections: full content on change (stdout, which tees to the wake
# log); one UNCHANGED line otherwise, with the full copy appended straight to the
# log so no information is ever lost. Hashes live in SEC_STATE.
mkdir -p "$SEC_STATE"
emit_section() { # $1 name; content on stdin
  local name="$1" content h sf prev
  content=$(cat)
  h=$(printf '%s' "$content" | sha256sum | cut -c1-12)
  sf="$SEC_STATE/$name"; prev=""
  [ -s "$sf" ] && prev=$(head -n1 "$sf")
  if [ -n "$prev" ] && [ "$prev" = "$h" ]; then
    printf '%s\n' "$content" >>"$WAKELOG"
    echo "  $name: UNCHANGED (full copy in wake log)"
  else
    printf '%s\n%s\n' "$h" "$(date '+%m-%d %H:%M')" >"$sf"
    printf '%s\n' "$content"
  fi
}

echo "--- live gate markers ---"
{ pgrep -x GoldenCheetah >/dev/null && echo "  GoldenCheetah RUNNING" || echo "  GoldenCheetah not running"
  echo "  activities: $(ls -1 ~/.goldencheetah/Andy/activities/ 2>/dev/null | wc -l) (newest: $(ls -t ~/.goldencheetah/Andy/activities/ 2>/dev/null | head -1))"
  echo "  tokens.json: $(stat -c '%s bytes %y' ~/.goldencheetah/Andy/config/garminconnect/tokens.json 2>/dev/null | cut -c1-30)"
  echo "  log lines: $(wc -l < ~/.goldencheetah/goldencheetah.log 2>/dev/null)"
} | emit_section gate-markers

echo "--- hold / scope ---"
{ cd "$GC" 2>/dev/null || { echo "  checkout unavailable"; exit 0; }
  h=$(git status --short src/Core/main.cpp)
  [ -z "$h" ] && echo "  main.cpp: CLEAN" || echo "  main.cpp: MODIFIED -> $h"
  echo "  builder-owned dirty paths:"
  git status --short --untracked-files=all -- src/Cloud unittests | sed 's/^/    /'
} | emit_section hold-scope

# Ledger budgets: caps from the tier model (state-and-tiers.md BUDGETS schema). A BREACH
# line is the librarian Job-3 (COMPACTION) dispatch trigger -- see SKILL.md step 2.
echo "--- ledger budgets ---"
{ budget() { # path cap_bytes label
    local s; s=$(stat -c%s "$1" 2>/dev/null || echo 0)
    if [ "$s" -gt "$2" ]; then echo "  BREACH $3 $((s/1024))kB cap=$(($2/1024))kB"
    else echo "  ok $3 $((s/1024))kB cap=$(($2/1024))kB"; fi
  }
  budget "$GC/STATE.md" 12288 state-cursor
  wikilines=$(wc -l < "$GC/WIKI.md" 2>/dev/null || echo 0)
  if [ "$wikilines" -gt 700 ]; then echo "  BREACH wiki ${wikilines}lines cap=700"; else echo "  ok wiki ${wikilines}lines cap=700"; fi
  maxrow=$(awk '{ if (length($0) > m) m = length($0) } END { print m+0 }' "$GC/.claude/workflow-garminconnect/findings.md" 2>/dev/null)
  if [ "${maxrow:-0}" -gt 200 ]; then echo "  BREACH findings-row ${maxrow}B cap=200B"; else echo "  ok findings-row ${maxrow}B cap=200B"; fi
  idx=$(awk '/^## DEC-/{exit} {n++} END{print n+0}' "$GC/.claude/workflow-garminconnect/decisions.md" 2>/dev/null)
  if [ "${idx:-0}" -gt 500 ]; then echo "  BREACH decidx ${idx}lines cap=500"; else echo "  ok decidx ${idx}lines cap=500"; fi
} | emit_section ledger-budgets

# Change fingerprint: ledger stats + HEAD. Last line of the block by design.
# Tag: SAME = identical to the previous logged block; SELF = equal to the KNOWN_FP
# self-authorship stamp (the Inspector's own ledger write/commit, stamped via
# --stamp); NEW = a foreign change -> the step-2 tiered re-read fires.
# The working-tree dirty-set stays deliberately EXCLUDED (see header): dirty paths
# print in hold/scope on change, full copy in every logged block.
FP="$(compute_fp)"
prev_fp="$(grep '^FINGERPRINT:' "$WAKELOG" 2>/dev/null | tail -n1 | sed 's/^FINGERPRINT: \([0-9a-f]*\).*/\1/')"
FP_TAG="NEW"
[ -n "$prev_fp" ] && [ "$prev_fp" = "$FP" ] && FP_TAG="SAME"
if [ "$FP_TAG" = "NEW" ] && [ -s "$KNOWN_FP" ] && [ "$(cat "$KNOWN_FP")" = "$FP" ]; then
  FP_TAG="SELF"
fi
echo "FINGERPRINT: ${FP:-unresolvable} · ${FP_TAG}"
