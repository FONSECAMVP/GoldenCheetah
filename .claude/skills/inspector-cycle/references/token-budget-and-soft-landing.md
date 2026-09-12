# Token budget and soft-landing

## Two separate thresholds — don't conflate them

- **Supervised agents (builder, reviewer, investigator, token-monitor): 250k hard budget.**
  Past this, start the soft-landing procedure below.
- **The Inspector's own context: ~300-350k warn threshold.** Separate, higher, and the
  restart-of-self is structurally different (see "Self-refresh" below) — never apply the
  250k worker number to yourself.

## Soft-landing procedure (for builder / reviewer / investigator / token-monitor)

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
   - token-monitor: flush its current status snapshot to the Inspector first.
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

## NO SCRIPTS, ever

Do not attempt to automate any part of this (a detached/background process that waits for
a pane's PID to exit and restarts it, `setsid`/`nohup`/`disown`, asking a peer session to run
it for you). This has been confirmed blocked outright by the harness's own safety
classifier as a self-persistence pattern, and separately confirmed by the user as wrong in
principle, not just blocked in practice. Every soft-landing is a manual, deliberate action
each time.

## Self-refresh (Inspector's own context, not a supervised agent)

You cannot self-execute your own restart the way you refresh a worker — there's no
controlling process left once you exit. At ~300-350k of your own tokens:
1. Recognize the threshold and confirm you're at a safe stopping point (real, substantive
   work, not deferral).
2. Compose two ready-to-use prompts for the user: a handoff/context briefing (current
   PIDs, stage/atomic-unit progress, what's independently verified vs. pending, known
   findings not yet logged) and a fresh loop-kickoff prompt refined with anything learned
   this cycle.
3. Present both, then issue your own `/exit` — the user opens the fresh session with them.
   That is the complete self-refresh, not a fallback.
