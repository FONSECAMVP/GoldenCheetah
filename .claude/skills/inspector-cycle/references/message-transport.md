# Message transport — how a brief and a reply actually move

Herdr's message channel is two commands. They are the ONLY sanctioned way work and
results move between the Inspector and a supervised agent:

```bash
herdr agent prompt <target> <TEXT> --wait --timeout <ms>          # Inspector -> agent
herdr agent read   <target> --source recent-unwrapped --lines 200 # agent -> Inspector
```

`agent prompt` atomically submits the text and an encoded Enter while honoring the
pane's live bracketed-paste mode, so a multi-line brief pastes as ONE block without
submitting early. `--wait` returns on the first settled `idle`/`done`/`blocked`.

**Invariant: a brief is a message payload, never a file.** Writing a brief to a path
and prompting an agent to go read it is a violation of this reference, not a style
choice. Herdr's own skill states the rule this package previously inverted: request
file output from an agent *only as a fallback*, never in the initial prompt.

## Leg 1 — Inspector to agent: message, no exception

Brief line caps are 50 (builder) / 40 (reviewer) / 30 (ad hoc)
(`agent-roster-and-dispatch.md` -> "Briefing rule", rule 4). A payload that small
always fits in a prompt. There is no size-based fallback on this leg and no
condition under which a brief is written to disk.

Dispatch through the one sanctioned path:

```bash
python3 scripts/dispatch.py --mode send --target <pane-or-name> \
  --unit <unit-id> --role <builder|reviewer|adhoc> <<'BRIEF'
...brief...
BRIEF
```

It reads the brief on stdin, validates it, appends the reply contract, and sends it.
Do not hand-roll `herdr agent prompt` in a shell command for a brief:
`agent prompt` has no `--file` flag in 0.8.0, so a hand-rolled call forces a 50-line
markdown payload full of backticks, quotes and `$` through shell quoting. That hazard
is what pushed this package to file-drop in the first place. `dispatch.py` passes the
brief as an argv element with no shell in between, so arbitrary text is safe.

## Leg 2 — agent to Inspector: message by default, bounded spill by exception

Every brief's `DELIVER:` / `REPORT:` block carries the reply contract up front, so
the agent decides before printing, not after:

```
DELIVER: reply in the pane. Open with <<<BEGIN <unit>>>> and close with
<<<END <unit>>>>. Keep it under 80 lines.
Over 80 lines: write the full text to /tmp/insp-exchange/<unit>.md and reply
with ONLY that path between the markers.
```

**Dispatch is asynchronous. Never block a turn on a reply.** `--mode send` returns as
soon as the pane starts working. The Inspector then arms `insp_wake.sh`, ends its turn,
and collects on the wake that reports that agent idle/done:

```bash
python3 scripts/dispatch.py --mode collect --target <pane> --unit <unit-id>
```

This is not an ergonomic preference. A blocking dispatch stalls the Inspector's whole
turn for as long as one agent takes — every other pane goes unsupervised, no context is
sampled, and the wake cadence the skill is built on stops. `--mode roundtrip` does block
and exists only for a reply you expect in seconds; routine dispatch is never roundtrip.
`collect` on a pane still `working` returns `still_working`, which is not a failure —
re-collect on the next wake.

The Inspector then:

1. `dispatch.py --mode send` fires; the turn ends with a wake armed.
2. On the wake, if the agent is `blocked`, read `--source visible` and resolve the
   dialog before assuming any reply exists. `blocked` is not a reply.
3. `dispatch.py --mode collect` reads back with `--source recent-unwrapped --lines 200`.
4. **Both markers present** -> that is the complete message. Done; no file exists.
5. **`<<<END>>>` present, `<<<BEGIN>>>` missing** -> the head scrolled off the
   alternate screen. Re-prompt: "write your previous reply verbatim to
   `/tmp/insp-exchange/<unit>.md`, reply with only that path." Then read the file.
6. **Neither marker** -> no reply landed. Diagnose with `herdr agent get`; do not
   re-send blind.

The sentinel pair is load-bearing. Without it a short complete reply and a long
half-lost reply look identical in the read, and silent truncation is what makes
people abandon message transport and go back to files.

`dispatch.py` classifies 4/5/6 itself and reports `complete` / `truncated` /
`no_reply`; act on that field rather than eyeballing the transcript.

## The spill file: named in advance, ephemeral, outside the worktree

Three properties, each closing one way a stray exchange directory appears:

- **The Inspector names the path, in the brief, before the agent runs.** An agent
  never invents a location. A path no brief declared is a violation.
- **It lives in `/tmp/insp-exchange/`, and that is deliberate.** A spill file is a
  transport buffer with a lifetime of one cycle: the agent writes it, the Inspector
  reads it, step 6 transcribes what matters into `findings.md`, and it is garbage.
  Wanting it to survive a restart is the error that produces a parallel ledger —
  which `SKILL.md` -> "Relationship to other skills" forbids outright. Durability
  belongs to the ledgers, not the channel.
- **Outside the worktree, so the wake fingerprint stays clean.** `insp_wake.sh`
  hashes `git status --porcelain` into the FINGERPRINT. A spill file inside the
  checkout flips that hash on every write, forcing the full tiered ledger re-read on
  every wake and defeating the optimization `herdr-polling-reference.md` -> "Change
  fingerprint" exists to provide. A `.gitignore` entry would technically hide it,
  but that leaves a load-bearing invariant resting on one line nobody watches.

Spill files are never cited in a ledger row, never referenced by a later brief, and
never read in a later cycle. Cite the `findings.md` id instead.

## Enforcement (this is mechanical, not just prose)

Prose rules get improvised around — an invented exchange directory is exactly what
that looks like. Two mechanisms make the rule hold:

- **One sanctioned dispatch path.** `scripts/dispatch.py` is the only supported way
  to brief an agent. It refuses a payload that looks like a file-path handoff
  (`Read /path/...`, `see /path/...`) and refuses a brief over its role's line cap,
  exiting 2 with the rule quoted.
- **A `PreToolUse` hook.** `hooks/guard_exchange_paths.py`, wired via
  `hooks/settings-snippet.json`, denies any `Write`/`Edit` whose path matches a
  brief/report/findings exchange pattern outside `/tmp/insp-exchange/`. It fires for
  the Inspector and for any supervised Claude Code pane that inherits the project
  settings, so neither side can re-create `~/gc-insp-exchange/` by improvisation.

A hook denial is a design signal, not an obstacle to route around: it means a brief
or a reply was about to become a file. Fix the transport, do not relocate the path.
