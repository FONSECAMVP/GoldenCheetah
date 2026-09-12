# Project state and next-step selection

## Read order, every cycle

Follow QGDW's own tiered-loading model (`.claude/skills/quality-gated-dev-workflow/
references/state-and-tiers.md`) — don't re-derive it:

1. Root `WIKI.md` first (the map — what exists and where), then root `STATE.md` (the
   cursor — current stage, live PIDs/topology narrative, "Next" section).
2. `.claude/workflow-garminconnect/traceability.md`, `decisions.md`, `findings.md` — the
   real ledger; status lives ONLY here (or in `decisions.md` for DEC status), never in
   `STATE.md`/`design.md` prose.
3. `references/orchestration.md` for gate/stage mechanics beyond the tier model.

## Goal / Stage / Atomic-unit selection

- **Goal** = the feature actually reaches a real installer/user, not just green tests. A REQ
  closed only against `ctest`/CMake evidence is NOT evidence it ships — GoldenCheetah has
  two non-equivalent build systems (qmake = real CI/release via `appveyor.yml`; CMake =
  dev/test-only). Before treating any REQ as "done" or a stage as "closed," check which
  build system the evidence actually cites.
- **Stage** = the current `PHASE`/`OPEN`/`NEXT_GATE` narrative in `STATE.md` naming a Stage
  N (the full historical stage table lives in `archive/state-history.md` — `STATE.md`
  itself only carries the live cursor). When every REQ in a stage's scope is closed in
  `traceability.md` and every relevant DEC is recorded (accepted/superseded/deferred) in
  `decisions.md`, the stage is done — identify the next stage immediately, don't wait to be
  told.
- **Atomic unit** = one REQ, DEC, or fix. Dispatch exactly one per builder turn. On closing
  one, immediately identify the next one inside the current stage.

## Before marking anything closed (id and status hygiene)

- **Id collisions, REQ/DEC/DES/VAL/TEST namespaces:** check root `WIKI.md`'s `REGISTRIES`
  section first — its `next:` line per namespace is the authoritative allocation pointer.
  For `TEST`/`T-NNN` ids specifically, treat both prefixes as one counter (the project
  silently switched prefixes once). Cross-verify by grepping the ledger for that same
  prefix before allocating, to catch a `REGISTRIES` line that's gone stale — never infer a
  number from a neighboring row or from memory of a past count.
- **Id collisions, per-REQ finding namespace (`B-R<REQ>-NN`):** there is no `REGISTRIES`
  entry per finding-cycle — grep `findings.md` directly for that exact `B-R<REQ>-` prefix
  and take its true max. Do this BEFORE briefing a reviewer/scout to assign ids, not just
  before writing the ledger row yourself.
- **Status placement:** `ledger-drift-lint` rejects any line in `STATE.md`/`design.md` that
  pairs an id with a status token (`GREEN`/`CLOSED`/`DEFERRED`/etc). Say "committed
  `<hash>`" there instead — status itself lives only in the canonical ledger files.
- **Independent second opinion before a DEC bends a standing rule** (a lesson, an
  architectural exception, a "can't be done" conclusion): dispatch a separate, fresh,
  unbriefed agent with only the narrow verification question before recording the DEC.

## Commit discipline

- Run `git diff --stat` AND `git diff --cached --stat` (not just `git status`) — the first
  alone misses anything already staged by an earlier step or another process. Read the
  actual diff for anything whose relevance isn't obvious from the path alone — this working
  tree routinely carries several genuinely unrelated uncommitted workstreams at once. Stage
  explicit file paths, never `git add -A`/`.`/a bare directory.
- If one shared file mixes this REQ's change with other unrelated already-uncommitted work,
  extract just the relevant hunk programmatically (`git diff` → isolate the `@@` block →
  `git apply --cached --check` dry run → apply for real) rather than skipping the file or
  hand-splitting by eye.
- Expect the real gate to be pre-commit's hook (clang-format, ruff, `mypy --strict`,
  `ledger_drift_lint.py`), not `ctest`/`pytest` green alone — budget a fix-and-retry cycle.
  If clang-format reformats files in a failed attempt, rebuild + rerun the affected test
  target directly to confirm the reformat was cosmetic before re-staging and retrying
  (never `--amend`).

## If a QGDW skill package bug surfaces mid-cycle

Log a finding + a lesson "miss," fixed upstream by the user — never patch the installed
`.claude/skills/quality-gated-dev-workflow/` copy locally, except in an explicit
skill-authoring session directed as such.
