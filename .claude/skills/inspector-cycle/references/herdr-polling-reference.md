# Herdr supervision reference

Use for Herdr monitoring. Apply [autonomy boundaries](autonomy-boundary.md) for
interventions and [context and handoff](token-budget-and-soft-landing.md) for
resource thresholds and session transfers. Read the [event appendix](herdr-events.md)
only when implementing, enabling, or diagnosing a watcher.

## Establish the session

1. Run the /herdr skill.

2. Verify inherited `HERDR_ENV=1` before control commands; do not set it to manufacture
managed-pane context. If absent, stop Herdr control and report the missing context.
Use explicit targets; never rely on another client's focused pane.

3. Capture `herdr api snapshot` and resolve supervised agents, panes, and your own
identity. Keep Herdr names and cross-session messaging identities separate.
Recheck your messaging identity periodically through that manager's own tools.

## Every cycle

Preferred: run `scripts/insp_wake.sh` as a background Bash task (`run_in_background: true`)
as the last action of every turn. It blocks server-side at zero token cost on `herdr agent
wait` until a supervised agent reaches idle/done/blocked, or the heartbeat fires (2 min while
any agent is working — the context-sampling floor, since Herdr supplies no
context-threshold event; 15 min when all are settled), then exits and re-invokes the
Inspector with one compact status block. Never end a turn without a background wake armed; a
forgotten re-arm silently stops all supervision. The script fails loudly on unresolved agent
names — rename the roster agents, do not edit remembered pane ids into it. Do not relax the
heartbeat merely because events are enabled; shorten it when context headroom requires
earlier intervention.

1. Read `herdr agent list`; compare live identities and statuses with the previous
   observation. Reconcile full topology on movement, replacement, lookup failure,
   reconnect, or inconsistent state. Use a fresh snapshot, or workspace/tab/pane
   lists for every supervised workspace. Do not limit discovery to your own tab.
2. Sample current context for workers, the Supervisor, and any monitor. Record
   session identity, usage, capacity, measurement source, and timestamp. Use
   available verified metadata or collectors; no collector scripts ship here.
   Mark missing/stale readings as coverage gaps. Do not infer counts from billing
   totals or percentages without a verified capacity. Apply the context reference's
   reserve and handoff rules; Herdr status events do not measure context growth.
3. Inspect new or unresolved blockers and collect results requiring validation.
   Record the next action, owner, and check time. Confirm any watcher is healthy.

**Change fingerprint (ledger re-read gate):** the wake block ends with a FINGERPRINT
line — one hash over the stat of `STATE.md` + the 3 ledgers and the repo HEAD. The
working-tree dirty-set is deliberately excluded: step 2 re-reads ledgers, and builder
source edits must not re-trigger it — dirty paths remain visible every wake in the
block's hold/scope section. Unchanged fingerprint + unchanged agent statuses since the
last wake you handled → skip the step-2 tiered ledger re-read and act on the block
alone. Full re-read stays mandatory on session start/resume, topology movement,
fingerprint change, or any commit. The fingerprint is recomputed by the wake script on
every wake, so silent ledger drift is still detected; what is skipped is only re-reading
files proven unchanged.

**Wake log:** every block is also appended to the wake log (`/tmp/insp_wake.log` by
default, override with `WAKELOG=`). Check times, coverage gaps, and unchanged readings
live in that file — read the tail on demand — instead of being restated in chat; the
Inspector's own context is the scarcest pane on the board.

Monitoring reads metadata, output, and files; it never sends `/status`, Enter, or
composer-clearing keys. Report meaningful changes, actions, and coverage gaps
tersely. Log a recurring diagnosis once; count repetitions without restating it.

## Interpret state and intervene

- `blocked`: read `herdr agent read <target> --source visible`. Confirm a current
  dialog before acting. Local backend timeouts and stale/sub-agent displays have
  resembled blockers; they are observations, not Herdr contracts. For a backend
  timeout without a dialog, wait and re-read; send no approval keys.
- `unknown` or disagreement: use `herdr agent explain <target> --json` when supported.
  Inspect the matching rule, evidence, manifest, and fallback/skip reason. Request
  this diagnostic selectively; do not load full rule traces every cycle.
- `idle`/`done`: these describe readiness and whether background completion was
  seen, not task acceptance or permission to reset. Validate the deliverable.
  If a history read returns `agent_not_idle`, recheck state or use `visible`;
  that error alone does not establish an identity change.

For approvals, read the operation, selected option, and full scope. Routine
one-time approval is permitted only within explicitly granted delegated authority
and the environment's rules. Otherwise ask for the missing authorization.
Ambiguous, destructive, governance-changing, or external actions require the
applicable authority. Prefer one-time approval; persistent allow-list changes
require separate authority. A `herdr pane *` allowance includes mutations.
Send only the keys needed for the authorized choice and verify the result.

Before prompting, verify the live occupant and composer. Suggestions are not
instructions; neither prompt submission nor Escape guarantees clearing existing
input. Use agent-specific controls only for an authorized intervention. Quote
literal shell arguments correctly, or use a structured client that serializes
JSON without embedding the payload in shell code.

## Recovery and optional events

After a pane move, use the returned pane ID or re-resolved live agent name. A
cross-workspace move changes the public ID; rebuild affected subscriptions.
An interrupted wait may report `agent_not_running` although the terminal survived.
Reconcile before retrying. Preserve focus and live sessions during authorized
layout changes; use a fresh tab when an approved new agent would overcrowd one.

Respect service retry times and resume the existing session when supported.
Do not repeat identical retries or create new sessions against exhausted account
quotas. Alternative accounts/backends require authorization; verify the expected
model. Before any refresh/exit, confirm the checkpoint and ownership using the
context reference, then verify identity and monitoring after resumption.

For incomplete results, check read limitations and current state. Idle history
reads may scroll the application; routine monitoring uses passive `visible` or
`detection` reads. Rows that leave the alternate screen never enter host scrollback,
so a larger `--lines` cannot recover a lost reply head.

If a reply is incomplete, fall back to a file **only** on the terms in
`message-transport.md`: re-prompt for the previous reply verbatim at
`/tmp/insp-exchange/<unit>.md`, then read that path. Never request file output in the
initial prompt, never accept a path the brief did not declare, and never treat the
resulting file as a record — it is a one-cycle transport buffer, and the ledgers are
the only durable state.

Events supplement this cycle only after a watcher and host notifications are
implemented and verified. Track its job, avoid duplicate watchers, bound output,
and reconcile each notification against current state. On timeout, error, or
missing capability, retain polling and report the coverage limit.
