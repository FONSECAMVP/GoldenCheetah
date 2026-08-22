# Lessons Memory — getting incrementally smarter

The lessons memory is how the workflow stops repeating mistakes. It is part of the project
brain: a small, scoped, self-pruning set of **checkable rules** distilled from errors that
actually happened — and only errors that clear the **capture threshold** below. A qualifying
mistake seen once becomes an *advisory*; a mistake that recurs becomes a *guard* that blocks
the relevant operation until it's checked.

It is the mistake-side complement of the wiki MAP. The MAP says *what exists*; the lessons
say *what to verify so you don't err here again*. (Distinct from `wiki/architecture.md`'s
**Watch** list, which is about *system* runtime risks; lessons are about *process/agent*
mistakes.)

## Design rules (so it helps instead of bloating)

1. **A lesson is a checkable rule, not a story.** If you cannot state a precondition that
   would have prevented the mistake, it is not a lesson — it's just a fix. Don't capture it.
2. **Scoped, not global.** Every lesson carries tags (operation, phase, component, type).
   At the start of an operation you load only the lessons whose tags match — so the cost is
   "the few rules relevant right now," never the whole ledger.
3. **Index-first.** `lessons.md` has one line per lesson at the head; full entries are cold.
4. **Recurrence escalates.** Advisory → guard on the 2nd occurrence (or on explicit user
   escalation, or a high-cost single occurrence). Guards are hard preconditions.
5. **Bounded.** Near-duplicate mistakes merge into one lesson with a recurrence counter.
   Advisories untriggered for a long time and never recurred are archived (kept, not
   deleted). Guards are never auto-pruned.

## Lesson levels

| Level | Meaning | Behavior at operation time |
|---|---|---|
| **advisory** | seen once; plausibly recurs | surfaced as a one-line reminder when scope matches |
| **guard** | recurred, or user-escalated, or high-cost | **hard precondition** — the operation does not proceed until the guard's check passes |
| **archived** | dormant, never recurred | not loaded; resurrected only if its signature reappears |

## `lessons.md` — index line + cold entry

Index (head of file, one line each):
```
LSN-007 | op:create-file type:duplication | guard | recur:3 saves:5 miss:0 | check WIKI MAP before mkdir
LSN-011 | op:decision type:cascade-miss   | advisory | recur:1 saves:0 miss:0 | flag dependents when DEC reversibility=expensive
```

Cold entry (below the index / in archive once dormant):
```
## LSN-007
sig:    create-file / duplication / directory      # the merge key
level:  guard      since:P1   recur:3   saves:5   miss:0
tags:   op:create-file, type:duplication
trigger:about to create a file or directory
mistake:created a folder that already existed under a different assumed path
rule:   target path MUST be absent from WIKI MAP; if MAP lists it, open the existing file
check:  look up the target in WIKI MAP → must resolve to one canonical entry
origin: VAL-006 FAIL + user correction
history:P1 captured · P2 recurred → promoted guard · P3 recurred
```

The **sig** (signature) is a normalized `op / failure-class / object` key. New captures with
a matching sig increment an existing lesson instead of creating a duplicate — this is what
keeps the memory bounded and makes it *incremental* rather than ever-growing.

## The loop (capture → generalize → surface → verify → escalate → prune)

### 1. Capture threshold (evaluated after the repair passes)

**Capture or promote a lesson only when at least one is true:**
- the mistake **recurs**;
- it **crosses component or project boundaries**;
- it reveals a **defective workflow rule or mechanism**;
- it creates a **material security, data-loss, or irreversible-operation risk**.

**A first-occurrence local implementation or harness bug is a repair, not automatically a
lesson.** Fix it, record it in the normal test/commit report (or as one finding line if a
paper trail is needed), and move on. Manufacturing a permanent rule out of a one-off typo
inflates every registry it touches and buys nothing.

Detection events are *candidates*, not automatic captures — a CLV FAIL, a user correction, a
failed create because the target exists, a cascade miss, scope creep caught in REFACTOR or A3,
an A5 retrospective finding. Each is evaluated against the threshold above, **after the repair
passes** — never while a gate is red.

A **failed gate may update one active blocker line** — the finding and its `STATE.BLOCKING`
entry with its BLOCKS effects — **without allocating a new lesson or expanding any registry.**

At capture: compute the sig. If a lesson with that sig exists → `recur++`, re-evaluate
level (advisory→guard on recur≥2), append to `history`. Else → create a new advisory.
Always write the **rule** and **check**; if you can't, don't capture (rule 1).

### 2. Surface (pre-flight, before an operation)
Before running an operation, load the lessons tagged for it (Tier-1, scoped). Assemble the
**guards** into a tiny pre-flight checklist and verify each *before* acting. Advisories are
shown as reminders. Typical scopes:
- before **create-file** → `op:create-file` guards (the anti-duplication check)
- before a **decision** → `op:decision` guards (e.g., always flag expensive-reversibility
  dependents)
- before a **phase exit / deploy** → `phase:` and `op:clv` guards
- when touching a **component** → `component:Cn` guards

### 3. Verify
If a guard's check would fail, **stop and correct first** — that is the whole point. Then
record the outcome:
- guard prevented the mistake → `saves++` (the memory is working)
- the mistake happened anyway, despite the guard → `miss++` (the guard is too weak)

**Mechanism-enforced guards are harvested too:** whenever the deterministic hook denies or
asks in-session, the orchestrator increments the linked lesson (deny that stopped a real
clobber → `saves++`; deny of a LEGITIMATE operation → `miss++` on the guard AND a mechanism
finding) as a byproduct. Without this, a hook can fire for months while its lesson reads
`saves:0` and the loop never closes.

**A guard's false positive is a mechanism bug (LSN-036).** When a deterministic guard
denies an operation you believe is legitimate: never work around it (a bypass invented once
gets reused on the next GENUINE catch). Reproduce the denial in isolation, classify it,
record it as a finding + `miss`, and fix the mechanism. **Loosening any guard requires a
two-directional behavior matrix**: genuine violations still denied AND the legitimate case
now passing. **And the matrix must be re-verified after every skill update**: a reinstall
replaces the deterministic mechanisms wholesale, so a fixed false positive can RETURN from
an update with zero new code (field-proven: LSN-036 recurred exactly this way). Post-update
routine: re-run the guard's behavior matrix — the bundled self-test now ships at
`scripts/guard_selftest.py`: `python3 .claude/skills/quality-gated-dev-workflow/scripts/guard_selftest.py`
(defaults to the sibling guard; pass the installed hook's path to test that copy instead) —
and re-apply any `--extra-hook` syncs, before trusting the guards again.

**Repair attempts are bounded.** An attempt counts only after both a material patch and an
executed reproducer; hook prompts, unavailable tools, infrastructure failures, and command
typos do not consume an attempt. After **two unsuccessful material attempts**, return to the
user rather than allocating more process work.

**When adding a lesson that MANDATES an operation, check it against existing guards for
conflict first.** A conflicting or impossible workflow rule carries `{TASK:workflow-repair}`
initially. Add `CHECKPOINT:<slice>` only when it invalidates that slice's evidence. Add
`RELEASE` or `DEPLOY` only when a stated, demonstrated reason shows the conflict invalidates
the corresponding gate. A workflow-mechanism defect qualifies for lesson evaluation, but its
blocking effects are still proven rather than assumed — no effect is assigned automatically.

### 4. Escalate
A guard accumulating `miss` is not strong enough as a reminder. Escalation ladder:
advisory → guard → **propose a deterministic enforcement** (e.g., a pre-create hook that
refuses a write whose path is absent from the MAP, or a commit hook that rejects a commit
missing its `REQ/TEST/DEC` trailer). Surface the proposal to the user; instructions that
keep getting ignored should become mechanism.

### 5. Prune
On phase close, move advisories with `recur:1` and no recent trigger to
`archive/lessons-dormant.md` (one line each). Guards stay active. This keeps the live set
to "what's actually biting us lately."

## Portable vs. project lessons

Most lessons are project-local (live in the project's `lessons.md`). A lesson about the
*workflow itself* (not this codebase) can be tagged `scope:portable`; on project
bootstrap/migration, portable lessons are copied into the new project's `lessons.md` as
guards, so hard-won workflow rules travel forward instead of being relearned. Keep portable
lessons few and high-value — they are the workflow's accumulated wisdom, not a junk drawer.

## WIKI + STATE wiring

- `WIKI.md` REGISTRIES gains:
  `LSN 001–012  active:9 guards:4  lessons.md   next:013`
  and a PAGES pointer: `lessons.md — checkable rules from past mistakes · read guards before the matching operation`.
- `STATE.md` need not list lessons; guards are loaded per-operation by tag. If a guard is
  currently blocking work, it appears in `STATE.BLOCKING` as a finding id with its BLOCKS
  effects, like any other — scoped to what it actually stops.

## Why this is "incremental"

Each error makes the next session smarter by exactly one checkable rule, deduped by
signature and weighted by recurrence. The memory measures its own effectiveness
(saves/misses) and escalates the rules that aren't working — so the system converges toward
"the mistakes we used to make are now things we automatically check," at a cost of a few
scoped lines per operation rather than an ever-growing log.

## Enforcement endpoint — the bundled deterministic guard

The escalation ladder's last rung (advisory → guard → **mechanism**) is realized for the
highest-recurrence mistake — re-creating files/folders that already exist (LSN-007) — by a
bundled `PreToolUse` hook: `scripts/anti_duplication_guard.py`.

What it does, deterministically, before every `Write`/`Edit`/`MultiEdit`/`Bash`:
- **deny** — `mkdir` of an existing directory; `touch`, `>`/`>>` redirect, or `tee` over an
  existing file; `cp`/`mv` onto an existing *file*; `ln`/`ln -sf` over an existing path (the
  clobber/duplication family). Deny outranks ask when a command has several targets.
- **ask** — a no-op `mkdir -p` on an existing dir (a "you may be lost, re-read WIKI" nudge),
  or creating a new file/dir (via any of the above verbs) that isn't yet in the WIKI MAP
  (register it as a byproduct).
- **silent passthrough** — edits of existing files, writes under a mapped directory, `cp`/`mv`
  *into* an existing directory, paths outside the project root, and any command with no
  creation intent. If no `WIKI.md` is found, MAP-registration asks are suppressed but the
  clobber deny still fires. Malformed input never blocks the turn.

Known remaining gaps (intentionally not parsed, low frequency / high ambiguity): `install -d`,
and tool-scaffolders that emit known artifacts (`npm init`, `cargo init`, framework
generators). These surface as `miss` events to widen later if they recur.

It is wired into `.claude/settings.json` so it is active for the project. The skill bundles
a one-command installer that does this for you:

```
python3 scripts/install_hook.py [project_dir]   # default: current directory
```

The installer copies `anti_duplication_guard.py` into `<project>/.claude/hooks/` and adds
(idempotently — safe to re-run) this block to `<project>/.claude/settings.json`:

```json
{
  "hooks": {
    "PreToolUse": [
      { "matcher": "Write|Edit|MultiEdit|Bash",
        "hooks": [ { "type": "command",
                     "command": "python3 \"${CLAUDE_PROJECT_DIR}/.claude/hooks/anti_duplication_guard.py\"" } ] }
    ]
  }
}
```

**Project-owned mechanisms (extension point):** when a lesson escalates to a project-local
deterministic mechanism (a drift lint, a custom gate), keep its canonical source in the
PROJECT (e.g. `scripts/your_lint.py`) and sync it with
`python3 .../install_hook.py --extra-hook scripts/your_lint.py .` — never patch the skill's
installer to add it: skill updates overwrite skill-owned scripts, and a local patch to one
is a divergence that the next update silently destroys. The project owns the extra hook's
`settings.json` wiring.

For an always-on personal install, add the same block to `~/.claude/settings.json` with an
absolute path to the script. Restart Claude Code (or `/hooks` to reload) after installing.
Note the quotes around `${CLAUDE_PROJECT_DIR}` — they are required so the hook works when the
project path contains spaces. Re-running `install_hook.py` auto-upgrades an older unquoted
command in place.

The script is stdlib-only Python 3 (no `jq`, no packages). Requires `python3` on PATH;
verify with `python3 --version`. Disable temporarily via `/hooks` in Claude Code or
`"disableAllHooks": true` in settings. When the hook denies or asks, its reason is ground
truth — open the existing path or register the new one; never work around it. Each time it
prevents a duplication, that's a `saves++` for LSN-007; if a duplication slips through anyway
(e.g. an unusual creation syntax the parser missed), record a `miss` and widen the parser.
