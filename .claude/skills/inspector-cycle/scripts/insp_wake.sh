#!/bin/bash
# Inspector event-driven wake. Run as a BACKGROUND Bash task (run_in_background:
# true) as the LAST action of every Inspector turn. Blocks server-side at zero
# token cost until a supervised agent reaches idle/done/blocked (or the heartbeat
# fires), prints ONE compact status block, and exits -- the exit re-invokes the
# Inspector. Handle the wake, run one cycle pass, then re-arm this script.
#   $1 = active heartbeat ms, while any agent is working (default 120000 = 2 min)
#   $2 = settled heartbeat ms, while all agents are settled (default 300000 = 5 min;
#        pass shorter, e.g. 120000, to recheck a wait-and-see blocked agent)
# ARM IT WITH NO ARGUMENTS. Passing both values explicitly overrides these defaults,
# so editing them here has no effect while the call site still spells them out --
# that is a real trap: you change the number, nothing changes, and the script looks
# like it ignored you.
#   SELF_PANE = Inspector's own pane id (optional; enables the self context read)
#   WAKELOG   = append every block to this log (default /tmp/insp_wake.log); the
#               Inspector reads the tail on demand instead of restating readings
# Every block ends with a FINGERPRINT line: one hash over STATE.md + the 3 ledgers
# + repo HEAD (the dirty-set is deliberately excluded -- builder source edits don't
# change what step 2 reads). Unchanged fingerprint + unchanged statuses since the last
# handled wake => skip the step-2 tiered ledger re-read (see
# references/herdr-polling-reference.md, "Change fingerprint").
# Checkout root. Overridable so the sandbox fixture can exercise this script
# unmodified; production leaves it unset and gets the real checkout.
GC="${GC:-/media/andy/TOSHIBA EXT/Backup2/Documents/GoldenCheetah}"
S="$GC/.claude/skills/inspector-cycle/scripts"
ACTIVE_MS="${1:-120000}"
SETTLED_MS="${2:-300000}"
WAKELOG="${WAKELOG:-/tmp/insp_wake.log}"
LEDGERS=("$GC/STATE.md" "$GC/.claude/workflow-garminconnect/traceability.md" \
         "$GC/.claude/workflow-garminconnect/decisions.md" \
         "$GC/.claude/workflow-garminconnect/findings.md")

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
  # Someone is burning context: heartbeat is the context-sampling floor.
  for w in "${WORKING[@]}"; do
    herdr agent wait "$w" --until idle --until done --until blocked --timeout "$ACTIVE_MS" >/dev/null 2>&1 &
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
  local info kind pid r
  info=$(herdr pane process-info --pane "$pane" 2>/dev/null)
  read -r kind pid <<<"$(printf '%s' "$info" | python3 -c 'import sys,json
d = json.load(sys.stdin)
for f in d["result"]["process_info"].get("foreground_processes", []):
    if f["name"] in ("claude", "codex"): print(f["name"], f["pid"]); break' 2>/dev/null)"
  [ -z "$pid" ] && { echo "  $pane unknown (no live agent process)"; return; }
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

echo "--- live gate markers ---"
pgrep -x GoldenCheetah >/dev/null && echo "  GoldenCheetah RUNNING" || echo "  GoldenCheetah not running"
echo "  activities: $(ls -1 ~/.goldencheetah/Andy/activities/ 2>/dev/null | wc -l) (newest: $(ls -t ~/.goldencheetah/Andy/activities/ 2>/dev/null | head -1))"
echo "  tokens.json: $(stat -c '%s bytes %y' ~/.goldencheetah/Andy/config/garminconnect/tokens.json 2>/dev/null | cut -c1-30)"
echo "  log lines: $(wc -l < ~/.goldencheetah/goldencheetah.log 2>/dev/null)"

echo "--- hold / scope ---"
cd "$GC" || exit 0
h=$(git status --short src/Core/main.cpp)
[ -z "$h" ] && echo "  main.cpp: CLEAN" || echo "  main.cpp: MODIFIED -> $h"
echo "  builder-owned dirty paths:"
git status --short --untracked-files=all -- src/Cloud unittests | sed 's/^/    /'

# Ledger budgets: caps from the tier model (state-and-tiers.md BUDGETS schema). A BREACH
# line is the librarian Job-3 (COMPACTION) dispatch trigger -- see SKILL.md step 2.
echo "--- ledger budgets ---"
budget() { # path cap_bytes label
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

# Change fingerprint: ledger stats + HEAD. Last line of the block by design.
# Deliberately EXCLUDES the working-tree dirty-set: step 2 re-reads ledgers, and builder
# source edits must not re-trigger it. Dirty paths stay visible in the hold/scope section
# above, which prints every wake regardless.
FP="$( { stat -c '%n %s %Y' "${LEDGERS[@]}" 2>/dev/null
        git -C "$GC" rev-parse HEAD 2>/dev/null; } | sha256sum | cut -c1-16)"
echo "FINGERPRINT: ${FP:-unresolvable}"
