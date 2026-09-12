# Token budget and soft-landing

## Two separate thresholds — don't conflate them

- **Supervised agents (builder, reviewer, investigator): 250,000-token refresh threshold.**
  At or above this, start the soft-landing procedure below.
- **The Inspector's own context: ~210k warn threshold.** A separate number from the 250k
  worker budget, not simply scaled from it (correction 2026-09-12: an earlier draft of this
  skill said ~300-350k — that was wrong; 210k is the real, confirmed warn point) — never
  apply the 250k worker number to yourself, and don't assume yours is the higher one just
  because you supervise. Refreshed by a different mechanism than a worker too (see
  "Self-succession" below). There is no token-monitor agent to catch this externally — read
  your own pane's PID with `scripts/claude_context.py --pid <your-pid> --threshold 210000`
  yourself, every poll tick, the same mechanical way you read the other 3 numbers.

## Measuring current context (read-only on every poll)

The number is the latest reported **current context**, not lifetime/billing usage.
Never send `/status` or `/context` into a supervised pane to measure it: these can queue
behind work, concatenate with existing input, or show no context count. Do not convert a
bare percentage, assume a model capacity, or treat `idle`/`done` as zero tokens. There is
no standing token-monitor agent for any of this — the Inspector runs these two one-shot,
read-only scripts itself, directly, every poll tick (including against its own pane); it is
a mechanical file read, not work worth spending a supervised LLM's own context on.

- **Claude Code panes** (builder, and the Inspector's own pane): `scripts/claude_context.py
  --pid <pid>` reads the process's own session transcript (`~/.claude/projects/<project>/
  <session-id>.jsonl`, located via the pane's live PID, never a remembered session id or a
  newest-by-mtime guess) and sums the latest assistant turn's `input_tokens +
  cache_creation_input_tokens + cache_read_input_tokens` — the same fields
  `~/.claude/statusline.sh` sums into its `tok Nk` figure (Claude Code 2.1.261's
  `context_window.total_input_tokens`). Output tokens are excluded, same as the footer.
  Sidechain (Task-tool subagent) usage events are skipped — they aren't the pane's own
  context. Pass `--threshold 210000` when reading the Inspector's own pane; the default
  `250000` applies to a supervised worker.
- **Codex panes** (reviewer, investigator): `scripts/codex_context.py --pid <pid>` reads the
  unique rollout file currently open by the pane's Codex process, then selects the latest
  `event_msg` / `token_count` record's `info.last_token_usage.total_tokens`. This includes
  cached context; do not subtract cached input or add reasoning/output again. Neither
  `info.total_token_usage` nor a session database's `tokens_used` is current context.

For each pane, get its current process identity without touching its input, then run the
matching reader:

```bash
herdr pane process-info --pane <live-pane-id>
python3 .claude/skills/inspector-cycle/scripts/claude_context.py --pid <claude-pid>   # builder / Inspector's own pane
python3 .claude/skills/inspector-cycle/scripts/codex_context.py --pid <codex-pid>     # reviewer / investigator
```

Use the `pid` of the `name: "claude"` or `name: "codex"` entry (as appropriate) in
`result.process_info.foreground_processes`; if there is no unique entry of the expected
kind, report `unknown`. Resolve it anew each tick and after `/new`/restart/soft-landing.
Never choose a session by modification time or cwd alone: multiple agents can share a cwd
and account. Run the reader from the checkout containing this skill, or use its absolute
path.

Both readers emit the same JSON shape: `used_tokens`, `session_id`, `source`, `sample_at`,
`threshold`, and `status`. `warn` means **used_tokens >= threshold**, including exactly
equal; `below_threshold` describes the last sample only. Exit 0 means the read succeeded
(even when warning); exit 2 and `status: unknown` mean no usable measurement. A new session
may have no transcript/rollout content yet until its first turn. Missing/ambiguous files,
incomplete records, and compaction without a new usage sample stay unknown; do not carry
the old session's count forward. An old timestamp in an idle, unchanged session is its last
reported count. If a working agent's sample stops advancing across polls, flag telemetry as
stale and do not claim it is safely below threshold. These are last-reported measurements,
not predictions of in-flight growth.

Both readers only read files and exit; neither polls in the background, sends keystrokes,
or performs a refresh. Continue the established `/loop` cadence, invoking the readers
yourself each tick. On a warning, start that session's soft-landing once, continue
coverage of the rest, and use the procedure below.

Field semantics: [Claude status-line documentation](https://code.claude.com/docs/en/statusline#context-window-fields).
Recheck a reader's field assumptions if the underlying transcript/rollout format changes.

## Soft-landing procedure (for builder / reviewer / investigator)

Never hard-kill an agent mid-work — a soft landing exists specifically because breaking an
in-flight process costs more to recover later than a short, controlled pause does now.

1. **WARN** at 250k, in advance, not after the fact. Tell the agent explicitly: stop
   starting anything new; finish only what's currently running to a safe stopping point.
   "Safe" is role-specific:
   - builder: finish the current RED-GREEN-REFACTOR step if it fits in remaining budget. If
     reaching a clean GREEN would exceed it, stop earlier and write down exactly what's
     mid-flight (files touched, last test status, what's unverified) instead of forcing
     completion — the next session resumes from that note, not from scratch or a guess.
   - reviewer: finish the current delta-check pass — don't leave a partial finding list.
   - investigator: finish the current isolated repro conclusion (nothing to leave clean in
     the real tree, since it never touched it).
2. **WAIT** for the agent to actually confirm the safe stopping point — read what it
   reports; don't guess from the outside, and don't treat a `done`/`idle` snapshot as
   permanent (a fresh turn can start again within seconds).
3. **EXIT and restart cleanly:**
   - Claude Code pane: `/exit` then a separate `enter` keypress — drops to a plain shell
     prompt; verify the old process is actually gone, THEN `herdr agent start <newname>
     --kind ... --pane <same-id>` re-establishes tracking (the old custom name is lost,
     re-rename if needed).
   - Codex pane: `/new` is lighter — resets context but keeps the SAME process/pane/agent
     name; there is no old process to verify gone and nothing to restart.
4. **Re-brief** the fresh agent using the briefing shape in `agent-roster-and-dispatch.md`
   (mirror its own first-ever prompt), carrying forward: current stage/atomic unit, last
   independently-validated state, any findings not yet in the ledger, and the soft-landing
   note itself (what was in flight when it paused).

## No restart scripts

Do not attempt to automate any part of this (a detached/background process that waits for
a pane's PID to exit and restarts it, `setsid`/`nohup`/`disown`, asking a peer session to run
it for you). This has been confirmed blocked outright by the harness's own safety
classifier as a self-persistence pattern, and separately confirmed by the user as wrong in
principle, not just blocked in practice. Every soft-landing is a manual, deliberate action
each time.

## Self-succession (Inspector's own context, not a supervised agent)

A bare self-restart doesn't work — there's no controlling process left once you exit to
verify the old one died and start the new one, the way you do for a worker. **Self-
succession solves this by reversing who controls whom: spawn your successor FIRST, while
you're still alive to do it, then have the successor (fresh, under threshold, with full
agency) retire YOU — not the other way around.**

At ~210k of your own tokens:
1. Recognize the threshold and confirm you're at a safe stopping point (real, substantive
   work, not deferral) — same discipline as any soft-landing.
2. Spawn a successor: `herdr tab create` a clean new tab (never squeeze into a busy one),
   then `herdr agent start <new-name> --kind claude --pane <new-pane-id>`. **Name it by
   incrementing your OWN version suffix**, not an arbitrary label: the first Inspector in a
   project session is `garmin_inspector_v1_0`; each succession bumps the minor number
   (`v1_0` → `v1_1` → `v1_2` → ...) — herdr agent names only allow lowercase letters,
   digits, `-`/`_` (confirmed: a literal dot is rejected with `invalid_agent_name`), so the
   separator is an underscore, not a dot, even though it reads as "v1.0" conceptually. If
   your own current name has no version suffix (an ad hoc name, or you're the very first
   session in this lineage and were never renamed), treat yourself as `v1_0` and name the
   successor `v1_1` — don't invent an unrelated name or a bare incrementing integer.
3. Send the successor a SHORT rebirth prompt — NOT a context dump. It only needs to say,
   in substance: "You are `<new-name>`, a reborn Inspector succeeding `<old-name>`/
   `<old-pane-id>`, which hit its self-refresh threshold. Invoke the `inspector-cycle` skill
   now — its own step 0 (environment setup) and steps 1-2 (herdr, current stage) will fully
   set you up and orient you from live project state; you do not need your predecessor's
   conversation history for that. Once set up and oriented, retire your predecessor's pane,
   then continue the cycle. Report the succession to the user when done." No PIDs, no stage
   recap, no findings list — the cycle itself supplies all of that once invoked, by design
   (see "Successor's first actions" below).
4. WAIT and verify the successor actually comes alive and starts working (`herdr agent get
   <new-name>` transitions to `working`) before considering the handoff underway — don't
   assume the prompt landed just because the send succeeded.
5. The predecessor does NOT retire itself. That's the successor's job (step 5 below) —
   this is the one thing that makes self-succession possible where bare self-restart
   wasn't.
6. NO SCRIPTS, same rule as any soft-landing: every step here is a deliberate, manual herdr
   command issued by a live agent, never a detached background process.

## Successor's first actions (what a reborn Inspector must do, unprompted)

On receiving a rebirth prompt, before anything else:

0. **Environment setup — do this FIRST, before step 1.** A freshly-started pane defaults to
   whatever the harness's own defaults are, not necessarily what an autonomous supervisor
   needs — don't assume, set it explicitly:
   - Make switching to auto mode your very FIRST action (e.g. a trivial `herdr --version`
     or `herdr agent list` check) rather than drifting into it several unrelated actions
     later. A permission dialog requires a live keypress from whoever is still piloting the
     pane — you cannot inject that yourself — so this one dialog is the predecessor's to
     click through (it's still alive at this exact moment, per the self-succession design).
     Confirmed 2026-09-12: doing this as the deliberate first action took it down to ONE
     predecessor approval (the "switch to auto mode" option, offered directly on that first
     dialog) instead of three scattered ones from before. That's the realistic floor for
     this mechanism, not literal zero-touch — but it's a single, narrow, one-time click, not
     ongoing babysitting.
   - Confirm the running model matches the project's intended one (check the pane's own
     status line, or `~/.claude/settings.json`'s top-level `"model"` key) — don't assume a
     fresh pane inherited the right one; a prior session's `/model` switch or a rate-limit
     fallback does not carry forward automatically, and a silent wrong-model session is
     easy to miss until much later.
   - Resume direct context-usage script reads (`claude_context.py`/`codex_context.py`) for
     all 3 supervised agents AND your own new pane immediately, not after a gap — there is
     no standing token-monitor agent to re-brief; this is just you running the readers.
1. Invoke the `inspector-cycle` skill and run its own step 1 (herdr) and step 2 (current
   stage) immediately — do not wait for, or ask for, a manual briefing from the
   predecessor. The skill's whole design (re-derive every step fresh from live state, never
   trust a cached snapshot) is exactly what makes a cold Inspector self-orienting: reading
   `STATE.md`, the ledgers, and live herdr topology yourself tells you what's running and
   what the next atomic unit is, the same way it would after any ordinary topology reset.
2. Once you can state, from evidence you just read yourself, what's currently running and
   what's next — you are oriented. Re-verify the predecessor's pane id via `herdr agent
   list`/`workspace list` rather than trusting the rebirth prompt's claimed id blindly
   (topology can drift between the predecessor sending it and you checking).
3. Retire the predecessor the same clean way a supervised Claude Code agent is refreshed:
   `herdr pane send-text <pane> "/exit"` then `herdr agent send-keys <target> enter` —
   never a raw `herdr pane close` or kill. Verify the pane actually dropped to a plain
   shell prompt.
4. Only then resume the normal cycle at step 3 (delegate work) and step 4 (poll agents) —
   confirm all 3 supervised agents, plus your own pane, are actually having their context
   usage read again on the established cadence (not just the one-off check from step 0),
   driven via `/loop` per `herdr-polling-reference.md`, not left as a one-off orientation
   check. Report the completed succession to the user plainly —
   this is a first-of-its-kind mechanism, worth narrating even though it doesn't need
   permission (see `autonomy-boundary.md`: an internal, fully-reversible operational action
   like this is not a human-in-the-loop gate).
5. Ending a session that a user is actively, interactively attached to (rather than a
   background herdr pane with no live viewer) is the one exception worth a direct heads-up
   before retiring it — confirm with the user first in that specific case, since it
   severs their current interactive connection even though the session itself is
   resumable (`claude --resume <session-id>`).
