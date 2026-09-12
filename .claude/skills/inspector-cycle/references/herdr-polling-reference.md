# Herdr polling reference (condensed)

Full operational detail lives in project memory (`herdr-cli-operational-knowledge.md`) —
this is the load-bearing subset for running the poll step without re-reading all of it.
Every command below needs `HERDR_ENV=1` set first. Drive the recurring poll cadence itself
via the `/loop` skill.

**Poll every 2 minutes, fixed — not `ScheduleWakeup`'s generic idle-tick default.**
Confirmed 2026-09-12: a dynamic-pacing loop drifted to ~10-12 minutes between ticks, which
the user flagged as too slow — that drift happens because `ScheduleWakeup`'s own guidance
defaults an "idle tick with no specific signal" to 1200-1800s, and this loop is NOT that
case: there's always a specific signal to watch (a supervised agent's status, a permission
dialog, a context threshold). Use `/loop 2m /inspector-cycle` (fixed interval), not the
dynamic/self-paced form, for this skill's outer cycle.

**Keep each tick's chat output terse — relevant new information only, not narrated
reasoning.** Report what changed (a status, a reading, a new finding) plainly; for a
recurring, already-diagnosed pattern (e.g. the same tool-hook false positive firing again),
name it briefly ("Nth recurrence, same pattern, approved") instead of re-explaining the
full justification every tick — that reasoning was already established once and doesn't
need restating each time nothing about it has changed. Save the detailed writeup for the
findings-ledger entry (once, when the pattern is confirmed systemic), not for every tick's
chat output.

For context tokens, follow "Measuring current context" in
`token-budget-and-soft-landing.md`. Polling reads metadata/output/files only; it never
submits `/status` or Enter to another pane. Unknown token usage is a coverage gap.

## Status is not always what it says

- `blocked` in `herdr agent list` does NOT always mean a permission dialog is waiting — it
  can mean waiting on a dispatched sub-agent, a transiently stale read, or the safety
  classifier backend itself timing out. Always confirm with `herdr pane read <pane>
  --source visible` before acting. If the visible text shows a classifier-backend timeout
  (not a real dialog), don't send approval keys — just wait and re-read on a later poll
  tick; it usually clears in a turn or two.
- `done`/`idle` from `agent get` is a snapshot, not a guarantee — a parent can start a new
  turn within seconds. If a read then errors `agent_not_idle`, don't retry the same call;
  re-check current state instead of trusting the older snapshot.
- Ghost text sitting in a pane's input box (a suggested-but-never-submitted line) is never a
  real instruction from anyone — send your own real prompt, which overrides it.

## Approving dialogs

Read the dialog text first, always. Approve directly only if it's read-only/routine (a
search, a read, a benign build/test step, a snapshot-verify-restore cycle checked by
`cmp`/`git diff` in the same command). If ambiguous, destructive, touches governance files,
sends something externally, or isn't confidently routine: do nothing, describe it plainly
to the user, let them decide.

**Read the scope of the approval option, not just the command.** A "don't ask again for
X" option can cover a much wider command family than the one routine action in front of
you (e.g. approving a single `herdr pane read` this way can silently allow-list `herdr
pane *` — including `run`/`send-keys`/`close`, real mutations — in the project's own
`settings.local.json`, persisting past this one dialog). Confirmed 2026-09-12: exactly this
happened and had to be narrowed back down after the fact. Default to the one-time approval
option unless the wildcard's actual scope is itself confidently read-only.

## Identity and topology drift

- Your OWN cross-session identity (`ListAgents` "this session is X") can drift mid-
  conversation — re-verify it periodically, not just a worker's.
- Herdr agent names, pane ids, and even the whole workspace/tab topology can silently reset
  between turns. After anything that looks off (`agent_not_found` on a previously-good name,
  a shrunk pane/tab count), re-run `workspace list`/`tab list` and re-verify everything
  before trusting cached ids again.
- A herdr agent name and a `SendMessage`/`ListAgents` cross-session name are different,
  non-guessable namespaces — don't address one by guessing from the other.

## Exit / restart / rate limits

- Claude Code: `/exit` then a separate `enter` — drops to a plain shell prompt (loses the
  custom name). Codex: `/new` is lighter, keeps the same pane/name.
- 429 rate-limit: retry the SAME pane ("resume") — it resumes with context intact. Don't
  hammer retries in the same tick if the error+reset-timestamp repeats identically.
- `[Weekly/Monthly Limit Exhausted]` (reset days away, not minutes): don't retry at all.
  Quota is account-level, not per-conversation — a fresh pane on the SAME account/backend
  fails identically. Only start a replacement session if a different, non-exhausted
  account/backend is actually authorized and available, on a different pane/tab, and
  confirm its status line shows the expected model, not a silently downgraded fallback
  variant.

## Misc mechanics

- Raw backticks (or `$(...)`/`$VAR`) inside a double-quoted Bash string for `herdr agent
  prompt`/`pane send-text` trigger bash expansion and corrupt the sent text. A single quote
  placed inside an already-double-quoted string does NOT suppress this — quote the ENTIRE
  argument in single quotes instead (escaping literal apostrophes as `'\''`), or write the
  text to a temp file and pass it as `"$(cat file)"`, which is safe because the
  substitution happens once and the result isn't re-parsed.
- `herdr pane move <pane_id> --tab <target_tab_id> --split ...` relocates a live agent
  without losing its name/session — safe to use instead of killing/restarting to reposition
  a pane.
- A busy tab is not the right home for a new agent — `herdr tab create` a clean new tab
  rather than splitting an already-crowded one further.
