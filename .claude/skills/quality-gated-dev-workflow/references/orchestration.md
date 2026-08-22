# Multi-Agent Orchestration

The workflow's roles — adversary, validator, scout, builder, librarian — can run as
specialized subagents, each in its own isolated context. The main session is the
**orchestrator**: it holds the brain (WIKI + STATE), routes work, merges reports, and is
the **single writer of all governance files**.

## Why subagents here (and the honest cost)

Three genuine wins:
1. **Independence.** An adversarial cycle run by a fresh-context agent cannot be anchored
   by the builder's reasoning. Self-review by the same context is structurally weaker —
   this is the biggest quality gain, not a convenience.
2. **Context isolation.** Builders accumulate heavy implementation context; the librarian's
   migration walk is exploration-noisy; the scout's research is web-noisy. All of that
   stays out of the orchestrator, which keeps holding only Tier-0.
3. **Parallelism.** Read-only work (adversary + validator) can run concurrently; multiple
   builders can run concurrently on **disjoint** requirements and file sets.

The honest cost: every subagent is its own context — agent-heavy flows can consume several
times the tokens of single-threaded work, and each handoff adds latency. So delegation
follows rules, not enthusiasm (below).

## The team

| Agent | Job | Writes | Parallel-safe |
|---|---|---|---|
| **orchestrator** (main session) | routes, merges, decides with the user | ALL governance files (sole writer) | n/a |
| `qgdw-adversary` | A0–A5 cycles | nothing (report only) | yes (read-only) |
| `qgdw-validator` | CLV incremental/full | nothing (report only) | yes (read-only) |
| `qgdw-scout` | 3-options research for one DEC | nothing (report only) | yes (read-only) |
| `qgdw-builder` | one REQ via TDD | tests + source ONLY | yes, on disjoint REQs/files |
| `qgdw-librarian` | migration map / archive-on-close | drafts + archive moves ONLY | no (structural) |

## The single-writer rule (non-negotiable)

Only the orchestrator writes `WIKI.md`, `STATE.md`, `decisions.md`, `traceability.md`,
`findings.md`, `lessons.md`, and `wiki/` spokes. Subagents return structured reports; the
orchestrator merges them and performs every ledger mutation as a byproduct (Principle 9).
This is what makes concurrency safe: builders may write disjoint source files in parallel,
but governance state has exactly one writer, so it can never fork or race.

Corollary: **the orchestrator allocates all IDs** (from WIKI REGISTRIES) and passes them in
briefings. Agents never invent numbers.

## Delegation rules — when to spawn vs. do it yourself

DELEGATE when at least one holds:
- **Independence matters** → all adversarial cycles and CLV runs go to the adversary /
  validator. Never self-run a cycle on artifacts you produced in this same context if the
  agents are installed.
- **The work is context-heavy** → feature implementation (builder), migration exploration
  (librarian), option research with web search (scout).
- **Parallelism on disjoint work** → 2–3 builders on independent REQs; adversary +
  validator concurrently at a phase gate.

DO IT YOURSELF (no agent) when:
- The op is a Tier-0/Tier-1 read (recap, status, a quick index lookup).
- It is a single small edit, a ledger mutation, or a user conversation (decisions are
  ALWAYS presented to the user by the orchestrator — the scout only drafts).
- Agents are not installed in this project (fallback: run the role inline, using the same
  briefing/report discipline mentally).

CONCURRENCY LIMITS:
- Builders in parallel ONLY on disjoint REQ sets with disjoint file footprints (check the
  briefings' canonical locations for overlap before spawning).
- Never two librarians. Never a builder concurrent with archive-on-close.
- Merge order when parallel reports return: builders first (they change the changeset),
  then validator, then adversary findings.

## Briefing contract (orchestrator → agent)

Every delegation passes a briefing with exactly these fields (omit none):

```
TASK: <one line — which cycle / which REQ / which DEC / which job>
SCOPE: <the artifact paths and IDs in scope — nothing else is to be read>
ORIENTATION: <paste of the relevant STATE lines + the relevant WIKI MAP/REGISTRY lines>
CONSTRAINTS: <governing DEC ids + their chosen options; user constraints verbatim>
ALLOCATED-IDS: <pre-allocated numbers the agent may use, e.g. TEST-045..047; or NONE>
RETURN: <name the report format from the agent's own definition>
```

Why paste orientation instead of "read WIKI.md": agents start with no skill context; the
briefing is their whole world. Pasting the *relevant slice* keeps them oriented AND scoped —
they get the map lines they need without license to wander.

**Completeness rule — a briefing with an empty field is not ready to send.** Before
spawning, verify: `TASK` is a single, checkable objective (one REQ, one cycle, one DEC —
never "and also…"). **For a validator dispatch, `TASK` must name the gate being evaluated**
(task / checkpoint:<slice> / release / deploy) — Check 5 fails a finding only when its BLOCKS
effects intersect that gate, and an unnamed gate is reported `UNVERIFIABLE` rather than
assumed to be the widest one. Also verify: `SCOPE` lists explicit paths/IDs; the acceptance criterion (for builds)
is quoted **verbatim** from the REQ, not paraphrased; `CONSTRAINTS` name every governing DEC
with its chosen option; `ALLOCATED-IDS` are pre-reserved in REGISTRIES. If you cannot fill a
field, the task isn't ready to delegate — resolve the gap first (usually a missing decision
or an ambiguous REQ). Vague orders produce vague work; the briefing IS the order.

**Premise Verification.** Any factual claim about the codebase in a briefing — "all N
callers do X", "nothing does Y", "symbol Z already exists", "this is safe because W" — must
carry the command that established it and its result count (`grep -rn ... | wc -l` → 15),
so the agent can falsify it in seconds. A claim you did not verify must be marked
`UNVERIFIED — check before relying on this`. Design ledgers describe TARGET shape and run
ahead of code: quote design for INTENT, quote disk for FACT (LSN-019/LSN-034 — one false
premise cost a whole slice and would have shipped a cross-service regression).

**Git-truth.** Read-only agents cannot see live git, and any session-start snapshot is
frozen. For any dispatch whose verdict depends on working-tree or commit state, paste a
FRESH `git status --porcelain` + relevant `git log --oneline -- <paths>` into the briefing;
brief the validator to scope out rows that are pending-merge so a merge-lag gap is never
recorded as a substantive FAIL (LSN-016/LSN-023). The orchestrator owns git-truth; agents
own content.

**Precedence + one bounded question.** Every briefing carries an explicit precedence line
for its own instructions (e.g. "fold-ins override the do-not-touch list; on conflict, do
the fold-in and flag it"), and may carry at most ONE bounded, implement-nothing,
report-only question about an adjacent concern — the agent is already in the code, and such
asides have produced session-best findings.

## Report merge (agent → orchestrator) — the byproduct step

On every returned report, the orchestrator immediately:
1. **Builder report** → update trace rows + `STATE.OPEN/CHANGESET` from FILES; check
   API-SURFACE/DEPENDENCIES against governing DECs (mismatch = drift finding); queue the
   feature for adversary A3.
2. **Adversary report** → assign F-numbers, append open findings to `findings.md` (one line
   each), full narrative to `cycles/` (cold). The adversary reports **technical severity**;
   the **orchestrator assigns the BLOCKS effects** at merge, defaulting to
   `TASK:<active-slice>` when wider scope is unproven. Findings with a non-empty effect set
   go to `STATE.BLOCKING` with their effects; advisory (empty-set) findings do not.
   LESSON-CANDIDATES are evaluated against the lesson capture threshold
   (`lessons-memory.md`) — they are candidates, not automatic captures.
3. **Validator report** → verdict line into `STATE.LAST_CLV`; clear `CHANGESET` on green.
   On FAIL, follow **failed-gate governance** (verdict · evidence pointer · effects · next
   repair) and nothing more; effects come from the failed property, not from why the run was
   requested (`cross-layer-validation.md`). Entries the validator marks
   `NEEDS-CLASSIFICATION` are classified by the orchestrator, never treated as global blocks.
4. **Scout report** → orchestrator formats the user-facing DECISION REQUIRED proposal,
   presents it, and on choice: full entry to `decisions.md` (cold), one index line,
   registry bump, STATE patch if dependents/one-way.
5. **Librarian report** → review drafts/anomalies, then the orchestrator itself applies the
   WIKI-EDITS-REQUIRED lines and commits drafts to live paths.

A report that doesn't parse against its contract is re-requested once with the contract
quoted; twice-failed = run the role inline and capture a lesson against `op:delegate`.

## Verification Gate — double-check every report before merge

Reports are claims, not facts. Before ANY merge (ledger update, commit, code acceptance),
run three checks in order:

1. **Contract check.** The report parses against the agent's declared format; every
   mandatory field is present and non-empty (builder `FILES`+`NOTES` especially); all IDs
   come from the allocated set (an invented ID = automatic re-brief).
2. **Evidence check.** Verify central claims against reality:
   - **builder** → re-run the feature's own tests + any suites the diff touches
     (targeted; the FULL suite re-runs once per wave/gate per Scale discipline, not per
     feature); diff claimed `FILES` vs actual
     (`git status` / `git diff --stat`) — undeclared changes are scope violations; check
     `API-SURFACE` and `DEPENDENCIES` against governing DECs (an undeclared dependency is
     drift, route to findings).
   - **validator / adversary** → spot-check 1–2 cited findings at their cited file/ID
     locations before accepting the verdict; a finding that doesn't reproduce downgrades
     the report to re-brief.
   - **scout** → three *genuinely viable* options (no strawman), every score has a reason,
     cascade impact names concrete downstream steps, volatile claims carry dated sources.
   - **librarian** → audit walked/mapped/skipped counts; spot-check 2–3 MAP lines against
     the tree; anomalies section present.
3. **Goal audit.** Re-read the briefing `TASK` and (for builds) the REQ acceptance
   criterion **verbatim**. The delivered work must satisfy that goal — the test must encode
   the criterion as written, not a weaker paraphrase; the diff must contain nothing outside
   the briefed scope. Out-of-scope changes: revert or explicitly re-brief. Never silently
   keep them.

**The stop-and-report hatch is a success, not a failure.** A builder that discovers its
briefing's premise is false, its target has no production caller, or its criterion cannot
be honestly tested must STOP, revert anything speculative, and report
(`STATUS: blocked-on` + what was built, what was reverted, and the conflicting evidence).
Firing this hatch correctly is a strictly better outcome than delivering against a bad
instruction, and is recorded as such — never penalized in re-briefing. Reports also carry a
`CONTRACT-CONFLICTS` field for instructions inside one briefing that contradict each other.

Pass → merge as byproduct. Fail → **re-brief once**, quoting the contract and the specific
defect ("FILES lists 3 files; git shows 5 — account for tests/helpers.py and src/util.py").
Fail twice → run the role inline this once and capture a lesson (`op:delegate | <agent> |
<defect class>`); recurring defect classes justify tightening that agent's prompt.

## Non-delegation is a failure mode — watch for it

The most common orchestration failure is silent: the orchestrator gives a correct status,
then does the next step's role-work itself. Counter-rules:
- `STATE.TEAM` is checked at every session start; when `on` **and `STATE.RIGOR` is
  standard or full**, the Delegation Table is consulted **before** any role-work begins —
  "I'll just quickly do this one" is the violation, not an exception. At RIGOR:light,
  inline role-work is permitted (spawning five agents for a small tool is overhead, not
  rigor) — but any agent that IS used still passes the Verification Gate.
- The orchestrator's own hands are limited to: briefings, verification, ledger merges,
  user conversations, single small edits. Anything else that matches a Delegation Table
  row gets dispatched.
- Catching yourself mid-role-work: stop, capture `op:delegate` lesson, dispatch properly.

## Blocking effects & the behavioral matrix

**Severity and blocking effect are different things.** An agent reports *severity*
(`blocking | non-blocking | informational`) — a technical judgement about the defect. The
**orchestrator** assigns *BLOCKS* — a **set** of zero or more effects saying what may not
proceed:

```
TASK:<task-or-id>      CHECKPOINT:<slice>      RELEASE      DEPLOY
```

An **empty set is advisory** and blocks nothing. Effects compose; they are not a single class,
and a finding may carry one, several, or none. **Scope is proven, not assumed:** any effect
beyond `TASK:` needs a stated reason it reaches that far, and effects are added or removed as
demonstrated scope changes. A finding never means "nothing else runs" unless it can affect
every independent operation. When scope is unproven, assign `TASK:<active-slice>` — never a
global block.

### Parallel-work rule

- Unrelated **read-only** work is **always permitted** under any effect set.
- Unrelated **write** work is permitted only when paths, ownership, generated artifacts, and
  evidence inputs are **all disjoint** — prove the disjointness, don't assume it.
- **Compaction is WRITE work**, not read-only: it waits if it edits any document involved in
  the active failure.
- Scouting is *normally* read-only, and is exempt only while it stays so. The moment it
  writes, the write rule applies.
- Owner-separated builds and checkpoints may proceed only when their build outputs **and**
  source footprints are disjoint.

### The behavioral matrix (twelve scenarios)

Each scenario derives seven fields: severity · BLOCKS · scope · parallel read · parallel
write · required evidence · termination condition.

**S1 · production ASan defect on a shipped path**
severity blocking · BLOCKS `{TASK:REQ-nnn, CHECKPOINT:REQ-nnn, RELEASE}` · scope: the owning
slice and any release containing it · read: everything · write: any slice with disjoint paths,
outputs and evidence inputs · evidence: sanitizer-instrumented rerun of the complete affected
target, clean, on the final content version · terminates when the defect is repaired and that
rerun is green.

**S2 · harness-only defect invalidating one slice's evidence (production untouched)**
severity blocking (for that evidence) · BLOCKS `{TASK:<repair>, CHECKPOINT:<slice>}` — add
`RELEASE` only when release acceptance depends on that evidence · scope: the slice whose
evidence is invalid · read: everything · write: every other slice, and disjoint documents ·
evidence: the bounded repair plus a rerun of the complete affected target on the final content
version · terminates when that rerun is green; the checkpoint is then permitted, and **no
A-cycle, CLV, lesson, or registry entry is allocated** unless a bounded-repair exception fires
(see `adversarial-cycles.md`).

**S3 · stale internal / non-acceptance documentation**
severity non-blocking · BLOCKS `{}` · scope: none until impact is demonstrated · read and
write: unrestricted — **implementation, checkpoints and release work may all continue** ·
evidence: none required to proceed · terminates when the document is corrected, or an effect
is added if impact is later demonstrated.

**S4 · unrelated dirty files in the worktree**
severity informational · BLOCKS `{}` · scope: none · read and write: unrestricted · evidence:
**path-scoped staging remains mandatory** (`git add -- <slice paths>`; never `git add -A`) ·
terminates immediately — the slice commits carrying only its own paths.

**S5 · requested verified checkpoint whose slice gate is green**
severity n/a · BLOCKS `{}` · scope: none · read and write: unrestricted · evidence: **only the
slice Verification Gate** — its own tests and touched suites green, contract and goal audit
passed, path-scoped staging; unrelated release gates (wave full suite, clean-worktree build,
full CLV, doc currency) are **not preconditions** · terminates when the checkpoint is
committed.

**S6 · hook false positive (a deterministic guard denies a legitimate operation)**
severity blocking for that operation · BLOCKS `{TASK:<denied op>}` · scope: the denied
operation only · read: everything · write: everything else · evidence: isolated reproduction,
a finding plus a `miss` on the guard's lesson, and a two-directional behavior matrix (genuine
violations still denied AND the legitimate case passing) · terminates when the mechanism is
fixed and the matrix is green — **never by a bypass**; two unsuccessful material attempts
return to the user.

**S7 · read-only scouting while another task is blocked**
severity informational · BLOCKS `{}` · scope: none — **no effect set ever suspends read-only
work** · read: unrestricted · write: none while it stays read-only · evidence: none ·
terminates immediately.

**S8 · production defect blocks REQ-A; REQ-B has disjoint files and build artifacts**
severity blocking (REQ-A) · BLOCKS `{TASK:REQ-A, CHECKPOINT:REQ-A, RELEASE}` · scope: REQ-A
and release · read: everything · write: **REQ-B may continue** building and taking verified
checkpoints, because its paths, ownership, generated artifacts and evidence inputs are
disjoint — demonstrate that before starting · evidence: REQ-B's own slice gate; REQ-A's repair
evidence separately · terminates with REQ-B checkpointing independently while release waits on
REQ-A.

**S9 · stale acceptance / compliance / migration / release documentation**
severity blocking at release · BLOCKS `{RELEASE}` · scope: release only · read and write: all
implementation continues and **verified checkpoints remain allowed** · evidence: the corrected
document plus the release gate's own checks · terminates when the document is corrected and
`RELEASE` is cleared.

**S10 · a formatter or hook edits a tested/compiled input after a green run**
severity informational (evidence-invalid, not a defect) · BLOCKS
`{TASK:<rerun-target>, CHECKPOINT:<affected-slice>}` — add `RELEASE` only if the invalidated
evidence was the release evidence · scope: the target whose content version changed · read:
everything · write: disjoint slices · evidence: rebuild if needed and **rerun the complete
affected target once for the new content version** · terminates when that rerun is green.
This ordinary invalidation **does not automatically allocate a finding, lesson, VAL, ORCH, or
registry entry**; it becomes a finding only if the rerun exposes a defect, or if the
invalidation mechanism itself repeatedly fails.

**S11 · 73 declared functions produce 78 executed QtTest cases**
severity informational · BLOCKS `{}` · scope: none · read and write: unrestricted · evidence:
both numbers reported separately, with the difference reconciled to `initTestCase`,
`cleanupTestCase` and data rows · terminates as **PASS; create no finding**. A finding exists
only when the runner's accounting cannot reconcile the difference.

**S12 · user-requested unverified local snapshot**
severity n/a · BLOCKS: **existing effects unchanged** — a snapshot preserves work; it does not
clear or weaken any blocker · scope: unchanged · read and write: unchanged · evidence: explicit
user authorization, local-only, `WIP`/`SNAPSHOT` at the start of the message, and every red or
ungated item named · terminates with the snapshot committed locally; it **cannot be pushed,
promoted, or treated as a verified checkpoint or release candidate**.

**Re-verify this matrix after every skill update**, together with the `guard_selftest.py`
post-update routine (`lessons-memory.md`) — a skill update replaces these documents wholesale,
so a corrected semantic can return to its old form with zero new code.

## Snapshot vs verified checkpoint vs release

Three distinct preservation/release operations. Do **not** call an unverified snapshot a
checkpoint.

| Operation | Requires | Does NOT require |
|---|---|---|
| **Snapshot commit** | explicit user authorization; local-only; message begins `WIP`/`SNAPSHOT`; names every red or ungated item | any gate — it is explicitly unverified |
| **Verified checkpoint** | the slice's own tests + touched suites green; contract and goal audit passed; path-scoped staging; message identifying REQ/TEST/DEC | wave full suite, clean-worktree build, full CLV, doc currency, zero unrelated findings |
| **Release commit** | all applicable acceptance and release gates; no open finding carrying `RELEASE` (or `DEPLOY` for a deploy) | — |

A snapshot **cannot be promoted or pushed as a release candidate**, and it leaves every
existing BLOCKS effect active. A known harness or documentation defect may carry `RELEASE`
while leaving verified checkpoints, read-only scouting, and disjoint write work permitted.

## Scale discipline — no duplicated processes as the project grows

Three redundancies emerge at scale unless explicitly forbidden. These rules assign each
piece of evidence exactly ONE owner and one run.

**Test-execution ownership (kills the triple-run).** The governing rule:

> The complete affected suite runs **once per final content version**, by **one designated
> owner**. A failed run followed by a repair, or any formatter/hook edit to a compiled or
> executed input, creates a **new content version** and requires a new run. Other roles
> inspect the command, log, exit code and artifact hashes and run only targeted independent
> checks; they do **not** duplicate the complete suite on the same content version.

Applied to the roles:
- **builder** runs its new tests (RED/GREEN) + the complete affected suite once at REFACTOR
  end — the authoritative "suite green" evidence, quoted in its report with the accounting
  below.
- **orchestrator verification** re-runs **only the feature's own tests + any suites the
  diff touches** (targeted), and inspects the builder's command/log/exit code rather than
  re-executing the same content version — unless the targeted run or the diff contradicts
  it, or the orchestrator itself edited an input (a new content version, which it then owns).
- **adversary A3** runs **mutation/property tooling only** — never the plain suite again.
- The complete suite re-runs once per wave/gate, not per feature. Total executions stay
  ~O(features), not O(features²).

**Evidence invalidation.** The last edit to any compiled or executed input starts a new
content version: rerun the **complete affected target** after that final edit — not a subset
of it — and **do not rerun unrelated suites**.

**Evidence accounting (portable schema).** Report these as *separate* measurements, never
collapsed into one number:

```
command:      <exact invocation>
environment:  <build config / sanitizer flags / applicable settings>
declared:     <test functions declared>            | N/A — <reason>
executed:     <test cases / data rows actually run>| N/A — <reason>
result:       pass <n> · fail <n> · skip <n>
sanitizers:   <ASan/UBSan/TSan: clean | n reports> | N/A — not instrumented
exit code:    <n>
```

Each measurement is either an **observed value** or **N/A with a concrete reason** when the
framework or toolchain cannot expose it. Always required: the command; the applicable
environment/configuration; the pass/fail/skip result; the process exit code; and the sanitizer
result when sanitizer instrumentation is applicable. The **declared-function count is optional**
when the framework has no discovery/list operation. Artifact hashes are required only when the
gate uses them for content identity. **Do not fail a gate merely because an inapplicable
measurement is N/A.**

`declared ≠ executed` is **expected** wherever init/cleanup functions and data rows exist (73
declared QtTest functions producing 78 executed cases is normal) and is **not itself a
finding**. It becomes a finding only when the runner's accounting cannot reconcile the
difference.

**Failed-gate governance.** On a FAIL, write **only**:
1. the current verdict;
2. the evidence pointer, with the command and the exit code;
3. the BLOCKS effects and their explicit scope;
4. the next repair.

Broad narratives, counts, registry rewrites, new lessons, compaction, and archive moves
**wait until the repair passes**. *Exception:* a security or data-loss discovery may
additionally record the minimum warning required to prevent unsafe use — and must not trigger
an automatic governance wave.

**Wave-gate checklist (run all four, once per wave/feature close):**
1. full suite (the once-per-wave run);
2. **clean-worktree configure+build of HEAD** — `git worktree add` a throwaway, configure
   and build there; committed build files referencing untracked paths, and unit-green
   binaries that don't link, are only visible here (LSN-018/ORCH-001 class);
3. **MAP-freshness count** — in deny-only guard mode nothing nudges registration, so count
   tree entries not covered by a MAP line (directly or via parent rollup); past a handful,
   dispatch librarian sync;
4. wave-level incremental CLV.
Prefer taking a **verified checkpoint** of each Verification-Gate-passed slice before starting
the next (see *Snapshot vs verified checkpoint vs release*):
**entanglement compounds with every uncommitted slice** (extracting one REQ from a tree
holding five required hand-built index blobs). If slices must accumulate, record the
intended commit split at dispatch time so builders keep footprints separable.

**CLV batching (kills redundant validation layers).** One changeset is validated once:
- Features built **in parallel (a wave)** → ONE wave-level incremental CLV over the merged
  changeset. No per-feature slice CLV inside a wave.
- A feature built **solo** → its slice CLV *is* the validation; the phase gate does not
  re-validate unchanged IDs (the validator sees `CHANGESET` cleared and checks only gate
  criteria).
- FULL walks run **only on explicit user request or at the release gate** — never as the
  automatic consequence of a phase exit or a rigor tier. Ordinary feature, wave, checkpoint
  and phase gates get the incremental pass; RIGOR:FULL adds an incremental CLV at every phase
  exit, not a FULL one.

**Briefing budget (kills orientation bloat).** The `ORIENTATION` field is capped at
**~12 lines**: the STATE lines that matter to this task + only the WIKI MAP/REGISTRY lines
covering the task's scope. Pasting the whole MAP or whole index into a briefing is a
violation — slices only. And because the briefing IS the agent's orientation, agents must
not re-read WIKI/STATE themselves (their prompts say so): one orientation, not two.

**Budget telemetry (the skill tracks itself).** `STATE.BUDGETS` records current size vs
cap for the hot artifacts: `WIKI <tok>/700 · DECIDX <tok>/500 · LSN <guards>/10 ·
FINDINGS <open>`. Tokens are measured by proxy: `wc -c` bytes / 4 (or line count vs a line
cap) — a number you can actually compute, not an estimate. The check is a **byproduct
assertion**: immediately after editing a hot file, run the one-line measurement and compare
to cap — a declared threshold nothing measures accumulates breach silently for months
(field-proven: an 18k-char REGISTRIES block, 71 lint findings).
**Any breach = dispatch librarian Job 3 (compaction)** before the next feature wave —
compaction is scheduled work, not an emergency. This is how growth is *observed* instead of
discovered when sessions get slow.

## Standard plays

**Phase gate (e.g., Phase 1 exit):**
spawn adversary (A1 on prd.md) ∥ validator (incremental CLV) → merge both → disposition
findings with the user → re-spawn only the failed role until clean → exit.

**Feature wave (Phase 2):**
pick 2–3 REQs with disjoint footprints → pre-allocate TEST ids → spawn builders in parallel
→ merge reports (trace/changeset) → spawn adversary A3 per feature (parallel, read-only) →
disposition → validator incremental over the merged changeset → green = wave done.

**Decision:**
spawn scout with the DEC question + constraints → merge draft → orchestrator presents the
three options to the user → log on choice. (The user-facing ask NEVER comes from the scout.)

**Migration:**
spawn librarian (Job 1) → audit its walked/mapped/skipped counts and ANOMALIES → orchestrator
commits WIKI/spokes → normal operation begins. Anomalies (pre-existing duplicates!) become
the first findings.

**Release:**
validator FULL walk ∥ adversary A4 → both must come back clean/dispositioned → DEC-011
rollout (scout optional) → deploy → A5 retrospective (adversary) feeds lessons.

## Installation & fallback

Agent definitions ship in the skill's `agents/` directory and install to the project with:
```
python3 scripts/install_hook.py <project>   # installs hook AND copies agents/
```
(They land in `<project>/.claude/agents/`; **restart the Claude Code session** to load
agents added on disk.) Without them, the workflow still runs single-context: the
orchestrator plays each role inline using the same briefing/report discipline — strictly
worse on independence, identical on rules. The anti-duplication PreToolUse hook applies to
subagent tool calls in the session too, so builders are guarded mechanically as well.
