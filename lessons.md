# Lessons Memory — GoldenCheetah            (index first, cold entries below)

LSN-001 | op:create-file type:duplication            | guard    | recur:n/a saves:1 miss:0 | check WIKI MAP before mkdir/touch/write — enforced by mechanism: .claude/hooks/anti_duplication_guard.py
LSN-002 | op:id-alloc type:cross-ledger-collision     | guard    | recur:2  saves:0 miss:0  | never merge ID ranges across ledgers; qualify with ledger prefix (garmin:/coach:)
LSN-003 | op:prd type:underspecified-nfr              | guard    | recur:11 saves:0 miss:0  | every NF-* requirement needs a quantified acceptance value before it enters an A1 cycle
LSN-004 | op:design type:missing-seam                 | guard    | recur:1  saves:0 miss:0  | any call into a mutable third-party library needs its adapter/interface seam defined before first implementation, not retrofitted… — see ## LSN-004 below
LSN-005 | op:design type:security-invariant-on-read   | guard    | recur:1  saves:0 miss:0  | security invariants enforced on write (perms, format) must also be validated on read, not assumed
LSN-006 | op:code type:error-handling                 | advisory | recur:1  saves:0 miss:0  | exception handlers at adapter/boundary layers must classify by type before a broad except, never swallow-and-misroute — see ## LSN-006 below
LSN-007 | op:commit type:hook-mutation-unverified      | guard    | recur:1  saves:0 miss:0  | if a pre-commit hook modifies files, all prior build/test evidence is void — rebuild + re-run affected tests before accepting the commit; — see ## LSN-007 below
LSN-008 | op:ledger-update type:index-vs-detail-drift  | MECHANISM | recur:10 saves:8 miss:9  | SAVE 2026-08-18 (REQ-028 Phase-2 gate) — AND IT CAUGHT THE ORCHESTRATOR, WHICH IS THE POINT. — see ## LSN-008 below
LSN-011 | op:ledger-update type:design-note-false-done | guard    | recur:1  saves:1 miss:0  | any DEC-cascade item written into a design. — see ## LSN-011 below
LSN-009 | op:test type:loose-timeout-bound-survivor   | advisory | recur:1  saves:1 miss:0  | a bounded-teardown/timeout assertion must be tight enough to FAIL if the graceful fast-path is skipped (assert « the fast-path… — see ## LSN-009 below
LSN-010 | op:commit type:commit-column-staleness      | advisory | recur:1  saves:1 miss:0  | a feature commit that bundles its OWN ledger byproduct necessarily records "uncommitted"/pending (the hash doesn't exist yet); — see ## LSN-010 below
LSN-012 | op:verify type:builder-lint-dirty-green      | advisory | recur:1  saves:1 miss:0  | a builder GREEN report is not verified until the DEC-009 style gate (ruff/clang-format/mypy) has run on the changeset — the… — see ## LSN-012 below
LSN-013 | op:test type:mask-coverage-gap              | advisory | recur:1  saves:1 miss:0  | a test guarding a security predicate that ORs multiple bit/flag classes (file perms Read/Write/Exec × Group/Other; — see ## LSN-013 below
LSN-014 | op:ledger-design type:status-duplication   | guard scope:portable | recur:n/a saves:0 miss:0 | ONE logical fact = ONE storage location. — see ## LSN-014 below
LSN-015 | op:cascade type:named-target-unverified    | guard scope:portable | recur:1  saves:0 miss:1  | when a DEC/cascade NAMES specific files to edit or repoint, grep-verify EACH named target was actually changed before declaring… — see ## LSN-015 below
LSN-016 | op:delegate type:clv-before-merge          | advisory | recur:1  saves:0 miss:0  | when incremental CLV is dispatched BEFORE the builder's byproduct is merged, its missing-row / stale-findings FAILs are… — see ## LSN-016 below
LSN-017 | op:test type:queued-self-post-captures-this | advisory scope:portable | recur:1  saves:0 miss:0  | a queued/deferred self-post (QMetaObject::invokeMethod/singleShot(0,…)) whose lambda captures an object OTHER than the QObject… — see ## LSN-017 below
LSN-018 | op:build type:main-binary-link-gap        | advisory scope:portable | recur:2  saves:0 miss:0  | GREEN unit tests do NOT prove the application links: a unit-test target compiles its OWN curated source subset, so a new production . — see ## LSN-018 below
LSN-019 | op:delegate type:briefing-symbol-claim-unverified | advisory scope:portable | recur:1  saves:0 miss:1  | a builder briefing that asserts an existing code symbol ("enum X already has value Y", "field Z exists") MUST be grep-verified… — see ## LSN-019 below
LSN-020 | op:test type:real-bridge-uncovered-behind-fake | guard scope:portable | recur:1  saves:0 miss:1  | when a slice adds an OP to an embedded-interpreter/FFI adapter seam (IGarminPyAdapter-style: a C++ interface with a Fake impl AND… — see ## LSN-020 below
LSN-021 | op:design type:wizard-page-state-not-reset-on-reentry | guard scope:portable | recur:1  saves:0 miss:1  | a QWizardPage (or any reusable multi-step UI page) that holds an async state machine (Idle→InFlight→Success/Error/terminal) MUST… — see ## LSN-021 below
LSN-023 | op:delegate type:readonly-agent-git-blind | guard scope:portable | recur:1  saves:0 miss:1  | a read-only agent (validator/adversary/scout — Read/Glob/Grep only, NO git tools) whose verdict depends on working-tree/commit… — see ## LSN-023 below
LSN-022 | op:test type:tautological-assertion | advisory scope:portable | recur:1  saves:0 miss:1  | a test assertion that is unconditionally true — QVERIFY2(X || true, …), assert x or True, expect(true), QCOMPARE(a||1, …) —… — see ## LSN-022 below
LSN-024 | op:verify type:consumer-of-deferred-contract | guard scope:portable | recur:1 saves:0 miss:1 | a slice that CONSUMES a cross-slice/cross-module contract (a field in a persisted file, an env value, state another slice must… — see ## LSN-024 below
LSN-025 | op:design type:idempotency-record-before-confirm | guard scope:portable | recur:1 saves:0 miss:1 | a persistent dedup/idempotency record (a Tier-1 sidecar, "already-processed" cache, imported-id set) must be written STRICTLY… — see ## LSN-025 below
LSN-026 | op:code type:rmw-not-salvaging-on-torn | advisory scope:portable | recur:1 saves:0 miss:1 | a read-modify-write persistence helper must SALVAGE surviving entries on a torn/permission-rejected precondition — reusing the… — see ## LSN-026 below
LSN-037 | op:design type:guard-covers-one-of-several-freed-objects | guard scope:portable | recur:2 saves:1 miss:1 | a teardown almost never frees ONE thing, and the frames suspended below it include the CONSTRUCTOR. — see ## LSN-037 below
LSN-036 | op:mechanism type:guard-false-positive-trains-bypass | guard scope:portable | recur:3 saves:0 miss:3 | when a deterministic guard denies a LEGITIMATE operation, the failure is the guard's, not the operator's — and the real damage is… — see ## LSN-036 below
LSN-035 | op:byproduct type:index-table-falls-behind-its-own-prose | guard scope:portable | recur:2 saves:1 miss:2 | when a ledger has BOTH per-row prose and a summary INDEX table, the byproduct step updates the prose and silently skips the index… — see ## LSN-035 below
LSN-034 | op:delegate type:unverified-premise-in-briefing | guard scope:portable | **recur:12 saves:10 miss:8 — MECHANISM PROMOTION IS OVERDUE, AND THE 2026-08-23 PAIR SHOWS WHY THE CURRENT GUARD CANNOT GET THERE ON ITS OWN** | REFINEMENT 8 (2026-08-23, DEC-038 A3 — TWO instances in ONE day, one per channel, and the contrast is the lesson). — see ## LSN-034 below
LSN-033 | op:build type:fail-closed-return-ignored-by-caller | guard scope:portable | recur:1 saves:0 miss:1 | a refusal is only a refusal if the CALLER can observe it. — see ## LSN-033 below
LSN-032 | op:verify type:checkout-revert-destroys-uncommitted-work | guard scope:portable | recur:1 saves:0 miss:1 | NEVER revert a temporary verification mutation with git checkout -- <file> unless that file is CLEAN at HEAD — on a file carrying… — see ## LSN-032 below
LSN-031 | op:test type:untouched-means-bytes-only | guard scope:portable | recur:1 saves:0 miss:1 | an assertion that a file is "untouched"/"unchanged"/"preserved" via a BYTE compare alone tolerates truncate+rewrite-identical, a… — see ## LSN-031 below
LSN-030 | op:code type:secret-deletion-misses-tmp-sibling | guard scope:portable | recur:1 saves:0 miss:1 | deleting a secret written through an atomic-write helper (tmp+fsync+rename) must ALSO remove the <path>. — see ## LSN-030 below
LSN-029 | op:test type:ordering-invariant-unobserved | guard scope:portable | recur:1 saves:0 miss:1 | when a DEC records an ORDERING invariant ("write A before B", "delete X before Y", "record only after confirm"), the test set… — see ## LSN-029 below
LSN-028 | op:verify type:unaudited-criterion | guard scope:portable | recur:1 saves:0 miss:1 | a requirement whose mechanism was built as a SIDE-EFFECT of another REQ's slice is NOT done until its OWN criterion is split into… — see ## LSN-028 below
LSN-027 | op:design type:qobject-method-name-hidden | guard scope:portable | recur:1 saves:0 miss:1 | before naming a NEW method (esp. — see ## LSN-027 below
LSN-041 | op:design type:fix-scoped-to-one-instance-of-a-class-wide-bug | guard scope:portable | recur:1 saves:1 miss:1 | when a defect is a CLASS (a repeatable code pattern — here: an unguarded nested QEventLoop in a dialog constructed/parented under… — see ## LSN-041 below
LSN-042 | op:test type:dormant-hazard-behind-unset-flag-is-still-a-defect | advisory scope:portable | recur:1 saves:0 miss:0 | a memory-safety (or security) hazard that is unreachable today ONLY because a UI flag / config setting / feature toggle is… — see ## LSN-042 below
LSN-040 | op:design type:qt-parent-deletes-stack-child | guard scope:portable | recur:1 saves:0 miss:1 | Qt's QObjectPrivate::deleteChildren() calls delete on EVERY child of a destroyed parent UNCONDITIONALLY — independent of the… — see ## LSN-040 below
LSN-039 | op:verify type:mutation-survives-because-branch-unreachable | guard scope:portable | recur:1 saves:0 miss:1 | a guard/line whose mutation SURVIVES the suite is not proven dead or wrong — it may be UNREACHABLE by the fixture. — see ## LSN-039 below
LSN-043 | op:verify type:configure-is-not-build | guard scope:portable | recur:1 saves:0 miss:1 | a build-definition fix is NOT verified until configure AND link AND the test suite have run from a CLEAN extract — configure-only… — see ## LSN-043 below
LSN-046 | op:build op:cycle type:self-lifetime-guard-does-not-cover-collaborators | guard scope:portable | recur:1 saves:0 miss:1 | when a fix closes a use-after-free by guarding an object's OWN lifetime (QPointer self-bail after each nested loop), that guard… — see ## LSN-046 below
LSN-047 | op:verify type:stub-cannot-prove-the-fault-it-is-credited-with | guard scope:portable | recur:1 saves:1 miss:1 | before writing "RED: dereferences <X> → ASan heap-use-after-free" in a test comment or a ledger row, OPEN the stub/mock that <X>… — see ## LSN-047 below
LSN-048 | op:design op:decision type:req-stub-understates-the-real-surface | guard scope:portable | recur:2 saves:1 miss:2 | REFINEMENT (2026-08-13, O-R027-01, recur:2 — the guard was HONOURED and the answer was still wrong): re-deriving the surface is… — see ## LSN-048 below
LSN-049 | op:build op:verify type:fix-newly-reads-a-never-initialised-member | guard scope:portable | recur:1 saves:1 miss:0 | when a fix makes code READ a member/field it did not read before, that member's initialisation is now load-bearing and must be… — see ## LSN-049 below
LSN-050 | op:test type:apparatus-cannot-observe-the-thing-asserted | guard scope:portable | recur:2 saves:1 miss:1 | a test that asserts something is BLOCKED, HIDDEN, or otherwise prevented must first demonstrate that its apparatus can observe… — see ## LSN-050 below
LSN-051 | op:design op:cycle type:reparent-changes-the-exposure-set-not-just-the-lifetime | guard scope:portable | recur:1 saves:0 miss:1 | a fix that changes an object's OWNER/PARENT to a shorter-lived one does not merely close the lifetime bug it targeted — it… — see ## LSN-051 below
LSN-052 | op:decision type:decision-premise-silently-invalidated-by-a-later-fix | guard scope:portable | recur:1 saves:0 miss:1 | when a change moves the TRIGGER of a previously-decided behaviour from a RARE context to a ROUTINE one, the earlier DEC whose… — see ## LSN-052 below
LSN-053 | op:verify op:decision type:surviving-mutant-read-as-dead-code | guard scope:portable | recur:1 saves:1 miss:1 | a mutation that SURVIVES the suite means exactly one thing — no current test kills it — and NEVER "the code it mutated is dead". — see ## LSN-053 below
LSN-054 | op:build op:cycle type:memory-safe-is-not-stop-when-asked | guard scope:portable | recur:1 saves:0 miss:1 | a self. — see ## LSN-054 below
LSN-055 | op:cycle op:verify type:finding-reported-via-its-hardest-trigger | guard scope:portable | recur:1 saves:1 miss:1 | when a report (adversary, scout, builder) establishes that a guard is MISSING, do not accept its trigger story as the complete… — see ## LSN-055 below
LSN-056 | op:build op:verify type:shared-stub-blast-radius | advisory scope:portable | recur:1 saves:1 miss:0 | when a stub/fixture file is compiled into N test targets, changing a previously-INERT method to do real work must be verified by… — see ## LSN-056 below
LSN-058 | op:build op:cycle op:verify type:unedited-guard-promoted-from-dead-to-load-bearing | guard scope:portable | recur:1 saves:1 miss:1 | when a change makes a frame RESUME where it previously RETURNED — a return→continue, a new re-drive, a loop that now iterates… — see ## LSN-058 below
LSN-059 | op:verify type:simultaneous-mutation-is-not-per-guard-coverage | guard scope:portable | recur:1 saves:0 miss:1 | mutating N guards SIMULTANEOUSLY proves only that SOME slot notices the aggregate; — see ## LSN-059 below
LSN-060 | op:design op:cycle type:suspension-set-omits-the-collection-element | guard scope:portable | recur:1 saves:0 miss:1 | a suspension-point safety analysis must enumerate the ROW / ITEM / ELEMENT the frame is holding, not only this, its collaborators and its owner. — see ## LSN-060 below
LSN-062 | op:verify op:build op:cycle type:evidence-collected-under-one-pinned-runtime-backend | **guard→MECHANISM 2026-08-15 scope:portable** | recur:1 saves:0 miss:1 | NOW BACKED BY MECHANISM, one day after capture: the target is registered with ctest TWICE — testGarminConnectSyncDialogClose… — see ## LSN-062 below
LSN-063 | op:cycle op:design op:verify type:guard-reinforcing-an-already-distributed-invariant | guard scope:portable | recur:1 saves:0 miss:1 | a new guard can be individually load-bearing against its own test and still be UNREACHABLE through every production route that… — see ## LSN-063 below
LSN-064 | op:commit op:verify type:mechanism-repair-uncommitted-so-the-gate-runs-the-old-one | guard scope:portable | recur:1 saves:0 miss:1 | pre-commit (and any hook framework that stashes unstaged work) runs the COMMITTED copy of the hook, not the one in your working… — see ## LSN-064 below
LSN-065 | op:delegate type:briefing-mandates-a-step-outside-the-agent's-tool-grant | guard scope:portable | recur:1 saves:0 miss:1 | a verification step you MANDATE in a briefing must be executable within that agent's tool grant — otherwise you have made an… — see ## LSN-065 below
LSN-066 | op:commit op:verify op:phase-exit type:inventory-of-uncommitted-work-that-only-looked-at-git-status | guard scope:portable | recur:1 saves:0 miss:1 | git status does not answer "what uncommitted work exists here" — it answers "what is dirty in this worktree right now". — see ## LSN-066 below
LSN-075 | op:decision op:build type:invalidation-justified-only-on-safety-never-costed-for-what-it-destroys | guard scope:portable | recur:1 saves:0 miss:1 | when a repair adds a CONTAINER of correlation state, every site that INVALIDATES that container must be re-derived for what the… — see ## LSN-075 below
LSN-076 | op:decision type:disproof-argues-from-a-behaviour-the-accepted-option-changes | guard scope:portable | recur:1 saves:0 miss:1 | when a decision's DISPROOF cites CURRENT behaviour as a reason to reject an alternative, re-check that the ACCEPTED option does… — see ## LSN-076 below
LSN-077 | op:verify op:cycle type:hand-enumeration-past-the-point-where-it-is-the-right-tool | guard scope:portable | recur:1 saves:0 miss:1 | once a component carries FIVE-PLUS interacting guard mechanisms, stop enumerating click sequences by hand and install a… — see ## LSN-077 below
LSN-079 | op:build op:verify op:cycle type:observer-embedded-in-the-object-it-observes | guard scope:portable | recur:1 saves:0 miss:1 | an observer that lives INSIDE the object it observes dies with it — so every delivery point that can fire after the subject's… — see ## LSN-079 below
LSN-080 | op:verify op:gate type:a-clean-build-gate-run-in-a-REUSED-build-directory-is-not-a-clean-build | guard scope:portable | recur:2 saves:0 miss:2 | REFINEMENT 1 (2026-08-23, SAME DAY, SAME FINDING — ORCH-036's SECOND wrong formulation, caught by the USER, not by me). — see ## LSN-080 below
LSN-081 | op:verify op:gate type:a-test-budget-set-by-the-CALLER-makes-the-verdict-a-function-of-machine-load | guard scope:portable | recur:1 saves:0 miss:1 | A GATE WHOSE TIME BUDGET LIVES IN THE INVOKING COMMAND RATHER THAN IN THE TEST IS NOT A GATE — its verdict is a function of… — see ## LSN-081 below
LSN-082 | op:verify op:gate op:ledger-update type:a-mechanism-that-reports-an-unreadable-input-as-not-blocking-is-failing-open | guard scope:portable | recur:1 saves:0 miss:1 | WHEN A CHECK CANNOT READ ITS INPUT, THE ONLY SAFE VERDICT IS FAIL. — see ## LSN-082 below
LSN-083 | op:gate op:ledger-update type:a-gate-whose-actions-cannot-change-its-own-pass-criteria | guard scope:portable | recur:1 saves:0 miss:1 | BEFORE PUBLISHING A GATE, TRACE ITS ACTIONS THROUGH THE MECHANISM THAT COMPUTES ITS PASS CRITERIA. — see ## LSN-083 below
LSN-084 | op:mutation-proof op:restore type:git-checkout-on-a-file-with-PRIOR-uncommitted-changes-discards-all-of-them | guard scope:portable | recur:1 saves:0 miss:1 | git checkout -- <file> DOES NOT UNDO "the edit I just made" — it restores the ENTIRE working-tree file to HEAD, discarding every… — see ## LSN-084 below
LSN-078 | op:ledger-update op:orient type:budget-line-carries-a-verdict-nobody-recomputes | guard scope:portable | recur:1 **saves:2** miss:1 | SAVE #2, 2026-08-22: this guard is what made the commit-safety gate re-run the suite instead of trusting CURRENT's "full… — see ## LSN-078 below
LSN-072 | op:build op:cycle op:verify type:no-pump-claim-derived-from-a-target-that-stubs-the-collaborator | guard scope:portable | recur:1 saves:0 miss:1 | a green run on a target that STUBS a collaborator to a no-op cannot support any claim of the form "nothing between X and Y… — see ## LSN-072 below
LSN-073 | op:build op:cycle type:suspension-point-census-taken-from-known-collaborators-not-from-a-grep | guard scope:portable | recur:1 saves:0 miss:1 | enumerate suspension points by GREPPING processEvents( and QEventLoop/exec() in the function under guard AND in everything it… — see ## LSN-073 below
LSN-074 | op:decision op:build type:identity-key-narrower-than-the-space-it-must-separate | guard scope:portable | recur:1 saves:0 miss:1 | a per-transfer/per-request ticket keyed on a STRING must be checked against every collection that can generate that same string. — see ## LSN-074 below
LSN-071 | op:delegate type:briefing-asserted-a-test-apparatus-that-does-not-exist | guard scope:portable | recur:1 saves:0 miss:1 | before a briefing tells an agent to REUSE an existing test apparatus — a counter, a fixture, a helper, a fake — GREP FOR ITS SYMBOL. — see ## LSN-071 below
LSN-068 | op:decision op:build op:cycle type:state-armed-at-N-consumed-at-M-invalidated-nowhere | guard scope:portable | recur:1 saves:0 miss:1 | any state that is ARMED at N sites and CONSUMED at M sites must have its INVALIDATION sites enumerated in the SAME decision entry… — see ## LSN-068 below
LSN-069 | op:build op:verify type:an-assertion-whose-value-is-FORCED-is-a-premise-not-a-verdict | guard scope:portable | recur:2 saves:0 miss:2 | REFINEMENT 1 (2026-08-23, A3-R038-F2) — THE FORCING AGENT NEED NOT BE THE FIXTURE. — see ## LSN-069 below
LSN-070 | op:cycle op:build type:reachability-claim-that-does-not-terminate-at-an-emitting-call-site | guard scope:portable | recur:1 saves:0 miss:1 | a REACHABILITY claim must terminate at a concrete emitting call site with file:line — not at a function that COULD return the triggering value. — see ## LSN-070 below
LSN-067 | op:delegate op:verify type:mandate-stripped-the-targets-pinned-configuration-not-just-the-gates-pin | guard scope:portable | recur:1 saves:0 miss:1 | "run it with no environment override" is not a stricter gate — it is a DIFFERENT PROGRAM. — see ## LSN-067 below
LSN-061 | op:delegate op:cycle type:briefed-from-a-reproduction-instead-of-the-finding-row | guard scope:portable | recur:1 saves:0 miss:1 | when dispatching work against a finding that ALREADY EXISTS in the ledger, brief from the FINDING ROW, not from a fresh reproduction of the symptom. — see ## LSN-061 below
LSN-057 | op:build op:cycle type:callback-driven-loop-branch-returns-without-rearming | guard scope:portable | recur:1 saves:1 miss:0 | in a loop shaped "process ONE item, return, and let the completion callback re-drive me", every branch must either initiate the… — see ## LSN-057 below
LSN-045 | op:decision type:dec-internally-inconsistent-about-its-own-target | guard | recur:1 saves:1 miss:1 | a DEC that states the same code fact in two places must be SELF-CONSISTENT, and its alignment probe must be RUN ONCE against the… — see ## LSN-045 below
LSN-044 | op:id-alloc type:unregistered-id-series-invites-guessing | guard scope:portable | recur:1 saves:0 miss:1 | never cite or allocate an id from a series that WIKI REGISTRIES does not list — if the series is absent, register it FIRST and take next from there; — see ## LSN-044 below
LSN-038 | op:verify type:snapshot-suffix-unrecognized-by-guard | advisory scope:portable | recur:2 saves:0 miss:1 | RECURRED 2026-09-04, orchestrator this time (not the prior DEC-026 case's author): snapshotted CloudService. — see ## LSN-038 below
LSN-085 | op:build op:test type:fixture-growth-unbounded-in-one-binary scope:portable | advisory | recur:1 saves:0 miss:0 | a single QTest binary growing past a fixture-growth cap (slot count + fuzzer iterations) with no split path (QTest selects functions, cannot exclude one) is a SKILL-level gap, not yet a rule — see ## LSN-085 below
LSN-086 | op:test-design op:verify op:mutation-proof type:fake-pins-a-nonexistent-library-api | guard scope:portable | recur:1 saves:0 miss:1 | A FAKE MAY ONLY DEFINE WHAT THE REAL LIBRARY ACTUALLY HAS — ship a real-dependency contract test (attributes AND parameter order) that skips, not fails, when the dependency is absent; a mutation score computed against fakes measures fake-fidelity and can CERTIFY the very defect it should kill — see ## LSN-086 below

---

## LSN-001
sig:    create-file / duplication / any-path
level:  guard      since:P0(bootstrap)   recur:n/a   saves:1   miss:0
saves:  2026-08-07 (ORCH-001 repair) — the hook DENIED `cp /tmp/hdr.txt unittests/CMakeLists.txt`,
        a clobber-shaped shell idiom used where an in-place edit was intended, on an existing tracked
        file. Genuine catch, not an LSN-036 false positive: the correct tool was Edit, and the denial
        was honoured rather than routed around.
tags:   op:create-file, type:duplication
trigger:about to create/overwrite a file or directory (Write, Edit, MultiEdit, Bash)
mistake:(seeded, not observed in this project) re-creating a file/folder that already
        exists under a different assumed path — the canonical failure mode the whole
        wiki-memory anti-duplication design exists to prevent
rule:   target path MUST be absent from WIKI MAP; if MAP lists it, open the existing file
check:  look up the target in WIKI MAP → must resolve to one canonical entry
origin: this project skipped the advisory stage — the deterministic hook is already
        installed and wired (.claude/settings.json PreToolUse → anti_duplication_guard.py)
history:migration (2026-07-04): captured directly at guard level per briefing; mechanism
        already bundled — see the hook's own "enforcement endpoint" note in
        references/lessons-memory.md

## LSN-002
sig:    id-alloc / cross-ledger-collision / DEC-TEST-REQ
level:  guard      since:migration(2026-07-04)   recur:2   saves:0   miss:0
tags:   op:id-alloc, op:migration, type:cross-ledger-collision
trigger:allocating, citing, or reading an ID (REQ/DEC/DES/TEST/VAL) in a multi-ledger project
mistake:workflow-aicoach and workflow-garminconnect independently both reached DEC-013 and
        both have TEST-00x entries with different meanings — a naive global-registry read
        would treat these as one range and silently mis-cite or double-allocate
rule:   every ledger owns its own 001-based counter; NEVER merge or globally renumber; all
        cross-ledger citations use the `<ledger>:<ID>` prefix (e.g. coach:DEC-013 vs
        garmin:DEC-013) — a convention this project already started in
        workflow-garminconnect/decisions.md's own header, now made explicit project-wide
check:  before allocating REGISTRIES.next for any ID type, confirm which ledger's counter
        you are reading (WIKI.md REGISTRIES lines are ledger-qualified for this reason)
origin: migration discovery — decisions.md header ("Cross-project refs use `coach:DEC-NNN`")
        + traceability.md TEST-001..020 (aicoach) vs traceability.md TEST-001..T-004 (garmin)
history:migration (2026-07-04): captured at guard level immediately (2 independent
        collisions found: DEC and TEST ranges) — recurrence already observed, not
        hypothetical

## LSN-003
sig:    prd / underspecified-nfr / acceptance-criteria
level:  guard      since:migration(2026-07-04)   recur:11   saves:0   miss:0
tags:   op:prd, phase:P1, type:underspecified-nfr
trigger:writing or reviewing a non-functional requirement (NF-Perf/Sec/Reliab/Threads/…)
        before it enters an A1 (first adversarial) cycle
mistake:the garmin ledger's A1/P1 cycle raised 11+ blocking findings in one pass, nearly
        all of the shape "REQ-NF-X missing a quantified budget/cap/key-definition"
        (A1-006 perf baseline, A1-007 trust-store spec, A1-008 atomic-write gap, A1-009
        stall budget, A1-012 concurrent-sync semantics, A1-013 hard-cap, A1-015 dedup key,
        …) — all fixed in the same cycle but at real review cost
rule:   an NF-* requirement is not ready for A1 until it states a number, a key, or an
        explicit boundary condition (not "reasonable", "fast", "safe")
check:  grep prd.md for NF-* rows lacking a digit/threshold/named-field before opening A1
origin: findings.md rows A1-006, A1-007, A1-008, A1-009, A1-012, A1-013, A1-014, A1-015,
        A1-016, A1-017 (workflow-garminconnect) — 11 same-signature blocking findings
history:migration (2026-07-04): captured at guard level directly — recur:11 in a single
        cycle vastly exceeds the recur:2 promotion threshold

## LSN-004
sig:    design / missing-seam / third-party-library
level:  guard      since:migration(2026-07-04)   recur:1   saves:0   miss:0
tags:   op:design, type:missing-seam
trigger:designing a component that will call a mutable/undocumented third-party library
        or service (e.g. python-garminconnect, an unofficial SSO flow)
mistake:DES for REQ-002 initially had no isolation boundary around `python-garminconnect`;
        A2-004 ("adapter seam absent → swap cost unbounded") was raised as blocking and
        fixed by retrofitting DES-012 (single point of library import) after the review
        cycle, rather than being designed in from the start
rule:   when a design depends on a library whose API/ToS the team does not control, the
        seam (single-file/single-class import boundary) is part of the FIRST design draft,
        not a post-hoc fix
check:  before DES is marked drafted, confirm exactly one file/class imports the volatile
        dependency directly
origin: findings.md A2-004 (fix-now) → design.md DES-012; high cost (swap-cost unbounded)
        justifies guard on a single occurrence per lessons-memory.md escalation rule 4
history:migration (2026-07-04): captured at guard level (high-cost single occurrence)

### migrated-from-index-row 2026-09-06 (compaction; verbatim, not re-worded)
any call into a mutable third-party library needs its adapter/interface seam defined before first implementation, not retrofitted after review

## LSN-005
sig:    design / security-invariant-on-read / file-permissions
level:  guard      since:migration(2026-07-04)   recur:1   saves:0   miss:0
tags:   op:design, type:security-invariant
trigger:designing or reviewing any code path that reads a previously-written
        security-sensitive file (tokens, secrets, credentials cache)
mistake:the per-athlete token-file design enforced 0700/owner-only perms on WRITE but did
        not validate perms on READ (A2-005, blocking) — a file widened by an external
        process (backup tool, misconfigured share) would be silently trusted
rule:   any invariant enforced at write time (permissions, format, ownership) must be
        re-checked at read time too, and the read must refuse/fail-safe on violation
check:  for every "written with mode X" design line, confirm a matching "refuses to load
        if mode wider than X" check exists
origin: findings.md A2-005 (fix-now) → design.md DES-002 invariant (refuse load on
        wider-than-owner permissions); security-sensitive, high cost → guard on first
        occurrence
history:migration (2026-07-04): captured at guard level (security-sensitive, high cost)

## LSN-007
sig:    commit / hook-mutation-unverified / formatter-reorder
level:  guard      since:P2.2(2026-07-04)   recur:1   saves:0   miss:0
tags:   op:commit, op:code, type:hook-mutation-unverified, component:build
trigger:any commit where a pre-commit hook reports "files were modified by this hook";
        any C++ TU whose include order is semantic (Python.h-before-Qt)
mistake:committing TEST-005's PyEmbeddedAdapter slice, the clang-format pre-commit hook
        re-sorted the test TU's includes, moving Python.h AFTER the Qt headers (Qt `slots`
        macro vs CPython object.h `slots` field → compile break). The files were re-staged
        and committed; an immediate ctest run even "passed" — against the STALE binary.
        Only a from-scratch rebuild exposed that the committed code did not compile.
rule:   (1) a hook that mutates files invalidates every build/test result obtained before
        the mutation — rebuild and re-run the affected test labels on the post-hook tree
        before treating the commit as verified; never trust a ctest PASS without a
        preceding successful build of the same sources. (2) semantically-ordered includes
        must be fenced with `// clang-format off/on` so formatters cannot reorder them.
check:  after any "files were modified by this hook" line: `cmake --build` the affected
        targets (must succeed) then ctest the affected labels, THEN commit. For any TU
        including Python.h alongside Qt: confirm the fence markers exist.
origin: commit 0cdcc27eb (initial, broken) — caught by orchestrator re-verify, fixed by
        amend with clang-format fences in PyEmbeddedAdapter.cpp + testGarminConnectPyAdapter.cpp
history:2026-07-04: captured at guard level — high cost (broken master commit), silent
        failure mode (stale-binary PASS masked it); relates to the include-order Watch
        item in wiki/architecture.md, which predicted the hazard for unity builds but
        missed the formatter as the reordering agent

### migrated-from-index-row 2026-09-06 (compaction; verbatim, not re-worded)
if a pre-commit hook modifies files, all prior build/test evidence is void — rebuild + re-run affected tests before accepting the commit; protect semantic include order with clang-format off markers

## LSN-008
sig:    ledger-update / index-vs-detail-drift / traceability-matrix
level:  guard     since:P2.2(2026-07-05)   recur:4   saves:1   miss:3   escalated:2026-07-05
tags:   op:ledger-update, op:byproduct, phase:P2, type:index-vs-detail-drift
trigger:updating traceability.md (or any index-first ledger) as the byproduct of a slice
        that added new DES/TEST/commit artifacts
mistake:the PyEmbeddedAdapter + tile-routing slice appended two fresh detail tables to
        traceability.md (the "PyEmbeddedAdapter slice artifacts" + "wizard tile-routing
        slice artifacts" appendices, lines 133-150) and updated state.md/WIKI.md — but the
        file's OWN primary DES index and REQ-002 REQ→DEC→DES→TEST→COMMIT matrix row were
        left stale: no DES-013 row, no TEST-005/006, no 212a4c258/e9e017fe1 commit refs, and
        the banner still read "Last updated: 2026-05-24". VAL-007 Check 6 (LEDGERS) FAILed on
        the index-vs-digest disagreement.
rule:   a byproduct ledger update is not complete until the PRIMARY index/matrix rows for the
        changed IDs are current — appendix/detail tables never substitute for the top-level
        index. Update index rows + banner date in the same action that adds the detail.
check:  after any traceability.md edit, grep the new DES/TEST IDs + commit hashes in the
        PRIMARY DES index and REQ matrix (not just appendices); confirm the banner date
        matches the slice date.
origin: VAL-007 (2026-07-05) Check 6 FAIL → orchestrator refreshed traceability.md DES index
        + REQ-002 matrix row + banner; saves:1 credited to the CLV pass that caught it
history:2026-07-05: captured at advisory level (VAL-007 Check 6 — traceability primary index
        vs appendix). ESCALATED to guard the same day (VAL-008 Check 6): recurred in a second
        location — ledger state.md's own ## reqs structured table left stale (TEST-007 absent,
        stale T-005/T-006 counts) while its prose banner was updated, PLUS a slot-count miscount
        (QTest-reported total 9/12 copied where named-slot count 7/10 belonged) propagated into
        three files. miss:1 recorded — the advisory did not prevent recurrence, so it is now a
        hard pre-flight guard. Root fix: standardized on named-slot counts with an explicit
        QTest-total annotation so the two metrics can't be conflated again.
        2026-07-08 (VAL-009 Check 6): recurred a THIRD time — REQ-007 download-chain byproduct
        updated the prose banners and the full design.md DES-001a/DES-013 sections, but left the
        traceability DES-index STATUS cells (DES-001/001a/013 still "Auth-only", no TEST-009/010),
        the state.md ## des status cells, and the root STATE CASCADE note ("IGarminPyAdapter Auth-
        only") stale. recur:3/miss:2 — the guard fired the same signature but its CHECK list did not
        name the DES-index Status column or the CASCADE note. Guard broadened this pass to enumerate
        DES-index STATUS cells + STATE CASCADE among the structured cells to diff. Promotion candidate:
        a deterministic pre-CLV lint diffing each changed DES/TEST id against its index status string.
        2026-07-11 (VAL-010 Check 6): recurred a FOURTH time — the REQ-004 byproduct updated the traceability
        DES-002/006 index rows but left the DES-012/013 index STATUS rows stale (no DEC-014/dump_tokens/TEST-013,
        no tokenBlob) while design.md/state.md were current. recur:4/miss:3. Co-occurred with a NEW sibling
        signature now split out as [[LSN-011]] (design.md prose ITSELF asserting a deferred item as done).
        2026-07-12 (VAL-011 Check 6/8): recurred a FIFTH time, WIDEST spread yet — across REQ-004 + REQ-006
        Slice-A + Slice-B byproduct updates the orchestrator refreshed the appendix artifact tables + root
        STATE.md/WIKI phase line, but left: (a) the traceability PRIMARY REQ matrix rows for REQ-004 (`_uncommitted_`)
        AND REQ-006 (all `—`); (b) DES-012/013 index STATUS rows still "DEFERRED"; (c) design.md DES-013 body — the
        old 2-arg `PyEmbeddedAdapter(...,tokenstorePath)` class snippet + step-4 sequence + a SECOND "DEC-014 note"
        under "what this does NOT cover" still reading "STILL forwards it today"; (d) the ENTIRE local
        `.claude/workflow-garminconnect/state.md` (banner 2026-07-08, ## reqs/## des/## vals/## open all pre-REQ-004);
        (e) WIKI REGISTRIES "T-015/16 … in build" though committed. VAL-011 (validator) caught all of it. recur:5,
        miss:4, saves:2. The guard's CHECK list has now been broadened four separate times and STILL missed the local
        state.md + design.md-second-note + primary-matrix — a strong promotion signal: escalate to a DETERMINISTIC
        pre-CLV/pre-commit lint that, for any commit touching a ledger, diffs each changed REQ/DES/TEST id against its
        primary-matrix row, DES-index status cell, local state.md row, and design.md refinement notes, failing on any
        stale "DEFERRED"/`_pending_`/`_uncommitted_`/"in build" string for an id that is committed/resolved. Until that
        mechanism exists the guard remains advisory-in-practice (it fires post-hoc via the CLV, not pre-commit).
        2026-07-12 (VAL-011 re-verify, 2nd pass): recurred a SIXTH time, in a NEW locus the five prior fixes never
        targeted — ROOT `STATE.md`'s OWN body sections (CASCADE line, NEXT_GATE, CHANGESET, COUNTS, VAL-registry line)
        left describing the pre-Slice-B/VAL-010 world while the SAME file's top banner already recorded both REQ-006
        commits + VAL-011. i.e. the drift is now intra-file (banner current, body stale), not just cross-file. The
        orchestrator's first repair pass fixed traceability primary matrix + DES index + design.md + local state.md +
        WIKI but treated STATE.md as "banner-updated = done" and missed its lower sections. recur:6, miss:5, saves:3.
        DEFINITIVE promotion trigger: six recurrences across six loci prove the manual CHECK list cannot be the control —
        the fix is the deterministic pre-commit/pre-CLV lint (already specified above), which must scan root STATE.md's
        CASCADE/NEXT_GATE/COUNTS/CHANGESET/VAL-registry lines against its own banner + committed git state, not only the
        cross-file index cells. Orchestrator remediation this pass: holistic full-file rewrite of STATE.md's stale
        sections in ONE pass (not piecemeal), + a self-run grep scan for stale strings before re-dispatching the validator.
        2026-07-12 (VAL-011 re-verify, 3rd pass): recurred a SEVENTH time, one level DEEPER — the index/matrix/banner/root-
        STATE cells were all correct, but the SAME fact (tokenstore_path forwarding removed by Slice B) survived in PROSE:
        design.md DES-003 body ("forwarded verbatim"), the DES-012 "### Surface" code snippet (old 3-arg __init__) + its
        "what lives where" table row + the DES-013 "Serves:" line, plus a traceability per-REQ "artifacts" NOT-done row and
        a stale TEST-014 slot count (5 vs 8). Index-level fixes were necessary but NOT sufficient. recur:7, miss:6, saves:4.
        Remediation this pass: fixed the 6 prose loci, then ran a DETERMINISTIC grep-sweep of the whole ledger tree for the
        old fact's literal strings (forward/forwarded tokenstore, 3-arg ctor, "5 slots", "in progress", "next:garmin-011")
        + confirmed only the benign LSN-008-narrative hit remained, BEFORE re-dispatching. This grep-sweep is the standing
        manual proxy for the not-yet-built lint; the lint must scan prose subsections + per-REQ artifact tables, not only
        index cells. Sibling test-count-drift rule folded in: when a test file gains N slots (A3 hardening), grep every
        artifact table for the pre-hardening count in the same commit.
        2026-07-13: **PROMOTED TO MECHANISM (DEC-015).** The manual CHECK list was broadened 4× across VAL-007…011 and
        STILL missed (recur:7, miss:6) — proving the manual control cannot be the enforcement. Root-caused as a DATA-MODEL
        defect, not a diligence defect: the same status fact was physically stored in ~7 places, so every byproduct update
        had to hand-sync all of them. Fix = SSOT: status single-homed in traceability.md (REQ/DES/TEST/VAL) + decisions.md
        (DEC); stripped from the non-canonical set (STATE.md/WIKI.md/wiki/*/design.md); the per-ledger state.md deleted
        (root STATE.md is the sole cursor — see [[LSN-014]]). Enforcement = `.claude/hooks/ledger_drift_lint.py` (TEST-017,
        absence-check, pre-commit + CLV): it deterministically fails any id+status pairing outside a canonical home, so the
        cross-file duplication class is now impossible to reintroduce silently. Verified: lint drove 108 live findings → 0
        tree-wide; full CLV PASS (9/9) after a repair pass. **Residual (the lint does NOT cover):** (a) drift INSIDE the
        canonical files themselves; (b) pointer/index files outside the scan set (conventions.md/workflow-INDEX.md). Both
        were caught by the closing CLV, not the lint — the reason the CLV gate remains alongside the mechanism. That
        verification discipline is captured as [[LSN-015]]. This lesson is now level:MECHANISM; the manual grep-sweep
        remediation above is retained only as the fallback when the hook/runtime is unavailable.

### migrated-from-index-row 2026-09-06 (compaction; verbatim, not re-worded)
**SAVE 2026-08-18 (REQ-028 Phase-2 gate) — AND IT CAUGHT THE ORCHESTRATOR, WHICH IS THE POINT.** Writing up the gate verdict, the orchestrator paired TEST-107 with a verdict word twice inside STATE.md (`STATE.md:14` and `:39`) — per-id TEST status in the cursor, exactly the DEC-015 violation this lint exists to stop, committed by the role that owns the rule while writing *about* rule compliance. Both copies exited 1 with precise locations; both spots were reworded to point at traceability.md instead, and both copies then exited 0. **No bypass was considered** ([[LSN-036]] class): the denial was ground truth, not an obstacle. The instructive part is the failure mode — the violation entered in the byproduct step at the END of a long verification, in prose whose subject was governance discipline itself. A rule you are actively reciting is not a rule you are automatically honouring, which is precisely why this one is a MECHANISM and not an advisory. | **MISS 2026-08-14 (ORCH-015) — the ORCH-010 repair is INCOMPLETE: it fixed the attributive-adjective position and left the PREDICATE position (`is … deferred.`) still reading as an assignment, and it never checks whose SUBJECT the status word belongs to (a line citing `DEC-030` while reporting `47/47 STILL GREEN` about the SUITE fires). Isolated 3-line repro run before reporting; the true-positive case in the same probe still fires correctly, so this is a precision bug, not a coverage loss. Prose deliberately NOT reworded. Must be repaired before the REQ-027 commit gate — it is a pre-commit hook currently red on correct text.**  **SAVE 2026-08-14 (REQ-027 byproduct merge) — a TRUE positive, against the orchestrator, and NOT the ORCH-010 false-positive shape.** Merging the REQ-027 build I wrote "clause (e) deferred with O-R027-01 to DEC-033" onto the WIKI TEST-registry line and a matching phrase in STATE, pairing `deferred` with ~30 ids on WIKI.md:60 and with TEST-095/098 on STATE.md:586. The repaired `find_statuses()` was right to fire: unlike ORCH-010's adjectival "frame-counted deferred reaper", this was `deferred` as a status VERB assigning lifecycle to ids outside their canonical home, which is precisely what DEC-015 forbids. **The text was REWORDED, not the mechanism bent and not the finding suppressed** — "clause (e) belongs to DEC-033" carries the same information with no status assignment, and the lifecycle claim already lives in traceability.md where it belongs. Verified clean afterwards (exit 0, unpiped). This is the discrimination the ORCH-010 repair bought: before it, this line would have been indistinguishable from prose and the lint's credibility was being eroded by subtraction; after it, the lint fires on assignments and stays quiet on adjectives. | **MISS 2026-08-11 (ORCH-010) — the MECHANISM ITSELF false-positives on prose.** The lint flagged `DEC-031 … (Option B — frame-counted deferred reaper)` because the adjective "deferred" co-occurs with an id on the line. Reproduced in isolation (2-line file: the status-word line fails, an id-only line passes). Root cause is by design, per the lint's own docstring — *"ANY id-token + status-token co-occurrence on a line is a violation"*; `find_statuses()` is a bare `rx.search(line)` with no assignment-shape check. **The mechanism embodies exactly the error [[LSN-034]] warns about: co-occurrence asserted as membership.** Two tokens (`committed`, `in progress`) were already deleted for this same reason — precision-by-subtraction that erodes coverage each time; `deferred` is the third and is a real vocabulary word, so deletion is the wrong fix. **The offending text was NOT reworded** (naming the decided mechanism in STATE is correct); recorded instead, then the MECHANISM WAS REPAIRED 2026-08-12 rather than the prose bent around it — `find_statuses()` gained assignment-shape discrimination (adjectival use is prose; punctuation / EOL / table-cell / function-word remain assignments), 15 pre-existing cases still green + 6 new two-directional cases = 21/21, mutation-proven, installed copy synced. **The escalation is what forced it: 1 false positive became 27 once an ordinary hyphenated adjective landed on a line carrying 20+ ids, and the lint is a pre-commit gate.** Standing rule this reinforces: when a mechanism must be routed around to get work done, repairing the mechanism IS the work. **PROMOTED TO MECHANISM 2026-07-13 (DEC-015 → `.claude/hooks/ledger_drift_lint.py`, TEST-017).** Root cause retired by SSOT: per-id lifecycle status is now single-homed in `traceability.md` (REQ/DES/TEST/VAL) + `decisions.md` (DEC) and STRIPPED from the non-canonical set (STATE.md/WIKI.md/wiki/*/design.md); the per-ledger `state.md` was deleted (one cursor). The absence-check lint (pre-commit + CLV) deterministically fails any id+status pairing outside a canonical home — the cross-file duplication class that drove all 7 recurrences can no longer occur. **Residual (NOT covered by the lint, stays a CLV concern):** drift INSIDE the canonical files themselves and in pointer/index files the lint doesn't scan — see [[LSN-015]]. Design principle: [[LSN-014]]. Sibling: [[LSN-011]] (design-note false-done, subsumed by the design.md status-strip).

## LSN-009
sig:    test / loose-timeout-bound-survivor / teardown-assertion
level:  advisory  since:P2.2(2026-07-05)   recur:1   saves:1   miss:0
tags:   op:test, phase:P2, type:mutation-survivor, component:C11(Cloud)
trigger:writing or reviewing a test that bounds a teardown / timeout / retry-with-fallback
        path by asserting on elapsed time
mistake:TEST-006's chain-teardown asserts (`elapsed < 3000ms`) were sized as the SUM of both
        fallback ceilings (kQuitWaitMs=2000 + kTerminateWaitMs). A3 built and ran a real
        mutant deleting `m_thread.quit();` from `~GarminAuthChain()`: the suite still reported
        8/8 PASS while runtime went 294ms→12306ms and every teardown logged "Qt has caught an
        exception thrown from an event handler". The bound could not tell "graceful quit ran"
        from "always fell through to hard terminate()" — a mutation survivor sitting directly
        on DES-001 invariant 3.
rule:   an elapsed-time bound on a path with a graceful fast-path + slower fallback(s) must be
        asserted WELL UNDER the fast-path ceiling (e.g. < kQuitWaitMs/10), so skipping the
        fast path makes the test FAIL. A bound at the sum-of-all-fallbacks only catches a true
        hang, not a skipped-graceful-path regression. Pair it with an explicit test that
        deterministically forces the fallback and asserts the fallback's own bound.
check:  for any `QVERIFY(elapsed < N)` on teardown/timeout code, confirm N is below the
        graceful-path ceiling, not the total of all fallback timeouts.
origin: A3/REQ-002-TR finding A3-R002-TR-02 (blocking) — empirically executed mutant, not
        hypothesized; saves:1 credited to the A3 cycle that caught it
history:2026-07-05: captured at advisory level — first occurrence, portable test-design rule
        (scope:portable — travels to any future timeout/teardown test); escalate to guard if a
        second loose-bound survivor appears

### migrated-from-index-row 2026-09-06 (compaction; verbatim, not re-worded)
a bounded-teardown/timeout assertion must be tight enough to FAIL if the graceful fast-path is skipped (assert « the fast-path ceiling, not < the sum of all fallback ceilings) — a loose bound cannot distinguish "worked" from "fell through to the last resort every time"

## LSN-006
sig:    code / error-handling / broad-except-misroute
level:  advisory  since:migration(2026-07-04)   recur:1   saves:0   miss:0
tags:   op:code, type:error-handling, component:C11(Cloud)/C12(Python)
trigger:writing or reviewing exception handling in adapter/boundary code (C++/Python seam,
        any translation-of-third-party-errors layer)
mistake:an over-broad `except` clause in the Garmin auth-adapter path misrouted non-auth
        exceptions as auth errors (A3-R002-M6); only caught by mutation testing (A3), not
        by initial review
rule:   boundary-layer exception handlers should classify by exception type/shape before
        falling through to a generic handler; a bare/broad except at a translation
        boundary is a code-review flag
check:  before approving an except-clause at a library-boundary/translation layer, confirm
        it discriminates by exception class, not just message content
origin: findings.md A3-R002-M6 (fix-now) → test_non_auth_exception_is_not_misclassified_as_auth;
        caught by the A3 mutation-testing process working as designed (not a repeated
        process failure), so kept at advisory rather than guard
history:migration (2026-07-04): captured at advisory level (single occurrence, existing A3
        process already catches this class of bug)

### migrated-from-index-row 2026-09-06 (compaction; verbatim, not re-worded)
exception handlers at adapter/boundary layers must classify by type before a broad except, never swallow-and-misroute

## LSN-010
sig:    commit / commit-column-staleness / self-recording-byproduct
level:  advisory  since:P2.2(2026-07-08)   recur:1   saves:1   miss:0
tags:   op:commit, op:byproduct, phase:P2, type:commit-column-staleness
trigger:committing a feature slice whose changeset INCLUDES its own ledger byproduct
        (traceability.md/state.md/WIKI.md/STATE.md updated in the same commit as the code)
mistake:REQ-007's download-chain commit `1eb5a6a16` bundled the ledger byproduct, which — because
        the commit hash cannot exist until the commit is made — necessarily still read the REQ-007
        Commit column as `_pending_` and every banner as "uncommitted". No immediate ledger-record
        follow-up was run, so VAL-009 Check 6 FAILed against a landed-but-unrecorded commit
        (git HEAD = 1eb5a6a16, traceability said "_pending_ … uncommitted"). REQ-002 handled the
        same chicken-and-egg correctly via a dedicated follow-up commit `caa5c3e5b`; REQ-007 skipped
        that step.
rule:   any commit that bundles its own ledger byproduct MUST be followed immediately by a
        ledger-record step (amend or a `docs(...): record commit <hash> into ledgers` follow-up,
        the caa5c3e5b pattern) that fills the traceability Commit column with the just-created hash
        and flips uncommitted→committed banners — BEFORE the CLV gate runs, so the spine's COMMIT
        citation is never factually wrong for a landed change.
check:  before delegating the CLV, run `git log -1 --format=%h` and grep the just-closed REQ's
        Commit-column cell in traceability.md — the HEAD hash must appear there; no `_pending_`/
        "uncommitted" string may survive for a change that is already in HEAD.
origin: VAL-009 (2026-07-08) Check 6 FAIL — the code/design/test spine was clean; the sole FAIL was
        this recording gap. saves:1 credited to the CLV pass that caught it. Mechanically checkable →
        promotion candidate to a deterministic pre-CLV lint (HEAD hash vs Commit column).
history:2026-07-08: captured at advisory level (first isolated occurrence as a distinct step-miss;
        REQ-002 had done the follow-up, REQ-007 omitted it). Distinct from LSN-008 (which is about
        STRUCTURED-cell-vs-prose drift within the ledger); LSN-010 is about the ledger-vs-git-HEAD
        commit-recording step. Escalate to guard if a second self-recording commit ships without its
        record-commit follow-up. scope:portable — travels to any ledgered project.

### migrated-from-index-row 2026-09-06 (compaction; verbatim, not re-worded)
a feature commit that bundles its OWN ledger byproduct necessarily records "uncommitted"/`_pending_` (the hash doesn't exist yet); it MUST be followed immediately by a ledger-record step that fills the traceability Commit column with the just-created hash and flips uncommitted→committed banners, BEFORE the CLV gate. Mechanically checkable: `git log -1` HEAD hash vs the REQ row's Commit-column string.

## LSN-011
sig:    ledger-update / design-note-false-done / dec-cascade-note
level:  guard      since:P2.2(2026-07-11)   recur:1   saves:1   miss:0
tags:   op:ledger-update, op:byproduct, phase:P2, type:design-note-false-done
trigger:drafting a DEC's cascade impact into a target DES design.md entry (a "DEC-NNN refinement"
        note), ESPECIALLY at decision-acceptance time BEFORE the implementing build has run
mistake:DEC-014's cascade was written into design.md's DES-012 and DES-013 entries in present tense
        ("login() is constructed **without** a tokenstore path", "this class no longer forwards a
        tokenstorePath") at acceptance time. The REQ-004 build then DEFERRED exactly that item
        (B-R004-01 — __init__ still forwards the path). The notes were left asserting a deferred item
        as accomplished fact, contradicting the same file's class-shape comment, the code
        (garmin_client.py:75 + PyEmbeddedAdapter.cpp:235 still forward), and the honest deferral in
        STATE.md/state.md/findings.md. VAL-010 Check 6+9 FAILed.
rule:   a cascade item written into a design note must match its findings.md status: if open/deferred,
        write it as "target / DEFERRED (finding B-R004-01)", never present-tense done. A DEC-refinement
        note drafted before the build lands uses future/target tense for everything the build hasn't
        yet delivered.
check:  before the CLV gate, grep every "DEC-NNN refinement" note in design.md for present-tense
        completion verbs (no longer / now / constructed without / stops); for each, confirm the item
        is NOT an open/deferred row in findings.md. Mismatch = drift.
origin: VAL-010 (2026-07-11) Check 6+9 FAIL → orchestrator rewrote the DES-012/013 refinement notes to
        DEFERRED tense + synced the traceability DES-012/013 index rows; saves:1 credited to the CLV
        pass. Related to [[LSN-008]] (index-vs-detail drift) — same "false-done" family, but LSN-008 is
        stale-STRUCTURED-cell-vs-current-prose, whereas LSN-011 is the current prose ITSELF asserting an
        unbuilt item as done.
history:2026-07-11: captured at guard level on first occurrence — high cost (a design doc misrepresenting
        build state misleads every later reader + the next builder) per lessons-memory escalation rule 4,
        and it co-occurred with a 4th [[LSN-008]] recurrence. scope:portable.

### migrated-from-index-row 2026-09-06 (compaction; verbatim, not re-worded)
any DEC-cascade item written into a design.md "DEC-NNN refinement" note that is still open/deferred in findings.md MUST be in TARGET/DEFERRED tense, never present-tense "already true" — grep DEC-refinement notes for completion verbs (no longer/now/without/stops) and confirm each is not an open findings.md defer row, before the CLV gate

## LSN-012
sig:    verify / builder-lint-dirty-green / style-gate-deferred
level:  advisory  since:P2.2(2026-07-11)   recur:1   saves:1   miss:0
tags:   op:verify, op:delegate, phase:P2, type:builder-lint-dirty-green
trigger:running the Verification-Gate Evidence check on a builder GREEN report before committing
        its changeset, when the DEC-009 style gate (ruff/clang-format/mypy) is available in-env
mistake:REQ-006 Slice B's builder reported GREEN (pytest 15/15, garmin-py 20/20, garmin-fast 10/10)
        having run the test suites via the repo .venv, but left two dead `tokenstore` locals
        (ruff F841) orphaned when their tests dropped the 3rd ctor arg. The orchestrator Evidence
        check re-ran only the TESTS (green) and relied on the commit-time pre-commit hook to catch
        lint — which it did, aborting the first commit and forcing a fix-and-recommit cycle.
rule:   the Evidence check for a builder report includes running the DEC-009 style gate on the
        changed files (ruff --check + clang-format --dry-run + mypy) BEFORE the commit, not
        discovering it at commit time. A passing test suite does not imply lint-clean; dead code
        and unused-import/-local violations survive green tests. Builders should self-run the gate
        or flag it unrun (cf. finding B-R004-02); the orchestrator verifies it regardless.
check:  before `git commit` of a builder changeset, run the project's lint/format gate on the FILES
        list (or `pre-commit run --files <changeset>`); zero diffs / zero errors, else fix + re-verify.
origin: REQ-006 Slice B commit — first attempt aborted by ruff F841 (test_adapter_login.py:176,198);
        orchestrator removed the two dead locals, re-verified pytest, recommitted 3edb705cb. saves:1
        credited to the deterministic pre-commit gate catching it. Distinct from [[LSN-007]] (which is
        about a hook MUTATING files voiding prior build/test evidence); LSN-012 is about lint-dirty
        code slipping a green test suite and being caught late rather than during verification.
history:2026-07-11: captured at advisory level (first occurrence; deterministic pre-commit gate
        already enforces the floor). scope:portable — travels to any project with a lint/format gate.
        Escalate to guard if a second builder GREEN ships lint-dirty and is caught only at commit time.

### migrated-from-index-row 2026-09-06 (compaction; verbatim, not re-worded)
a builder GREEN report is not verified until the DEC-009 style gate (ruff/clang-format/mypy) has run on the changeset — the orchestrator Evidence check runs it BEFORE commit, not deferred to the commit hook. A builder with the tooling available (ran pytest via the repo .venv) can still leave lint-dirty code (dead locals, F841) a passing test suite won't surface.

## LSN-013
sig:    test / mask-coverage-gap / security-predicate-bitmask
level:  advisory  since:P2.2(2026-07-12)   recur:1   saves:1   miss:0
tags:   op:test, phase:P2, type:mutation-survivor, component:C11(Cloud)
trigger:writing or reviewing a test for a security/authorization predicate that combines several
        bit or flag classes with OR (file-mode Read/Write/Exec × Group/Other; a capability or
        permission bitmask; a set of "any of these forbidden flags" checks)
mistake:TEST-014's load-side permission refusal only exercised Read-class widened modes (0640, 0644).
        A3-R006 executed a mutant narrowing the production refusal mask from all six group/other bits
        to `ReadGroup|ReadOther` only; it SURVIVED the whole garmin-fast suite because no test set a
        Write-only (e.g. 0620) or Exec-only (e.g. 0601) widened mode — the two bit classes with zero
        coverage. The production mask was correct; the test simply did not pin it, so a future
        narrowing regression would pass silently on a security-critical path.
rule:   for a predicate that ORs N bit/flag classes, the test set must include at least one case per
        class that is NOT already exercised by another case — verifying each class independently
        contributes to the decision. Two variants inside one class (0640/0644 are both Read-class) do
        not substitute for coverage of the other classes.
check:  enumerate the bit/flag classes the predicate ORs; for each, confirm a test case sets ONLY that
        class's bit (with the rest owner-only/clear) and asserts the predicate fires. Missing class = gap.
origin: A3-R006-01 (blocking) — executed mask-narrowing mutant survived TEST-014; closed by two new
        slots (0620 group-write-only, 0601 other-exec-only), builder demonstrated the kill (narrowed
        mask → both FAIL → reverted → green). saves:1 credited to the A3 cycle that caught it.
history:2026-07-12: captured at advisory level, first occurrence. scope:portable — travels to any
        bitmask/flag-set security predicate. Sibling of [[LSN-009]] (both are "the assertion is too
        weak to catch a real mutant" test-design rules from executed A3 mutants).

### migrated-from-index-row 2026-09-06 (compaction; verbatim, not re-worded)
a test guarding a security predicate that ORs multiple bit/flag classes (file perms Read/Write/Exec × Group/Other; capability/permission bitmasks) must exercise ≥1 case per bit class NOT already covered by another — two Read-class widened modes (0640/0644) do NOT pin a Read|Write|Exec mask; a mutant dropping the Write*/Exec* bits survives. One case per uncovered class (e.g. 0620 write-only, 0601 exec-only) kills it.

## LSN-014
sig:    ledger-design / status-duplication / single-source-of-truth
level:  guard  scope:portable   since:P2.2(2026-07-13, DEC-015)   recur:n/a   saves:0   miss:0
tags:   op:ledger-design, op:ledger-update, type:status-duplication, scope:portable
trigger:designing or maintaining the governance/ledger memory of a project — deciding WHERE a
        per-id status, a phase, or a "where are we" cursor is written.
mistake:(root cause of [[LSN-008]], recur:7) the same logical fact — a REQ/DES/TEST/VAL's lifecycle
        status — was physically stored in ~7 files (traceability primary matrix + DES/DEC index cells,
        per-slice appendix tables, a per-ledger state.md, the root STATE.md body, design.md prose, WIKI
        REGISTRIES). Every byproduct update had to hand-sync all copies; with context lost between turns
        the sync was lossy, so one copy always drifted. Two cursors (a "condensed" root + a "full"
        per-ledger state.md) is the same defect at the cursor layer.
rule:   ONE logical fact = ONE storage location. (1) Per-id lifecycle status lives in exactly one
        canonical file (here: traceability.md for REQ/DES/TEST/VAL, decisions.md for DEC) and is NEVER
        restated elsewhere. (2) A project has exactly ONE live cursor — no per-ledger cursor AND a root
        cursor. (3) Design docs describe INTENT (target shape), not live status. (4) Non-canonical files
        reference ids WITHOUT a status token.
check:  before adding a status/phase string to any governance file, ask "is this the canonical home for
        this fact?" If not, reference the id and point to the canonical home instead. Enforce mechanically
        with an absence-check lint over the non-canonical set (see the [[LSN-008]] mechanism).
origin: DEC-015 (2026-07-13) — the SSOT redesign that promoted [[LSN-008]] to mechanism. Captured at
        guard because the design rule must hold for every future ledger, not just this one.
history:2026-07-13: captured as the durable design principle behind the LSN-008 promotion. scope:portable
        — a hard-won governance rule that travels to any future project's memory design. Verify the
        migration that establishes it with [[LSN-015]].

### migrated-from-index-row 2026-09-06 (compaction; verbatim, not re-worded)
ONE logical fact = ONE storage location. A per-id lifecycle status lives in exactly one canonical file and is never restated in cursors/design/wiki; and a project has exactly ONE live cursor (no per-ledger cursor + root cursor duplicate). "Condensed copy" and "full copy" of the same status are two things that drift — collapse them. Enforce absence elsewhere with a deterministic lint; keep design docs in INTENT tense (target shape), not live status. This is the SSOT design that retired [[LSN-008]]; see also [[LSN-015]] for verifying the migration that establishes it.

## LSN-015
sig:    cascade / named-target-unverified / migration-completeness
level:  guard  scope:portable   since:P2.2(2026-07-13, DEC-015)   recur:1   saves:0   miss:1
tags:   op:cascade, op:ledger-migration, type:incomplete-cascade, scope:portable
trigger:executing a DEC cascade or a governance migration whose text NAMES specific dependent files to
        edit, repoint, or delete-and-repoint.
mistake:DEC-015's cascade text explicitly named the files to repoint after deleting the per-ledger
        state.md ("all pointers — WIKI PAGES, workflow-INDEX, any 'full live cursor' refs — repointed").
        The orchestrator repointed the ones found by a PATH-based grep (`workflow-garminconnect/state.md`)
        but missed two files that referenced the cursor by its RELATIVE name (`state.md`): wiki/conventions.md
        and workflow-INDEX.md. Separately, DEC-015 promised to drop the traceability DEC-index Status column
        and did not. Both were declared-done-but-not-done; the absence-lint (scoped to the non-canonical set)
        could not see either, since one was in canonical/pointer files and the other was a structural shape.
rule:   when a cascade/DEC NAMES targets, grep-verify EACH named target actually changed before declaring
        the cascade complete — never infer completeness from the changeset file-list. Search by BOTH the full
        path AND the bare basename (relative references hide from a path-only grep). Corollary: a deterministic
        lint over a NON-canonical subset proves nothing about content INSIDE canonical files or about pointer/
        index files it doesn't scan — gate those with an explicit CLV check, not the lint alone.
check:  after a named cascade: for each named target, `grep` the changed file for the old string (must be
        absent) and the new (must be present); for a delete-and-repoint, `grep -rn` the tree for BOTH the full
        path and the basename of the deleted file — any live pointer is an incomplete cascade.
origin: DEC-015 closing CLV (2026-07-13) — the validator caught both misses (dangling state.md pointers +
        un-dropped DEC-index Status column) after the orchestrator declared the migration done. miss:1 credited:
        the cascade was declared complete while two named targets were stale; the CLV gate (not the mechanism)
        caught it.
history:2026-07-13: captured at guard, first occurrence, from the DEC-015 migration. scope:portable — applies
        to any named cascade/migration in any project. Directly complements [[LSN-014]] (the design) and closes
        the loop on [[LSN-008]] (the mechanism does not replace the CLV; it narrows what the CLV must still catch).

### migrated-from-index-row 2026-09-06 (compaction; verbatim, not re-worded)
when a DEC/cascade NAMES specific files to edit or repoint, grep-verify EACH named target was actually changed before declaring the cascade done — never infer completeness from the changeset file-list (a path-based grep also misses RELATIVE references, e.g. a pointer to `state.md` rather than `.claude/workflow-x/state.md`). Corollary: a deterministic lint scoped to a NON-canonical set proves nothing about content INSIDE the canonical files or in pointer/index files it doesn't scan — those need an explicit CLV check. Both misses (dangling `state.md` pointers in conventions.md/workflow-INDEX.md + an un-dropped DEC-index Status column) were caught by the DEC-015 closing CLV, not the lint — which is exactly why the CLV gate exists alongside the mechanism.

## LSN-019
sig:    delegate / briefing-symbol-claim-unverified / design-ahead-of-code
level:  advisory  scope:portable   since:P2.2(2026-07-18, REQ-003 Slice A)   recur:1   saves:0   miss:1
tags:   op:delegate, type:briefing-symbol-claim-unverified, scope:portable
trigger:writing a builder/agent briefing that asserts an EXISTING code symbol as fact — "enum X already
        has value Y", "field/method Z already exists", "this is already wired" — especially when the claim
        is drawn from a design/DES entry rather than the source file.
mistake:the REQ-003 Slice A briefing stated `GarminAuthFailure::MfaRequired` "ALREADY exists — do not
        re-add", taken from the DES-003a code sketch (which listed the full eventual enum). The on-disk
        enum was `{Auth, Network, Unknown}` — the value had never been added. Per [[LSN-014]] design docs
        describe TARGET shape and run ahead of code, so the sketch was intent, not fact.
rule:   in a briefing, quote design for INTENT and disk for FACT. Any assertion that a concrete symbol
        already exists must be grep-verified against the actual header/source before the briefing ships —
        never carry a symbol claim straight from design.md into an instruction to "not re-add" it.
check:  before sending a briefing, for each "already exists / already has / already wired" claim, run one
        grep against the real file (e.g. `grep -n 'MfaRequired' src/Cloud/IGarminAuthClient.h`); if absent,
        reword to "add it" or drop the claim. Corollary of [[LSN-014]] at the delegation layer.
origin: REQ-003 Slice A (2026-07-18). Harmless in effect — the builder grepped, flagged the discrepancy
        (D-R003-01), and correctly left the enum untouched (MFA routes via the dedicated `mfaRequired(QUuid)`
        signal, not a failure kind) — but the false claim was avoidable with one grep at brief time.
        miss:1 credited: the inaccurate claim shipped in the briefing and was caught downstream by the
        Verification Gate / builder diligence, not prevented at source.
history:2026-07-18: captured at advisory, first occurrence, from the REQ-003 MFA seam build. scope:portable
        — applies to any delegated briefing in any project. Directly extends [[LSN-014]] (design = intent).

### migrated-from-index-row 2026-09-06 (compaction; verbatim, not re-worded)
a builder briefing that asserts an existing code symbol ("enum X already has value Y", "field Z exists") MUST be grep-verified against the actual header on disk before it ships — design.md/DES ledgers describe TARGET shape and run AHEAD of code ([[LSN-014]]), so a symbol drawn from the design sketch may not exist yet. Corollary of LSN-014 at the delegation layer: quote design for INTENT, quote disk for FACT. (REQ-003 Slice A: the briefing claimed `GarminAuthFailure::MfaRequired` "ALREADY exists" from the DES-003a sketch; the on-disk enum was `{Auth,Network,Unknown}`. Harmless — the builder grepped, flagged it (D-R003-01), and correctly left the enum alone — but the false claim was avoidable with one grep at brief time.)

## LSN-020
sig:    test / real-bridge-uncovered-behind-fake / adapter-op-coverage
level:  guard  scope:portable   since:P2.2(2026-07-18, A3-R003-01)   recur:1   saves:0   miss:1
tags:   op:test, type:real-bridge-uncovered, scope:portable
trigger:a slice adds a new OP to an adapter seam that has TWO implementations — a Fake (for fast contract
        tests) and a real embedded-interpreter/FFI bridge (CPython marshalling, refcounting, dict-key parsing).
mistake:REQ-003 added `submitMfa` + a dict-sentinel MFA branch to `authenticate`. The C++ FakePyAdapter tests
        (T-029..031) and the wizard tests all passed, but PyEmbeddedAdapter.cpp's REAL bridge — the
        `mfa_required` dict-key detection and the ~65-line submitMfa with manual Py_INCREF/Py_XDECREF of
        m_client across the two-call flow — got ZERO tests: the `garmin-py` pystub had no MFA scenario and
        testGarminConnectPyAdapter.cpp never mentioned mfa, even though login/download/load_tokens each had a
        dedicated pystub scenario. A refcount bug, wrong dict key, or method-name typo would ship undetected.
rule:   for every new adapter-seam op, the real-bridge fixture (the pystub scenario + its C++ test under the
        `garmin-py`/real label) MUST gain a scenario in the SAME slice — the Fake test proves contract SHAPE,
        not the production marshalling. Parity with sibling ops is the bar.
check:  after adding an adapter op, `grep -i <opname>` BOTH the real-bridge pystub and its C++ test; empty =
        untested production code = blocking. Verify the new op has a pystub scenario matching the sibling ops'
        pattern in the same file.
origin: A3-R003 (2026-07-18) rated this blocking. miss:1: the untested bridge shipped through the builder's
        GREEN + the orchestrator Verification Gate (which re-ran the Fake-backed suites, not the real bridge)
        and was caught only by the adversary's coverage audit — which is why A3 exists alongside the gate.
history:2026-07-18: captured at guard, first occurrence. scope:portable — any Fake+real dual-impl seam
        (FFI, embedded interpreter, mocked network client) has this exact blind spot.

### migrated-from-index-row 2026-09-06 (compaction; verbatim, not re-worded)
when a slice adds an OP to an embedded-interpreter/FFI adapter seam (IGarminPyAdapter-style: a C++ interface with a Fake impl AND a real CPython/marshalling impl), the FakeXxx tests prove only the C++ contract SHAPE — they never touch the production bridge (dict-key detection, Py_INCREF/XDECREF refcounting, method-name strings). The real-bridge test fixture (here the `garmin-py` pystub scenario + its C++ test) MUST gain a scenario for the new op in the SAME slice, exactly like every sibling op already has. Check: for each new adapter op, grep the real-bridge pystub + its test for the op name; empty = untested production code. (A3-R003-01, blocking: PyEmbeddedAdapter's dict-sentinel MFA detection + ~65-line submitMfa refcounting shipped with ZERO garmin-py coverage while login/download/load_tokens all had pystub scenarios.)

## LSN-021
sig:    design / wizard-page-state-not-reset-on-reentry / back-navigation-leak
level:  guard  scope:portable   since:P2.2(2026-07-18, A3-R003-05)   recur:1   saves:0   miss:1
tags:   op:design, op:test, type:ui-state-leak, scope:portable
trigger:a reusable multi-step UI page (QWizardPage or equivalent) holds an async state machine
        (Idle→InFlight→Success/Error/terminal) as member state, and the container allows Back/re-entry.
mistake:GarminCredentialsPage/GarminMfaPage never override initializePage()/cleanupPage(), so m_state,
        m_attempts, m_pendingId and the message label persist across Back within one wizard instance. Concrete
        leak: creds A → mfaRequired → page 22 → 1 wrong OTP (attempts=1) → Back to page 21, whose m_state is
        still MfaRequired, so isComplete()==true and validatePage() returns true on its FIRST line before ever
        re-reading the (now-edited) email/password → the correction is silently discarded and routing returns
        to the SAME page-22 instance with stale attempts/message.
rule:   any reusable page with a member state machine MUST reset that state on re-entry (override
        initializePage()/cleanupPage()), OR carry an explicit test asserting Back-then-retry is an intentional
        no-op. A validatePage()/isComplete() that early-returns on a cached terminal state without re-reading
        inputs is the smell.
check:  for each page class with a `State`/`m_state` member + a validatePage()/isComplete() early-return on a
        terminal value, grep for an `initializePage`/`cleanupPage` override; absent both that AND a
        back-navigation test = state leak.
origin: A3-R003-05 (2026-07-18) — found by code-trace; no existing test drove a Back-then-Next sequence.
        miss:1: the leak shipped in Slice B, caught by the adversary not the builder/gate.
history:2026-07-18: captured at guard, first occurrence. scope:portable — applies to any wizard/stepper UI
        with reusable stateful pages in any Qt (or analogous) project.

### migrated-from-index-row 2026-09-06 (compaction; verbatim, not re-worded)
a QWizardPage (or any reusable multi-step UI page) that holds an async state machine (Idle→InFlight→Success/Error/terminal) MUST override initializePage()/cleanupPage() to reset that state on re-entry, OR carry an explicit test proving Back-then-retry is intentionally a no-op. Absent both, Back-navigation leaves a terminal state latched: a `validatePage()` that early-returns true on a cached Success/terminal state skips re-reading edited fields and silently discards the user's correction. Check: any page with a member state enum + validatePage() early-return-on-terminal needs an initializePage reset or a documented no-op test. (A3-R003-05: after MFA, Back to the credentials page kept `MfaRequired`, so an edited email/password was discarded with no re-auth.)

## LSN-022
sig:    test / tautological-assertion / always-true-guard
level:  advisory  scope:portable   since:P2.2(2026-07-18, A3-R003-07)   recur:1   saves:0   miss:1
tags:   op:test, type:tautological-assertion, scope:portable
trigger:writing or reviewing a test assertion whose boolean can never be false.
mistake:TEST-034's stale-reply guard check was `QVERIFY2(codeField(page)->text().isEmpty() == false || true,
        "stale failure changes nothing observable here")` — the `|| true` makes it unconditionally pass. It
        was the ONLY assertion meant to cover the stale-failure branch of onAuthFailed, so mutant M1 (dropping
        the `id != m_pendingId` guard inside GarminMfaPage::onAuthFailed) survived the whole suite: the stale
        event corrupted the attempt counter, but no assertion inspected it.
rule:   an assertion that is structurally always-true (`X || true`, `or True`, `expect(true)`, a disjunction
        with a literal truth) verifies nothing and must be treated as a review-lint FAILURE — replace it with
        the real observable it was meant to check (here: the attempt counter is unchanged after a stale event).
check:  grep the changeset's test files for `\|\| true`, `\| \|true`, `or True`, and `QVERIFY2?\([^,]*\|\|`
        before merge; each hit is a dead assertion. Mechanizable as a pre-commit/CLV grep gate (promotion path
        if it recurs).
origin: A3-R003-07 (2026-07-18), tied to surviving mutant M1. miss:1: shipped in Slice B's T-034, caught by
        the adversary's mutation probe.
history:2026-07-18: captured at advisory, first occurrence. scope:portable — dead/tautological assertions are
        a universal test smell; candidate for mechanization (grep lint) if it recurs. Sibling of [[LSN-009]]
        (a bound too loose to fail) — both are "assertions that cannot distinguish pass from fail."

### migrated-from-index-row 2026-09-06 (compaction; verbatim, not re-worded)
a test assertion that is unconditionally true — `QVERIFY2(X || true, …)`, `assert x or True`, `expect(true)`, `QCOMPARE(a||1, …)` — verifies NOTHING and gives false coverage confidence; it is the classic reason a "covered" line still lets a mutant survive. Treat any `|| true` / `or True` / literal-true disjunction in an assertion as a review-lint failure (mechanizable as a grep gate). Check at test review: grep the changeset's test files for `\|\| true`, `or True`, `QVERIFY2?\([^,]*\|\|`. (A3-R003-07: TEST-034's `QVERIFY2(…isEmpty()==false || true, …)` never fails and is exactly why mutant M1 — dropping the stale-id guard in onAuthFailed — survived.)

## LSN-023
sig:    delegate / readonly-agent-git-blind / stale-status-snapshot
level:  guard  scope:portable   since:P2.2(2026-07-19, VAL-015)   recur:1   saves:0   miss:1
tags:   op:delegate, op:verify, type:readonly-agent-git-blind, scope:portable
trigger:dispatching a read-only agent (qgdw-validator/adversary/scout — Read/Glob/Grep, no Bash/git) for a
        task whose verdict depends on working-tree or commit state ("is REQ-x committed?", "what's dirty?",
        "is the commit path-scoped?"), i.e. any CLV run that gates a commit decision.
mistake:the final REQ-003 CLV briefing did not include a live `git status`; the validator has no git tools,
        so it fell back to the harness's session-START git-status snapshot — which is FROZEN at session start
        and predates every REQ-003 file the builders created this session. Seeing no Garmin paths in that
        stale list, it (correctly, given its evidence) raised a FAIL on the "uncommitted working tree" premise
        it could not corroborate. All 9 CONTENT checks passed; the FAIL was purely evidence-blocked. The
        orchestrator resolved it in seconds with `git status --porcelain`/`git log` (which it alone can run).
rule:   the orchestrator owns git-truth (it has the shell); read-only agents own content. When a dispatch's
        verdict turns on VCS state, paste a FRESH `git status --porcelain` + relevant `git log --oneline --
        <paths>` INTO the briefing. Never let a read-only agent infer commit/dirty state from the frozen
        session-start snapshot — it is stale by construction the moment any file changes.
check:  before dispatching a commit-gate/working-tree-dependent CLV: does the briefing contain a git-status
        block captured THIS turn? If not, run it and paste it. On return, treat any git-state finding from a
        git-less agent as advisory-until-the-orchestrator-confirms-with-real-tooling, never as a hard FAIL.
origin: VAL-015 (2026-07-19). miss:1: the briefing omission shipped and produced a false FAIL at the commit
        gate; caught+resolved by the orchestrator's direct git inspection, not prevented at brief time. The
        validator independently proposed the same rule ("CLV-git-truth") from its side.
history:2026-07-19: captured at guard, first occurrence. scope:portable — applies to any read-only delegated
        role whose judgement depends on live VCS state, in any project. Related: [[LSN-016]] (merge-lag vs
        real defect — another "separate the evidence artifact from the true finding" rule at the CLV gate).

---

### migrated-from-index-row 2026-09-06 (compaction; verbatim, not re-worded)
a read-only agent (validator/adversary/scout — Read/Glob/Grep only, NO git tools) whose verdict depends on working-tree/commit state CANNOT see live git; and the session-start git-status snapshot in the harness prompt is FROZEN (it predates this session's work). So when a CLV/commit-gate dispatch turns on "is X committed / what's dirty", the orchestrator MUST paste a FRESH `git status --porcelain` + relevant `git log --oneline -- <paths>` into the briefing — else the agent reasons off the stale snapshot and raises a false git-state FAIL. Orchestrator owns git-truth (it has the shell); agents own content. (VAL-015: the final REQ-003 CLV FAILed the commit-scope check purely because it couldn't corroborate "uncommitted" — all 9 content checks passed; live git confirmed the ledger. The validator's own "CLV-git-truth" candidate is the same insight from its side.)

## LSN-024
sig:    verify / consumer-of-deferred-contract / cross-slice
rule:   a slice that CONSUMES a cross-slice or cross-module contract (a field read from a persisted file, an
        env value, state another slice is responsible for WRITING) is not end-to-end-done until the PRODUCER
        side is verified present in PRODUCTION code. A unit suite that supplies the value through a test-only
        override (constructor override, mock, fixture, monkeypatch) proves the consumer's own logic but is
        structurally BLIND to a missing or deferred producer — it can be 100% green while the live path always
        no-ops.
check:  for each EXTERNAL input a slice reads (a file field, persisted state, another module's output), grep
        the production write path that PRODUCES it. If the only writer is a test/override/mock, mark it an
        end-to-end gap and either build the producer in-scope or record an explicit blocking-for-end-to-end
        finding — never let unit-green stand in for integrated. Same family as [[LSN-018]] (green unit targets
        don't prove the app links): unit-green != integrated, at both the link layer and the data-contract layer.
origin: A3-R008-01 (2026-07-19). miss:1: REQ-008 Slice C shipped a readdir that resolves the active account by
        reading `garmin_user_id` from tokens.json, and records via a `GarminTokenStore::save` that NO connect-flow
        code calls (the save-wiring was deferred at REQ-004/006; dump_tokens() also omits the field). All three
        slices were fully unit-tested (17/17 executables) via a ctor uid-override, which masked that live sync
        can never resolve a uid → readdir no-ops. Caught by the A3 adversary hypothesis (run inline when the
        subagent hit a session limit), not at build time.
history:2026-07-19: captured at guard, first occurrence. scope:portable — applies to any slice consuming a
        value another slice/module must persist or emit, in any project. Related: [[LSN-018]] (link-layer
        sibling), [[LSN-004]] (adapter-seam-before-impl — the producer/consumer seam should be defined up front).

---

### migrated-from-index-row 2026-09-06 (compaction; verbatim, not re-worded)
a slice that CONSUMES a cross-slice/cross-module contract (a field in a persisted file, an env value, state another slice must WRITE) is not end-to-end-done until the PRODUCER side is verified to exist in production — a unit suite that injects the value via a test-only override (ctor override, mock, fixture) proves the consumer's logic while being BLIND to a missing/deferred producer. Check: for each external input a slice READS (file field, persisted state, another module's output), grep the PRODUCTION path that writes it; if the only writer is a test/override, it is an end-to-end gap even at 100% green. Sibling of [[LSN-018]] (green units != linked app) — both say unit-green != integrated. (A3-R008-01: REQ-008 readdir reads garmin_user_id from tokens.json + a GarminTokenStore::save the connect flow never calls; every test used the ctor uid-override, so 17/17 green hid that live sync always no-ops.)

## LSN-025
sig:    design / idempotency-record-before-confirm / any dedup-cache
rule:   a persistent dedup/idempotency record (Tier-1 sidecar, processed-id set, cache key) must be written
        STRICTLY AFTER the consuming pipeline confirms terminal success — never merely after the input is
        staged/enqueued. When the confirming step is async and/or in another module, a record-before-confirm
        turns any downstream failure into a SILENT, PERMANENT, happy-path-untestable loss: the item is marked
        done and skipped forever with no error and no retry.
check:  for every record/markProcessed/addToImported call, trace whether the success it asserts is confirmed
        AT that point or still pending downstream. If pending: move the record into the confirmation callback,
        OR add a reconcile pass that drops records whose product cannot be found. A happy-path test that only
        feeds valid input does NOT exercise this — add a case where the downstream step fails after staging.
origin: A3-R008-F1 (2026-07-20, blocking). GarminConnect::readFile records imported-<uid>.json (GarminConnect.cpp
        :374/:392) before the async CloudService import (CloudService.cpp:1937 ride==NULL → silent return); a FIT
        that passes the shallow magic-sniff but fails the full parse is dropped from every future sync silently.
        Tier-2 RideCache does NOT backstop it (no file was ever written to catch).
history:2026-07-20: captured at guard, first occurrence. scope:portable — applies to any dedup/idempotency
        record whose confirming step can fail after the record is taken. Related: [[LSN-024]] (producer/consumer
        integration gaps), [[LSN-017]] (async-completion lifetime — the same "the real success is later/async"
        family).

### migrated-from-index-row 2026-09-06 (compaction; verbatim, not re-worded)
a persistent dedup/idempotency record (a Tier-1 sidecar, "already-processed" cache, imported-id set) must be written STRICTLY AFTER the consuming pipeline confirms terminal success — never merely after the input bytes are staged/enqueued. If the record lands before a downstream parse/import can fail (esp. when that step is async + in another module), a failure becomes a SILENT, PERMANENT, happy-path-untestable data loss: the item is marked done, skipped forever, no error, no retry. Check: for every `record/markProcessed/addToImported` call, trace whether the success it claims is actually confirmed at that point or still pending downstream; if pending, either move the record to the confirmation callback or add a reconcile pass that drops records whose product doesn't exist. (A3-R008-F1: GarminConnect::readFile records imported-<uid>.json before the async CloudService import; a magic-sniff-pass/full-parse-fail FIT is dropped from sync forever.)

## LSN-026
sig:    code / rmw-not-salvaging-on-torn / aggregate-file persistence
rule:   a read-modify-write persistence helper must SALVAGE surviving entries on a torn / permission-rejected
        precondition — reusing the SAME per-entry-tolerant parse its load path already has — instead of silently
        starting from an empty aggregate and clobbering prior good data. record() and recover() must not diverge.
check:  a recordX() that reads-then-writes an aggregate must handle a non-Ok load status the same tolerant way
        loadX() does; add a test exercising recordX() after a torn/rejected precondition.
origin: A3-R008-F2 (2026-07-20, non-blocking). GarminSidecarStore::recordImported (GarminSidecarStore.cpp:164-188)
        drops the whole imported map to {} when the existing file is Torn/Rejected, contradicting its own "merges"
        doc comment; mitigated to a wasted redownload (Tier-2 catches the dup), not data loss.
history:2026-07-20: captured at advisory, first occurrence. scope:portable. Related: [[LSN-025]] (same sidecar,
        the record-timing sibling), [[LSN-005]] (validate-on-read as well as write).

### migrated-from-index-row 2026-09-06 (compaction; verbatim, not re-worded)
a read-modify-write persistence helper must SALVAGE surviving entries on a torn/permission-rejected precondition — reusing the SAME per-entry-tolerant parse its own load path already has — rather than silently starting from an empty object and clobbering prior good data. "record" and "recover" must not diverge. Check: a recordX() that reads-then-writes an aggregate file must handle a non-Ok load status the same tolerant way loadX() does, and a test must exercise recordX() after a torn/rejected precondition. (A3-R008-F2: GarminSidecarStore::recordImported drops the whole map to {} when the existing file is Torn/Rejected, contradicting its "merges" doc; mitigated to wasted redownload by Tier-2.)

## LSN-027
sig:    design / qobject-method-name-hidden / any QObject subtree
rule:   before naming a NEW method (especially a virtual) on a QObject-derived base class, grep for collisions
        with QObject's own member names (disconnect, connect, sender, parent, event, deleteLater, ...). C++
        name-hiding silently hides ALL base overloads of that name for EVERY subclass in the hierarchy — a
        familiar `obj->disconnect(sig,slot)` becomes a compile error or wrong-overload bind project-wide, not
        just in the extended class.
check:  `grep -E 'virtual .*\b(disconnect|connect|sender|parent|event|deleteLater)\s*\('` on any new method
        added to a QObject subtree; if it collides, rename to a domain verb (e.g. disconnectService()).
origin: A3-R008-F4 (2026-07-20, informational). CloudService::disconnect() (CloudService.h:121) name-hides
        QObject::disconnect() for all ~15 CloudService subclasses; currently inert (no site relies on
        QObject::disconnect on a CloudService*), a standing footgun.
history:2026-07-20: captured at guard, first occurrence. scope:portable — applies to any framework base with
        well-known member names (Qt QObject, etc.). Related: [[LSN-004]] (get the seam/API shape right up front).

### migrated-from-index-row 2026-09-06 (compaction; verbatim, not re-worded)
before naming a NEW method (esp. a virtual) on a QObject-derived base class, grep for collisions with QObject's own member names (disconnect, connect, sender, parent, event, ...) — C++ name-hiding silently hides ALL base overloads of that name for EVERY subclass, not just the one being extended, turning a familiar call (`obj->disconnect(...)`) into a compile error or a wrong-overload bind across the whole hierarchy. Check: `grep -E 'virtual .* (disconnect|connect|sender|parent|event|deleteLater)\b'` on any new method added to a QObject subtree; rename to a domain verb (disconnectService) if it collides. (A3-R008-F4: CloudService::disconnect() name-hides QObject::disconnect() for all ~15 CloudService subclasses — currently inert, a standing footgun.)

## LSN-032
sig:    verify / checkout-revert-destroys-uncommitted-work / any-mutation
level:  guard scope:portable      since:DEC-020(2026-08-03)   recur:1   saves:0   miss:1
tags:   op:verify, op:mutation-test, type:work-loss
trigger:about to revert a temporary mutation (mutation testing, a spike, a bisect probe)
mistake:the orchestrator mutated `GarminConnect::accountStillConnected()` to `return true;` to prove
        the builder's fail-closed gate was load-bearing — a legitimate and successful verification,
        4 tests died as they should — then reverted with `git checkout -- src/Cloud/GarminConnect.cpp`.
        The builder's production change was UNCOMMITTED, so checkout reset the file to HEAD and
        destroyed the slice's whole production half. Unstaged content has no reflog; it was
        unrecoverable from git and had to be rebuilt by re-dispatching the builder.
rule:   `git checkout -- <file>` is a valid mutation-revert ONLY when the file is clean at HEAD.
        When the working tree carries uncommitted work on that file, snapshot first
        (`cp f f.orig`) and restore from the snapshot, verifying with `cmp`.
check:  before mutating, run `git status --porcelain <file>`; if it shows ` M` (or `??`),
        checkout is FORBIDDEN as the revert path — use a file copy. Corollary: the safety of a
        revert technique is a property of the CURRENT tree, not of the technique — the adversary's
        identical `git checkout --` one step earlier was safe only because src/Cloud was clean then.
origin: 2026-08-03, DEC-020 hardening slice verification. Recovered by resuming the builder agent
        from its transcript to re-apply the lost GarminConnect.cpp changes; the .h, GarminTokenStore
        and all test files were untouched and survived.
history:2026-08-03: captured at guard, first occurrence, miss:1 (cost: one rebuild round-trip).
        scope:portable — applies to any agent doing mutation testing in a dirty tree. Sibling:
        [[LSN-007]] (a tool mutating files under you voids prior evidence). See also [[LSN-029]],
        the lesson whose mutation discipline prompted this verification in the first place.

### migrated-from-index-row 2026-09-06 (compaction; verbatim, not re-worded)
NEVER revert a temporary verification mutation with `git checkout -- <file>` unless that file is CLEAN at HEAD — on a file carrying uncommitted work, checkout discards the real work along with the mutation, silently and unrecoverably (no reflog for unstaged content). ALWAYS `cp <file> <file>.orig` before mutating and restore from that copy (`mv`/`cmp` to prove byte-identity), or stash-and-pop deliberately. Check before mutating: `git status --porcelain <file>` — a ` M` means checkout is FORBIDDEN as the revert path. The adversary's earlier `git checkout --` was safe ONLY because src/Cloud was clean at HEAD then; one slice later the same command on the same directory destroyed the builder's production change. (2026-08-03, orchestrator's own error while verifying the DEC-020 fail-closed gate: mutated `accountStillConnected()` to `return true`, confirmed 4 tests died — a GOOD verification — then reverted with checkout and lost the whole GarminConnect.cpp slice; recovered by re-dispatching the builder from its transcript. Sibling: [[LSN-007]] — evidence is void once files mutate under you.)

## LSN-033
sig:    build / fail-closed-return-ignored-by-caller / any-early-return
level:  guard scope:portable      since:DEC-022(2026-08-03)   recur:1   saves:0   miss:1
tags:   op:create-file, op:build, op:review, type:hang, type:leak
trigger:about to add (or review) an early `return false` / refusal path in a function that an
        existing caller drives
mistake:DEC-020's fail-closed guard, and then REQ-017 Slice A's epoch gate, both refused by
        `return false` with no completion posted. But `CloudServiceSyncDialog::syncNext`/
        `downloadNext` DISCARD readFile's bool and advance only on the `readComplete` signal — so
        the refusal produced a HANG ("Downloading n of N" forever), not a refusal, plus a leaked
        `new QByteArray` per attempt that the caller had allocated for the never-arriving callback.
        The security goal was met (nothing downloaded) and every test was green, so it shipped in
        `f001c7d20` and was only caught two REQs later, by an aside in a report-only question.
rule:   a refusal is only a refusal if the CALLER can observe it. Before adding an early return,
        read every call site and establish how failure is detected there. In a completion-driven
        contract (caller waits on a signal/callback), the refusal path MUST post that completion —
        returning a status the caller never reads is a hang. Corollary: whoever allocates a buffer
        for a callback frees it in that callback, so a skipped callback is also a leak.
check:  `grep -rn '<fn>(' src/ | grep -v '<fn>('` — for each call site, confirm the return value is
        assigned/branched on. If any caller ignores it, the early return needs a completion/signal
        on the same path. Test it as "the loop ADVANCES", never merely "the function returned false".
origin: B-R017-06 (2026-08-03), found while a Slice-B builder answered a report-only feasibility
        question about an unrelated error channel; orchestrator confirmed on disk at
        CloudService.cpp:1413/1495 vs GarminConnect.cpp:452/460. Fixed by DEC-022.
history:2026-08-03: captured at guard, first occurrence. miss:1 — latent from `f001c7d20`
        (2026-08-03) through two REQs. Siblings: [[LSN-018]] (unit-green != integrated) and
        [[LSN-028]] (code-exists != criterion-satisfied); this is the caller-contract version.
        See also [[LSN-024]] (a consumer built against a contract nobody produces).

### migrated-from-index-row 2026-09-06 (compaction; verbatim, not re-worded)
a refusal is only a refusal if the CALLER can observe it. Before adding an early `return false`, read EVERY call site and establish how failure is detected there; in a completion-driven contract (the caller waits on a signal/callback rather than the return value) the refusal path MUST post that completion, or the "guard" is a HANG — and any buffer the caller allocated for the never-arriving callback also leaks. Check: `grep -rn '<fn>('` every call site and confirm the return value is assigned/branched on; test the refusal as "the loop ADVANCES", never merely "the function returned false". (B-R017-06, 2026-08-03: DEC-020's `accountStillConnected()` guard and then REQ-017's epoch gate both returned false with no completion, while `syncNext`/`downloadNext` (CloudService.cpp:1413/1495) discard the bool and wait for `readComplete` — the sync dialog hung at "Downloading n of N" and leaked one QByteArray per attempt. All tests green; shipped in `f001c7d20`; caught two REQs later only as an aside in a report-only question. Fixed by DEC-022.)

## LSN-028
sig:    verify / requirement-satisfied-as-side-effect / cross-REQ
level:  guard scope:portable      since:REQ-012(2026-08-02)   recur:1   saves:0   miss:1
tags:   op:verify, op:ledger-update, type:unaudited-criterion
trigger:a requirement whose mechanism was built as a SIDE-EFFECT of another requirement's slice — i.e. the
        code exists but no builder was ever briefed with THIS requirement's acceptance criterion
mistake:REQ-012's disconnect mechanism shipped inside REQ-008's DEC-019 trigger slice, and the ledger read as
        though REQ-012 was essentially done. A clause-by-clause audit of prd.md:84 found only 2 of 5 criterion
        clauses had any test: "reconnect must perform a full SSO", "prior-account sidecars are not consulted",
        and "same-account reconnect resumes history" were entirely uncovered, and a fifth ("deletes the token
        file BEFORE clearing in-memory state") turned out to have NO MECHANISM AT ALL — disconnectService()
        clears no in-memory state (B-R012-01). Incidental construction had been silently mistaken for coverage.
rule:   a requirement satisfied incidentally by another REQ's slice is NOT done until its OWN acceptance
        criterion is split into clauses and each clause is mapped to a covering test or an explicit,
        recorded residual. The building slice was briefed with a DIFFERENT criterion, so it optimized for a
        different goal — the overlap is coincidence, not coverage.
check:  before closing (or dispatching) any REQ whose code "already exists", quote the criterion verbatim,
        enumerate its clauses (split on sentence + "and"/";"), and for EACH clause name the test id that
        encodes it; any clause with no id is either a build item or a recorded finding — never an assumption.
        Corollary: a test that passes on first run against unchanged production must prove it CAN fail
        (targeted, reverted mutation) before it counts as coverage.
origin: REQ-012 (2026-08-02). The clause audit produced TEST-054/055/056 + finding B-R012-01. Every new test
        passed first-run — the builder's 4 reverted mutations (incl. M4, which killed only the "not consulted"
        assertion) are what turned "green" into evidence.
history:2026-08-02: captured at guard, first occurrence. miss:1 — the gap existed from `ff9cce966` (2026-07-20)
        until the REQ-012 audit two weeks later. Siblings: [[LSN-024]] and [[LSN-018]] (unit-green != integrated
        — this is the ledger-layer version: code-exists != criterion-satisfied). See also [[LSN-022]]
        (green-but-vacuous assertions) and [[LSN-014]] (one fact, one home).

### migrated-from-index-row 2026-09-06 (compaction; verbatim, not re-worded)
a requirement whose mechanism was built as a SIDE-EFFECT of another REQ's slice is NOT done until its OWN criterion is split into clauses and each clause maps to a covering test id or a RECORDED residual — the building slice was briefed with a different goal, so the overlap is coincidence, not coverage. Corollary: a test that passes first-run against unchanged production must prove it CAN fail (targeted, reverted mutation) before it counts. (REQ-012: disconnect shipped inside REQ-008's DEC-019 slice; only 2 of 5 prd.md:84 clauses had tests and a 5th had no mechanism at all — B-R012-01.)

## LSN-043
sig:    verify / configure-is-not-build / build-definition-change
level:  guard      since:2026-08-07   recur:1   saves:0   miss:1
tags:   op:verify, op:commit, type:incomplete-gate
trigger:about to accept ANY build-definition change (CMakeLists, qrc, source lists, toolchain flags)
rule:   run the gate to completion from a CLEAN extract — configure AND generate AND full build AND
        the test suite. "Configure succeeds" is NOT evidence a build works.
check:  did the verification actually LINK the product binary and run ctest, or did it stop at
        "Configuring done"? If it stopped, the gate has not run.
why:    ORCH-001 was recorded as two causes, both configure-stage. Fixing them made configure pass —
        and the build still died three more times: generated translations never ported from qmake
        (ORCH-005, fails at BUILD), 11 tracked sources absent from the CMake lists (ORCH-006, fails
        at LINK), and a C++ dialect divergence breaking a Qt overload (ORCH-007, fails at LINK).
        Each failure stage is invisible to the stage before it. Had the repair stopped at "configure
        works", the branch would have merged with a still-unbuildable master and the clean-checkout
        gate would have reported success.
history:2026-08-07: captured at guard. miss:1 — the original ORCH-001 investigation (2026-08-05) used a
        throwaway worktree but only configured it, so it recorded 2 of 5 causes and understated the
        defect for two days. Siblings: [[LSN-018]] and [[LSN-024]] (unit-green != integrated) — this is
        the build-layer version: configure-green != builds. See also [[LSN-022]].

### migrated-from-index-row 2026-09-06 (compaction; verbatim, not re-worded)
a build-definition fix is NOT verified until configure AND link AND the test suite have run from a CLEAN extract — configure-only passed while 3 of ORCH-001's 5 causes were still fatal; missing-source and dialect defects surface at LINK, generated-asset defects at BUILD, and neither is visible at configure

## LSN-044
sig:    id-alloc / unregistered-series / any-id
level:  guard      since:2026-08-07   recur:1   saves:0   miss:1
tags:   op:id-alloc, type:collision
trigger:about to write ANY id (REQ/DEC/DES/TEST/VAL/LSN/ORCH/finding) into a ledger or commit message
rule:   take every id from WIKI REGISTRIES. If the series is not listed there, REGISTER IT FIRST
        (range + next), then allocate. Never infer "the next one" from memory or from context.
check:  does WIKI REGISTRIES name this series and its `next`? If not, stop and add it.
why:    The ORCH-nnn finding series existed in findings.md but was never listed in REGISTRIES, so
        there was no `next` to take. Writing up ORCH-001's repair I guessed ORCH-002/003/004 for three
        NEW findings — all three were already taken by unrelated 2026-08-05 skill-reinstall findings.
        The collision reached a commit message before being caught, needing an amend. An id series
        that lives outside the registry will be guessed sooner or later; registering it is the fix.
history:2026-08-07: captured at guard; ORCH series registered in WIKI REGISTRIES the same action
        (next:ORCH-008), new findings re-allocated ORCH-005/006/007, commit message amended (tree
        unchanged, so the build evidence still held). miss:1. Sibling: [[LSN-002]] (cross-ledger id
        collision) — same family, different cause: LSN-002 is merging registered ranges, this is
        allocating from an UNregistered one. See also [[LSN-014]] (one fact, one home).


## LSN-048
tags:   op:design op:decision scope:portable
level:  guard
rule:   a stub written by a finding is a POINTER to a defect, not a survey of it (see body below for the full check/why/history)
body (verbatim, pre-existing cold content that was missing its '## ' heading):
        A stub written by a finding is a POINTER to a defect, not a survey of it. The cycle that
        found the bug walked one call path and wrote down what it saw there; nothing in that process
        enumerates the rest of the surface. So the DEC that decides the FIX SHAPE must re-derive the
        affected-site list from the code first — because option scoring is a function of surface size,
        and a wrong surface silently selects the wrong option.
        The modal/modeless distinction is the specific trap: `exec()` blocks and returns, so the
        object's exposure is roughly the one frame; `open()` returns immediately and the object lives
        on servicing slots, so its exposure is EVERY member function. A finding raised against a
        blocking frame will therefore systematically understate a modeless object's surface.
        CHECK (before scoring options in any DEC that repairs an object): grep every member function
        of that object for derefs of the pointer at issue, count them, and compare against the stub.
        If the counts differ, the stub is the lower bound and the DEC text says so explicitly.
        (2026-08-10, DEC-030/S-R021-02. REQ-021's stub named two `CloudServiceSyncDialog::start()`
        sites. The real surface was eleven: nine more `context->` derefs in `refreshClicked`
        (CloudService.cpp:1318/:1371/:1443), `syncNext` (:1743/:1788), `downloadNext` (:1857),
        `uploadNext` (:1985) and `saveRide` (:2064/:2088) — all live because `MainWindow::syncCloud`
        (MainWindow.cpp:2604-2606) calls `open()`, not `exec()`. Orchestrator spot-checked all nine.
        Effect on the decision: a per-site guard option closes ~18% of the surface, a structural
        reparent closes 100% for two lines — so the surface correction is what selected the option.
        Scored against the stub, the cheap option would have looked sufficient and REQ-021 would have
        closed leaving 82% of its own axis open, which is precisely the shape that produced
        A3-R019-F1 in the first place.)
        Sibling family: [[LSN-041]] scans for other OBJECTS carrying the bug; [[LSN-046]] scans for
        other POINTERS dereferenced in the same frame; this one scans for other FRAMES in the same
        object. All three are the same failure — declaring a boundary without measuring it.


### migrated-from-index-row 2026-09-06 (compaction; verbatim, not re-worded)
counters: recur:2 saves:1 miss:2
**REFINEMENT (2026-08-13, O-R027-01, recur:2 — the guard was HONOURED and the answer was still wrong): re-deriving the surface is not enough if you enumerate by the wrong PREDICATE.** Opening REQ-027, the orchestrator did exactly what this guard prescribes — went to the code, enumerated, and found a second axis the finding's stub had missed (the discarded `readFile` bool). It then bounded that axis by asking **"which services LACK a `readFile` override?"** (answer: `Withings` alone, and unreachable because base `readdir` returns empty) and recorded the hazard as DORMANT. The scout, researching the same question, asked the RIGHT predicate — **"which services can return `false` without emitting?"** — and the answer is *nearly all of them*, on their ordinary error paths: `LocalFileStore::readFile` alone has four such returns, one of which (`!file.exists()`) fires whenever a listed file is deleted before download. The hazard is LIVE today, not dormant. **The failure shape: enumerating by the ABSENCE of an override (a structural property) when the criterion is a BEHAVIOUR the present overrides also exhibit.** Check, now explicit: after naming the population, state the predicate as a sentence and ask "could a member that PASSES the structural test still exhibit the behaviour?" — if yes, the structural test is the wrong enumerator and you must read the implementations. Tell-tale: the population comes out suspiciously small (one service) for a defect in shared base-class code. SAVE side: the delegation hatch caught it — the scout's independent research corrected the orchestrator's own recorded finding before it reached a decision, which is the second time this wave a subordinate has falsified an orchestrator premise. See [[LSN-034]] (same disease on the briefing side) and [[LSN-023]]. | a REQ/finding stub enumerates the sites the DISCOVERING cycle happened to walk, never the full surface — so **re-derive the affected-site list at DEC time, from the code, before scoring options**, and treat the stub's list as a lower bound. The specific trap: an object opened with `open()` (modeless) rather than `exec()` (modal) OUTLIVES the function that created it and keeps running slots, so its exposed surface is every slot, not just the one frame the finding named. Check: for the object under repair, grep EVERY member function for derefs of the pointer at issue - not just the function the finding cited - and compare the count against the stub. Tell-tale: the stub names one function and the class has a dozen slots. (2026-08-10, S-R021-02/DEC-030: REQ-021's stub named 2 `start()` sites in `CloudServiceSyncDialog`; the scout found NINE more `context->` derefs across `refreshClicked`/`syncNext`/`downloadNext`/`uploadNext`/`saveRide`, all reachable because `MainWindow::syncCloud` calls `open()` not `exec()`. Orchestrator spot-checked all nine. This inverted the DEC: a per-site fix closes ~18% of the surface, which is why the structural option won. Had the options been scored against the stub's 2 sites, the cheap per-site fix would have looked sufficient and REQ-021 would have closed with 82% of the axis still open.) Sibling family: [[LSN-041]] (other OBJECTS with the bug) and [[LSN-046]] (other POINTERS in the same frame) - this is the third axis, other FRAMES in the same object.

### migrated-from-index-row 2026-09-06 (compaction; verbatim, not re-worded)
never cite or allocate an id from a series that WIKI REGISTRIES does not list — if the series is absent, register it FIRST and take `next` from there; the ORCH-nnn series was unregistered, so ORCH-002/003/004 were guessed for new findings and collided with three existing ones

## LSN-054
sig:    build / memory-safe-is-not-stop-when-asked / suspension-point-in-a-loop
level:  guard      since:2026-08-12   recur:1   saves:0   miss:1
tags:   op:build, op:cycle, type:memory-safe-is-not-stop-when-asked, scope:portable
trigger:about to accept a guard placed after a nested loop / `processEvents()` inside a loop body
rule:   a `self.isNull()`-class guard proves MEMORY SAFETY only. A branch that suspends and then
        keeps ITERATING must also read the abort/cancel flag, because the flag can have been set by
        the very event that suspension delivered.
check:  per suspension point, answer TWO questions separately — "can `this` be dead here?" (the
        null-bail) and "can the user have said stop here?" (a flag read). A branch answering only the
        first and then falling through to the next iteration is a defect with ASan silent.
why:    the two properties have different guards and different failure modes. A lifetime miss crashes
        loudly under a sanitizer; a stop-when-asked miss is SILENT and outward-facing — it completes a
        real transfer to a third party after the user cancelled, which no sanitizer will ever flag.
history:2026-08-12: captured at guard, first occurrence, miss:1. A3-R021b-F1 — `uploadNext`
        (CloudService.cpp:2314-2411) never reads `aborted` (all reads in the file are :2187/:2292/
        :2421). An abort during the parse-failure branch's `processEvents()` (:2378) cleared the
        `self.isNull()` bail at :2385, fell through, and uploaded the NEXT ride. The success path was
        safe only incidentally (it `return`s; re-entry checks :2421). 40 green ASan slots were silent.
        The wave that introduced the counted frames is the wave that made this branch's survival
        matter, so REQ-025 is where it should have been caught. Sibling: [[LSN-046]] is about WHICH
        POINTER a guard covers; this is about WHICH PROPERTY it covers. See also [[LSN-055]].

### migrated-from-index-row 2026-09-06 (compaction; verbatim, not re-worded)
a `self.isNull()`-class guard after a suspension point proves MEMORY SAFETY and nothing else — it does not prove the code STOPS when the user asked it to. Any branch that suspends (nested loop / `processEvents()`) and then KEEPS ITERATING rather than returning must additionally read the abort/cancel flag, because the flag can have been set by the very event the suspension delivered. Check: for each suspension point, ask two separate questions — "can `this` be dead here?" (answer: the null-bail) and "can the user have said stop here?" (answer: a flag read); a branch that answers only the first and then `continue`s is a defect even though ASan is silent. Tell-tale: a `for`/`while` body containing a suspension point where the only post-suspension check is a lifetime check. (2026-08-12, A3-R021b-F1: `CloudServiceSyncDialog::uploadNext` (CloudService.cpp:2314-2411) reads `aborted` NOWHERE — every read in the file is at :2187/:2292/:2421 — so an abort during its parse-failure branch's `processEvents()` (:2378) passed the `self.isNull()` bail at :2385, fell through, and UPLOADED the next ride to a third-party service after the user pressed Abort. The success path was safe only incidentally, because it `return`s and re-entry goes through `completedWrite`'s :2421 check. 40 green ASan slots were silent on it.) Sibling: [[LSN-046]] (a self-lifetime guard covers exactly ONE pointer) — that one is about WHICH pointer, this one is about WHICH PROPERTY.

## LSN-055
sig:    cycle / finding-reported-via-its-hardest-trigger / reachability
level:  guard      since:2026-08-12   recur:1   saves:1   miss:1
tags:   op:cycle, op:verify, type:finding-reported-via-its-hardest-trigger, scope:portable
trigger:about to disposition (or score the severity of) any agent-reported missing-guard finding
rule:   accept the DEFECT from the report, never the TRIGGER STORY. Enumerate every route that sets
        the condition, cheapest first, before arguing reachability.
check:  grep every ASSIGNMENT to the flag/state the finding turns on — not just the reads — and open
        each writer's definition. If any writer is a plain user-facing control, reachability is
        settled and the subtle route is a footnote.
why:    a finding argued through its subtlest mechanism turns disposition into a debate about that
        mechanism's reachability. If a one-click route exists, that debate is not merely wasted, it
        can talk a real blocker down to "latent".
history:2026-08-12: captured at guard, first occurrence, saves:1 miss:1. A3-R021b-F1 was framed
        entirely on `deferCloseIfBusy()` (CloudService.cpp:1471-1478), gated on
        `blockingCallDepth > 0`. The Verification Gate found `downloadClicked` (:1908) relabels the
        Download button to "Abort" (:1918) mid-batch and sets `aborted=true` at :1916 synchronously —
        no deferral, no depth condition, no teardown (→ O-R021-05). SAVE: caught before disposition.
        MISS: the adversary shipped the harder story as the only story. The suite calls
        `downloadClicked()` at four sites to START batches; nobody clicked it twice.
        Sibling of [[LSN-034]] — the same unverified-premise failure, but on the REPORT side, which
        makes falsifying it the orchestrator's job, not the agent's.

### migrated-from-index-row 2026-09-06 (compaction; verbatim, not re-worded)
when a report (adversary, scout, builder) establishes that a guard is MISSING, do not accept its trigger story as the complete one — enumerate every route that sets the condition, cheapest first, before scoring severity or reachability. A finding argued through the subtlest mechanism invites a disposition debate about that mechanism's reachability, when a one-click route may exist. Check: grep for every ASSIGNMENT to the flag/state the finding turns on (not just the reads) and open each writer's definition; if any writer is a plain user-facing control, the finding's reachability is settled and the subtle route is a footnote. (2026-08-12, A3-R021b-F1/O-R021-05: the adversary framed the whole finding around `deferCloseIfBusy()` (CloudService.cpp:1471-1478), which fires only when `blockingCallDepth > 0`. Spot-checking under the Verification Gate found `downloadClicked` (:1908) relabels the Download button to "Abort" (:1918) during a batch and sets `aborted=true` synchronously at :1916 — no deferral, no depth condition, no teardown. SAVE: the gate caught it before disposition, so severity was argued on the real route. The suite calls `downloadClicked()` at four places to START batches; nobody clicked it twice.) Sibling of [[LSN-034]] — same failure of unverified premises, but on the REPORT side rather than the briefing side, so the orchestrator is the one who must falsify it.

## LSN-056
sig:    build / shared-stub-blast-radius / inert-method-made-real
level:  advisory   since:2026-08-12   recur:1   saves:1   miss:0
tags:   op:build, op:verify, type:shared-stub-blast-radius, scope:portable
trigger:about to change a stub/fixture method from inert to doing real work
rule:   run EVERY target that compiles the stub, not just the one that motivated the change.
check:  grep the build files for all targets listing the stub source; run each; state the count.
why:    an inert method has no callers to break, so the change is invisible until some other
        target's path reaches it — and then it fails in a target nobody was looking at.
history:2026-08-12: captured at advisory, saves:1 miss:0 — recorded as a SAVE because the practice
        was followed correctly. `ImportSeamStubs.cpp`'s `RideItem::ride()` began really calling
        `RideFileFactory::openRideFile` when `open && !ride_`; the adversary enumerated the three
        targets sharing the file and EXECUTED all three (SyncDialogClose 40/40, Import 4/4,
        ReadFailedConsumer 4/4) rather than reasoning them safe, refuting its own hypothesis with
        evidence. Stays advisory until a real miss occurs.

### migrated-from-index-row 2026-09-06 (compaction; verbatim, not re-worded)
when a stub/fixture file is compiled into N test targets, changing a previously-INERT method to do real work must be verified by running all N targets, not just the one that motivated the change — an inert method has no callers to break, so the change is invisible until another target's path reaches it. Check: `grep` the build files for every target that lists the stub source, and run each; state the count in the report. (2026-08-12, A3-R021b: `ImportSeamStubs.cpp`'s `RideItem::ride()` became a real `RideFileFactory::openRideFile` call when `open && !ride_`. SAVE — the adversary enumerated the three targets sharing the file and executed all three (testGarminConnectSyncDialogClose 40/40, testGarminConnectImport 4/4, testGarminConnectReadFailedConsumer 4/4) rather than reasoning that they were unaffected, and REFUTED its own hypothesis with evidence.)

## LSN-057
sig:    build / callback-driven-loop-branch-returns-without-rearming / silent-stall
level:  guard      since:2026-08-12   recur:1   saves:1   miss:0
tags:   op:build, op:cycle, type:callback-driven-loop-branch-returns-without-rearming, scope:portable
trigger:about to add, review or audit a `return` in a loop that is re-driven by a completion callback
rule:   every branch must either initiate the async work that will re-drive the loop, or re-drive it
        explicitly. A bare `return` that arms nothing ends the batch SILENTLY.
check:  enumerate the function's re-entry points (`grep -n 'funcName()'`, read each caller), then walk
        every `return` in the body and name which re-entry point that path arms. A `return` arming
        none is a stall, not a stop.
why:    it is invisible to every kind of test that asks "did the wrong thing happen?" — nothing wrong
        happens. The batch simply stops, with the UI still showing in-progress state. It is also
        invisible to a sanitizer, and it looks locally CORRECT: the branch returns, which is exactly
        what the other branches do.
history:2026-08-12: captured at guard, saves:1 miss:0 — recorded as a SAVE because it was caught by a
        builder that had been asked a NARROWER question (is `uploadNext` the unique REQ-026 site?) and
        volunteered the answer's neighbour. B-R026-01: `syncNext`'s parse-failure branch
        (CloudService.cpp:2067-2071 → `return true` at :2075) satisfies REQ-026 — it does not
        over-transfer — but arms none of syncNext's four re-entry points (:2273/:2326/:2472/:1962),
        because a parse failure emits no readFile, no writeFile, no async work at all. A sync with one
        unparseable local file freezes on that row. Mirror image of [[LSN-054]] (a branch that keeps
        iterating when it should stop); both are only visible with the re-entry map in hand, which is
        why the check is written as an enumeration rather than an inspection.

### migrated-from-index-row 2026-09-06 (compaction; verbatim, not re-worded)
in a loop shaped "process ONE item, return, and let the completion callback re-drive me", every branch must either initiate the async work that will re-drive it, or explicitly re-drive itself. **A branch that just `return`s without initiating anything terminates the batch SILENTLY** — the UI keeps the half-finished state (a row label, a frozen progress bar) and nothing is ever scheduled again. This is the mirror image of [[LSN-054]]: there, a branch kept iterating when it should have stopped; here, a branch stops when it should have continued. Both look correct in isolation and both need the RE-ENTRY MAP to see. Check: enumerate the function's re-entry points (`grep -n 'funcName()'` and read each caller), then walk every `return` in its body and ask which of those re-entry points that path arms. Any `return` arming none is a stall. (2026-08-12, B-R026-01: `CloudServiceSyncDialog::syncNext`'s parse-failure branch (CloudService.cpp:2067-2071) correctly falls to `return true` at :2075 — REQ-026 is satisfied, it does not over-transfer — but `syncNext`'s only re-entry points are :2273/:2326/:2472 (`completedRead`/`failedRead`/`completedWrite`) plus the initial dispatch at :1962, and a parse failure emits NONE of them. So a sync whose local file fails to parse freezes on that row forever. SAVE: the builder found it while confirming `uploadNext` was the unique REQ-026 site, reported it instead of fixing it out of scope, and the orchestrator confirmed it by reading the re-entry map.)


## Cold entries migrated from bloated index rows by librarian Job-3 compaction, 2026-09-06
(content verbatim from the pre-compaction index row; sig/trigger/mistake/check
 fields not yet hand-split out — do that opportunistically, not required for
 correctness, since the full original text is already here and addressable.)

## LSN-016
level:  advisory
tags:   op:delegate type:clv-before-merge
counters: recur:1  saves:0 miss:0
rule:   when incremental CLV is dispatched BEFORE the builder's byproduct is merged, its missing-row / stale-findings FAILs are…
history:when incremental CLV is dispatched BEFORE the builder's byproduct is merged, its missing-row / stale-findings FAILs are self-inflicted MERGE-LAG, not code defects — the fix's TEST-id is absent from traceability, the WIKI registry `next` is unconsumed, and the fixed finding still reads "open." Either merge byproducts (TEST rows + registry bump + findings disposition) FIRST then validate, or brief the validator that those rows are pending-merge so it scopes them out. Never let a merge-lag FAIL be read (or recorded) as a substantive code FAIL — separate the two in the VAL line. (VAL-013, B-R007-01 fix.)

## LSN-017
level:  advisory scope:portable
tags:   op:test type:queued-self-post-captures-this
counters: recur:1  saves:0 miss:0
rule:   a queued/deferred self-post (QMetaObject::invokeMethod/singleShot(0,…)) whose lambda captures an object OTHER than the QObject…
history:a queued/deferred self-post (`QMetaObject::invokeMethod`/`singleShot(0,…)`) whose lambda captures an object OTHER than the QObject passed as the connection CONTEXT is use-after-free-unsafe: Qt auto-cancels a pending post only when its CONTEXT object is destroyed, so if the captured object can outlive-or-predecease independently it can dispatch into freed memory. FIX: make the context an owned QObject member whose lifetime == the captured object's (a bare `QObject` member needs no Q_OBJECT and preserves a non-Q_OBJECT class contract; declare it so it destroys before the base the lambda calls into). Add a test that destroys the capturing object FIRST (opposite of natural RAII/declaration order) with the post still pending and assert no UAF / post cancelled. Corollary: when the regression test lives in a lightweight NON-QObject stub, note in the test which real Qt mechanics (signal emit, AutoConnection resolution, `QEventLoop::quit`) the stub cannot exercise — stub-recorder coverage ≠ real-caller proof. (A3-R007-01/-02; fixed via `m_completionContext` owned member, DES-014 preserved.)

## LSN-018
level:  advisory scope:portable
tags:   op:build type:main-binary-link-gap
counters: recur:2  saves:0 miss:0
rule:   GREEN unit tests do NOT prove the application links: a unit-test target compiles its OWN curated source subset, so a new production .
history:GREEN unit tests do NOT prove the application links: a unit-test target compiles its OWN curated source subset, so a new production .cpp that calls into another module (e.g. GarminConnect::open()→GarminTokenStore::loadChecked→AtomicFile) can pass every test while the MAIN binary fails with `undefined reference` because that module's .cpp is absent from the app's source list. When a production TU adds a cross-module call, grep the MAIN binary's target_sources for EACH referenced module's .cpp (not just the test target's) before calling the feature done — and run one actual app-config/link build at the feature gate. (B-R007-03: src/CMakeLists.txt GC_WANT_GARMINCONNECT had GarminConnect.cpp but omitted GarminTokenStore.cpp + AtomicFile.cpp; latent across slices, flagged by two builders, masked by all-green ctest.)

## LSN-029
level:  guard scope:portable
tags:   op:test type:ordering-invariant-unobserved
counters: recur:1 saves:0 miss:1
rule:   when a DEC records an ORDERING invariant ("write A before B", "delete X before Y", "record only after confirm"), the test set…
history:when a DEC records an ORDERING invariant ("write A before B", "delete X before Y", "record only after confirm"), the test set must contain an assertion that OBSERVES the order — end-state assertions leave the invariant fully mutable, so the loudest-documented guarantees end up the least defended. Corollary: every production guard shaped `if (x.isEmpty()) return;` needs a mutation that removes it, or it will be refactored away unnoticed. Check: for each ordering clause in a DEC/DES, name the assertion that would fail if the order inverted; if none, it is uncovered. (A3-R012-F3/F4: MUT-1 [persistConnectSuccess writes active-account BEFORE tokens, inverting DEC-018's crash-safety rationale], MUT-N [clearAccount deletes in the wrong order] and MUT-C [drop recordImport's empty-uid guard] all SURVIVED 18/18. Sibling: [[LSN-025]] — the record-before-confirm ordering defect this project already paid for.)

## LSN-030
level:  guard scope:portable
tags:   op:code type:secret-deletion-misses-tmp-sibling
counters: recur:1 saves:0 miss:1
rule:   deleting a secret written through an atomic-write helper (tmp+fsync+rename) must ALSO remove the <path>.
history:deleting a secret written through an atomic-write helper (tmp+fsync+rename) must ALSO remove the `<path>.tmp` sibling — a crash between write and rename leaves the complete secret there, and "the file is absent" is not "the secret is gone". Check: for every atomic writer, the matching delete/clear path must sweep the same tmp name. (A3-R012-F2: GarminTokenStore::clearAccount removes tokens.json + active-account.json but never their .tmp siblings; MUT-B survived 18/18. The class was already known — TEST-049 asserts !exists(.tmp) after a successful WRITE — but the DELETE side was never extended.)

## LSN-031
level:  guard scope:portable
tags:   op:test type:untouched-means-bytes-only
counters: recur:1 saves:0 miss:1
rule:   an assertion that a file is "untouched"/"unchanged"/"preserved" via a BYTE compare alone tolerates truncate+rewrite-identical, a…
history:an assertion that a file is "untouched"/"unchanged"/"preserved" via a BYTE compare alone tolerates truncate+rewrite-identical, a permission widening, and an mtime bump — for a security-relevant file (tokens, sidecars, keys) assert the MODE (owner-only) and mtime-or-inode too, not just content. (A3-R012-F5: MUT-D2 rewrote every prior-account sidecar and chmod'd it 0644 with bytes preserved; TEST-055's headline "sit on disk untouched" PASSED.)

## LSN-034
level:  guard scope:portable
tags:   op:delegate type:unverified-premise-in-briefing
counters: **recur:12 saves:10 miss:8 — MECHANISM PROMOTION IS OVERDUE, AND THE 2026-08-23 PAIR SHOWS WHY THE CURRENT GUARD CANNOT GET THERE ON ITS OWN**
rule:   REFINEMENT 8 (2026-08-23, DEC-038 A3 — TWO instances in ONE day, one per channel, and the contrast is the lesson).
history:**REFINEMENT 8 (2026-08-23, DEC-038 A3 — TWO instances in ONE day, one per channel, and the contrast is the lesson).** ORCH-035: the orchestrator's A3 briefing swapped the two driver-tail labels (`:2789` is `downloadNext`, `:3402` is `uploadNext`, not the reverse). Caught, cost nothing — **a SAVE, and it was a save for a structural reason: the briefing had ordered the agent to verify every line number at the definition, so the agent traced by CONTENT and the wrong labels were inert on arrival.** A3-R038-F3: three stale citations in ONE paragraph of the DEC-034 comment block (`CloudService.h:726-728`, duplicated at `CloudService.cpp:3435`) — **a MISS, shipped into candidate source, found only because an adversary happened to be pointed at that file.** THE RULE THIS ADDS: the delegation hatch polices the BRIEFING channel and nothing else. A line-number claim written into a CODE COMMENT is never re-read by anyone who could falsify it, and comments drift by construction — every edit above them moves the target. So: **a citation in a comment must name a SYMBOL, not a line (`saveRide`'s `autoProcess` call, not `:2910`); if a line number is genuinely wanted, it belongs beside the symbol as a convenience, never as the sole identifier.** That is a rule a mechanism could check, which is the promotion path the previous four refinements kept gesturing at. | **REFINEMENT 7 (2026-08-15, A3-R027c-F3, recur:10 — and this one is a MISS, not a save: it SHIPPED into the artifact and was found by the next cycle, not by the delegation hatch).** The stale line number landed in a place no existing discipline covers: an **assertion MESSAGE STRING**. TEST-105/106 carry `"syncNext:2011"` and `"downloadNext:2225"` as diagnostic labels, copied from the pre-DEC-035 block comment rather than re-derived from the guards they test (`CloudService.cpp:2030` / `:2268`); `:2225` is not even in `downloadNext`, which begins at `:2231` — it is `rideCache->save()` in `syncNext`'s own tail. **Why this position is worse than a comment:** a stale comment is read while the reader is calm and can doubt it; a stale assertion string is read at the exact moment a test has gone red, when the reader is chasing a defect and takes the label as the one fact they need not verify. It sends them into the wrong function. Check, extending the guard to a new position: **every `file:NNN` a slice writes into a STRING LITERAL — assertion messages, log lines, error text, `QVERIFY2` explanations — is subject to the same re-derivation rule as a comment or a briefing, and is MORE urgent because it is consumed under failure conditions.** Grep the diff for `":[0-9]` before calling a slice done. **EXTENSION, same day (ORCH-020, found while fixing the above): the string literal was not the only place, and the count was not one but SIX.** Re-deriving every `file:NNN` in the DEC-035 comment blocks found that `:2507`, `:2088`, `:2151`, `:2372`, `:2432` and `:2626` ALL failed to point at an abort read — they landed on a `self.isNull()` guard, an unrelated branch, DEC-030 prose, a `QByteArray` declaration and TEST-097 narrative. Two more (`:2214`, `:2277`) were stale before the session began. **None was caught by the builder, the Verification Gate, or a fresh adversary** — they are invisible to every gate this project runs, because nothing executes a comment. **The decisive detail: they were plausibly CORRECT when written, and were invalidated by the very wave that wrote them** — REQ-027 and DEC-035 each inserted code above them. So this is not a care failure that more care fixes; a citation into a file under active edit has a shelf life measured in commits. **Therefore the rule changes from "verify it" to "don't write it": cite the SYMBOL (`completedRead`, "the parse-failure branch"), and spend a line number only where the number IS the payload — an assertion message — re-deriving it LAST, after every other edit in the changeset has settled.** Quote the old numbers in place when you remove them, so the next reader knows they moved rather than guessing. Corollary nobody enjoys: any edit that inserts lines silently ages every downstream citation in every OTHER file too — this one aged ~30 lines' worth across another wave's fixture (ORCH-017 v), which is a cost of editing that no gate prices in. Same root as every refinement below: a line number copied rather than opened. | **REFINEMENT 6 (2026-08-15, DEC-035 briefing, recur:9 — and this one would have SHIPPED a silently-ineffective fix).** The briefing pasted a literal code snippet to add at BOTH new guard sites: `if (aborted == true) { curr->setText(7, tr("Aborted")); return true; }`. **Column 7 is wrong at `downloadNext`.** The download list has SIX columns with Status at **5** (`CloudService.cpp:1104-1106`); the sync list has EIGHT with Status at **7** (`:1172`); production itself already discriminates with `int col = sync ? 7 : 5` at `:2330`/`:2462`. `setText(7, …)` on the download list writes to a column the view does not display — the row would simply not be labelled, and no test that asserts only the read-count would ever notice. The builder caught it because the SAME briefing also said "confirm `curr` and the label column index are correct at BOTH sites by reading them", and it followed the verify-first instruction over the literal snippet. **The generalisable rule: a briefing that pastes literal code is asserting the code is correct in EVERY context it names — and a snippet lifted from site A is an unverified premise at site B, exactly like a line number.** Check: before pasting a snippet into a multi-site briefing, diff the sites' assumptions (column indices, container identity, return contracts) and either verify the snippet at each or state it as a template to be adapted, not text to be copied. **Second-order lesson: the "verify it yourself" clause is what saved this, so never drop it as redundant when the briefing also supplies the answer — the two together are a checksum, and the snippet is the half more likely to be wrong.** | **REFINEMENT 5 (2026-08-15, ORCH-015 briefing, recur:8 — TWO instances in ONE dispatch, both caught, neither costing anything).** (i) The briefing asked the builder for a test case of the shape `DEC-030 = ACCEPTED`. **"ACCEPTED" is not in `STATUS_TOKENS`** (the vocabulary is GREEN/CLOSED/DEFERRED/deferred/drafted/in build/`_pending_`/`_uncommitted_`, `ledger_drift_lint.py:81-90`), and an existing case (`DEC-031 ACCEPTED (Option B …)` → exit 0) depends on it NOT being there; building the case as briefed would have required weakening the vocabulary to satisfy a premise that was simply wrong. The builder flagged it in NOTES instead of complying — the report contract working exactly as designed. **A vocabulary is a premise too**: this project's status words are a closed list, and citing one from memory is the same error as citing a line number from memory. (ii) The SAME dispatch: minutes after briefing the builder to quote `STATE.md:580` and `:591` verbatim, the orchestrator edited STATE.md itself (recording the dispatch) and shifted both lines — and incidentally created a FOURTH instance of the very false positive under repair. **A position in a file you are still writing to is invalid the moment you write.** Caught and corrected by an immediate follow-up message, and the correction's own mapping was off by one slot, which the builder also caught and reported. New sub-check, cheap and absolute: **when briefing about a LIVE artifact, pass the TEXT, never the coordinates** — and if you must pass coordinates, do not touch that file until the agent returns. | **REFINEMENT 4 (2026-08-13, REQ-027 briefing, recur:6 — B-R027-01; the worst instance yet, because it recurred ONE STEP AFTER the orchestrator wrote a refinement about this exact disease).** The briefing asserted "every real implementation returns false only on paths that armed nothing" and cited ten services, including `GarminConnect.cpp:451-452`. **False.** `GarminConnect::readFile` has EIGHT `return false` sites (452, 474, 493, 519, 540, 557, 572, 588) and **SEVEN of them post a completion on the line immediately above** (:472→474, :492→493, :516→519, :537→540, :554→557, :569→572, :587→588), both posters being `Qt::QueuedConnection` so the emission lands AFTER the false return. Only :452 is silent — **the single site the orchestrator sampled, then generalised from.** Building to the briefed spec would have double-driven the loop on GarminConnect: the new branch advancing the bar synchronously, then the queued completion advancing it again, labelling `child(listindex-1)` (a different row by then) and re-driving the loop — the precise double-drive the SAME briefing correctly forbade for `writeFile`, and a regression of DEC-022/023. **The compounding failure is the orchestrator's, twice over:** (i) the claim was inherited from the scout's F1 and carried into a briefing after spot-checking only `LocalFileStore` — the Verification Gate's evidence check was run on ONE of ten cited locations and the verdict generalised; (ii) it is the identical wrong-predicate shape recorded minutes earlier in [[LSN-048]]'s own refinement ("enumerating by a structural property when the criterion is a behaviour"), which proves that *writing a lesson does not install it* — only mechanism does. Check, unchanged but now non-negotiable: **a claim of the form "every/all/none of X do Y" may not enter a briefing until every member of X has been opened**, and a per-member grep whose output you did not read line by line is not an opening. SAVE side: the builder refused to build, cited the falsifying command, and shipped zero code — six for six caught by the hatch, and this one would have been a silent production regression across ~11 integrations rather than a test failure. **Six recurrences, six catches, zero escapes, and the guard has now failed while its own author was actively thinking about it: the lint that extracts every `file:NNN` and every universal quantifier from a briefing and re-reads them is the only remaining move.** | **REFINEMENT 3 (2026-08-12, REQ-026 briefing, recur:5 — O-R021-06): the shape recurs even when the orchestrator DID open the file.** The briefing said `downloadClicked` "relabels the Download button to 'Abort' at :1918"; :1918 is the `return;` of the abort branch and the relabel is at :1924. The orchestrator had read :1900-1966 in full and still mis-attributed a line WITHIN the range it had opened, because it quoted from memory of the read rather than re-checking the specific statement. Check, tightened: opening the enclosing range is NOT enough — for each `Symbol/behaviour (file:NNN)` a briefing writes, the assertion at NNN must be the one being described, re-read at write time. Five for five caught by the delegation hatch; the argument for promoting this guard to MECHANISM (a lint that extracts every `file:NNN` claim from a briefing and re-reads it) is now stronger than the argument for a sixth restatement. | the sentence in a briefing or DEC that begins "this is what makes it safe" is a CLAIM ABOUT THE TREE and must be grepped BEFORE it is written, not after. **REFINEMENT (2026-08-07, DEC-029 briefing, recur:2): running *a* grep is not honouring this guard — the grep must be shaped so a false positive is IMPOSSIBLE.** A proximity grep (`grep -A3 'capabilities() const' *.h | grep -B1 Upload`) asserts CO-OCCURRENCE within a context window, not MEMBERSHIP, so it silently harvests neighbouring lines: it produced a briefing list naming Azum/Nolio/Withings/GarminConnect as Upload-capable when all four explicitly OMIT the bit, and named GarminConnect — the ledger's own subject, `GarminConnect.h:76 = Query|Download`, recorded as read-only by DEC-005 — as a service reachable through the upload path. Check: enumerate by MATCHING THE PREDICATE ITSELF, one line per candidate (`grep -n 'int capabilities() const' *.h` and read the returned bitmask), and separately account for the ones that inherit the base default by having NO override — a membership claim needs both halves. Tell-tale: if the count is right but you never saw each member's own line, you have co-occurrence, not membership. SAVE side of the same event: because the count WAS pasted for cheap falsification (the guard's original prescription), the scout re-derived it independently and corrected the composition before it reached a decision — the hatch working as designed, which is why this is saves:1 miss:2 and not a pure miss. **REFINEMENT (2026-08-10, REQ-021 Phase-1 briefing, recur:3): the failure has a single recurring SHAPE — quoting a line number for a SYMBOL without opening that symbol's DEFINITION.** The briefing said "`MainWindow::switchAthleteTab` (MainWindow.cpp:2165) hides tabs"; :2165 is a CALL SITE, the definition is at :2367, and it contains no compiled `hide()` at all (the only ones are inside `#if 0`) — the real mechanism is `tabStack->setCurrentIndex()` at :2389. The substance survived but the MECHANISM was wrong, and a gating probe was being asked to measure that mechanism. Check, mechanically: for every `Symbol (file:NNN)` a briefing writes, confirm NNN is where the symbol is DEFINED, not where it is called — `grep -n "^ClassName::method\|^void ClassName::method"` and use THAT line. A call site tells you the symbol is reachable; it tells you nothing about what the symbol does. SAVE side: the builder probed BOTH the described route and the real one and got the same verdict, so the gate held — three for three, the hatch has now caught every instance of this guard's failure, which is the argument for promoting it toward mechanism rather than relaxing it. See O-R021-01. **REFINEMENT 2 (2026-08-11, DEC-031 briefing, recur:4): the guard covers IDs, not just line numbers.** The same briefing made three errors of one family: it cited "DEC-017 (b)/(e) store ownership" when DEC-017 is *REQ-008 sidecar-persistence component shape* (the contract is **REQ-017**(b)/(e)); it named the functions enclosing two line numbers as "uploadNext/saveRide" when they are `syncNext`/`uploadNext` (`saveRide` makes no store calls at all); and it called the leak trigger a "routine single-athlete-tab close" when a SINGLE tab takes `closeWindow()` instead (MainWindow.cpp:2129-2130), so the population needs >=2 tabs or >=2 windows. Generalised check, now covering both shapes: **every identifier a briefing quotes — a line number, a symbol name, OR a DEC/REQ/TEST id — must be OPENED before it is written.** For an id: `grep -n "^## DEC-0NN" decisions.md` and read its title; a DEC id recalled from memory is exactly as unverified as a line number recalled from memory, and is more dangerous because it silently imports a whole decision's authority. For an enclosing symbol: never infer it from a line number — `grep -n "^ClassName::"` and bracket the range. SAVE side: the scout corrected all three in its own analysis before they reached a recommendation, and explicitly used the REQ id throughout instead. Four for four caught by the hatch. See O-R021-03.An orchestrator's premise is load-bearing in a way its instructions are not: the agent cannot see the codebase the orchestrator saw, so a false premise is inherited whole and shows up as a defect the agent gets blamed for. Check: before asserting "all N callers/subclasses do X", run the grep that enumerates them and paste the COUNT into the briefing so the agent can falsify it cheaply. (2026-08-04, DEC-022: the orchestrator wrote "the ~15 other CloudService subclasses pass an empty message — that is what makes this shared edit safe" without grepping. Every one of them passes `tr("Completed.")`, so the specified change would have rendered every sibling's parse failure as "Completed." across ~10 shipped integrations. The builder built it, RED-verified it, saw the contradiction, REMOVED it and stopped — the hatch working exactly as designed. Sibling: [[LSN-023]] — agents are blind to state the orchestrator can see, so the orchestrator owns the accuracy of what it pastes.)

## LSN-035
level:  guard scope:portable
tags:   op:byproduct type:index-table-falls-behind-its-own-prose
counters: recur:2 saves:1 miss:2
rule:   when a ledger has BOTH per-row prose and a summary INDEX table, the byproduct step updates the prose and silently skips the index…
history:when a ledger has BOTH per-row prose and a summary INDEX table, the byproduct step updates the prose and silently skips the index — nothing enforces index COMPLETENESS (the DEC-015 drift lint only enforces status-word PLACEMENT). Check, cheap and mechanical: `grep -o 'DEC-[0-9]\{3\}' traceability.md decisions.md | sort -u` vs the ids present in the `## DEC index` table; any id cited anywhere but absent from the index is a FAIL. Same for the DES index. Corollary: a hub file (WIKI REGISTRIES) must POINT, never RESTATE — restated detail is the thing that goes stale, and the deterministic lint cannot see prose it has no vocabulary for. (2026-08-05, VAL-017 check-6 FAIL: the `## DEC index` in traceability.md stopped at DEC-019 while DEC-020/021/022/023 were fully specified in decisions.md and actively cited by rows in the SAME file — DEC-020 had been missing since the previous REQ. Second occurrence of the class after [[LSN-008]]/[[LSN-015]]. Remediated by appending the four rows AND compacting WIKI REGISTRIES from 18.3k→10.1k chars so it points instead of restating — which also cleared 71 drift-lint findings to zero. Siblings: [[LSN-014]] one fact one home, [[LSN-009]].)

## LSN-036
level:  guard scope:portable
tags:   op:mechanism type:guard-false-positive-trains-bypass
counters: recur:4 saves:0 miss:4
rule:   when a deterministic guard denies a LEGITIMATE operation, the failure is the guard's, not the operator's — and the real damage is…
history:when a deterministic guard denies a LEGITIMATE operation, the failure is the guard's, not the operator's — and the real damage is that agents learn to route around it (the next genuine catch gets bypassed too). Treat "an agent invented a workaround for the hook" as a MECHANISM BUG with the same weight as a missed catch. Check: when a guard fires on something you believe is correct, do not just work around it — reproduce it in isolation, classify the false-positive, fix the mechanism, and prove BOTH directions with a behaviour matrix (genuine violations still denied AND the legitimate case now passing). A text-scanning guard must never treat a verb inside quotes or a heredoc body as an invocation: that is DATA, not a command. (2026-08-05, A3-R017-F5: `anti_duplication_guard.py` denied the adversary's LSN-032-mandated `cp` snapshot restore — it worked around it with `python3 shutil.copyfile`, which the regexes miss — and the orchestrator then hit two more false positives itself, a `grep` whose PATTERN held `"ln "` and a heredoc that merely MENTIONED `cp`. Fixed with `_is_snapshot_restore` + `strip_heredocs` + `_search_unquoted`, verified by a 9-case matrix, both copies md5-synced. Siblings: [[LSN-001]] the guard itself, [[LSN-032]] the lesson it was in conflict with.) **RECURRED 2026-08-05, ORCH-002 — miss:2.** The SAME false positive came back not from new code but from a **package reinstall**: the upstream `quality-gated-dev-workflow` skill update replaced `_is_snapshot_restore` with a narrower reimplementation (`src == dest + suf`, 3 suffixes) that re-denies the scratchpad snapshot shape `/tmp/…/foo.cpp.PRISTINE` while still allowing the sibling shape. Second-order lesson: **a mechanism fix proven locally is only as durable as its upstream — verify BOTH directions of a guard's behaviour matrix after every package/skill update, exactly as after a code change**, and prefer shipping the fix upstream over carrying a local patch that the next reinstall silently reverts. **CLOSED SAME DAY (2026-08-05, 21:18 package): upstream widened the match to basename+suffix, verified 9/9 in both directions (4 genuine clobbers still deny, 5 legitimate ops pass incl. the scratchpad restore) — fixed with ZERO local patching, and the interim sibling-only discipline is retired.** Both second-order rules were adopted upstream into `references/lessons-memory.md`: re-run the behaviour matrix after every skill update (a reinstall replaces mechanisms wholesale, so a fixed false positive can return with no new code), and keep a project-local mechanism's canonical source in the PROJECT, synced via `install_hook.py --extra-hook`, never as a patch to the skill's installer. **Standing post-update routine, therefore: re-run the matrix + re-apply `--extra-hook` syncs before trusting the guards again** — and as of the 21:45 package the matrix is BUNDLED, so the routine is concrete: `python3 .claude/skills/quality-gated-dev-workflow/scripts/guard_selftest.py .claude/hooks/anti_duplication_guard.py` (pass the INSTALLED hook's path — the default tests the skill-tree sibling, not the copy that actually runs). 46 cases incl. the scratchpad restore that caused this recurrence (case 39); 0 core failures on the 21:45 install. The same package also promoted "don't put project files in the skill tree" from advice to mechanism (vendor-territory deny/ask) — which immediately flagged our own DEC-015 lint, see ORCH-004. miss:2 stands — the counter records that the guard was too weak, not that it stayed so. Siblings: [[LSN-032]], [[LSN-001]]. **RECURRED 2026-09-05, [[ORCH-058]] — miss:3.** Third data point, and a THIRD distinct root cause under the same class: `_is_snapshot_restore()`'s match required the source basename to equal `dest-basename + suffix` EXACTLY, so a legitimate LSN-084/A3-mutation-proof snapshot named with mid-basename decoration for traceability (`CloudService.cpp.orch-mutation-orig`, not the bare `CloudService.cpp.orig`) was denied on its restore half — the orchestrator hit this directly while doing exactly the DEC-042/TEST-158 verification this project's own hardened rules require, and (correctly, per this lesson) reported the guard defect rather than routing around it with a raw `cp`/`shutil` bypass. **Fixed the same class of way as the 2026-08-05 recurrence — widen the match, then prove both directions — but this time as a LOCAL patch, not an upstream one:** `_is_snapshot_restore` now matches via `_BACKUP_WORD_RE` (a regex tolerating decoration between the destination's basename and the backup word) instead of exact string equality, applied identically to `.claude/hooks/anti_duplication_guard.py` and its skill-source mirror (sha256-confirmed identical). Re-verified this session: `python3 .claude/skills/quality-gated-dev-workflow/scripts/guard_selftest.py .claude/hooks/anti_duplication_guard.py` — 90/90 PASS, 0 core/gap failures, EXIT=0, same result against the mirror. **miss:3 stands and is NOT yet closed the way ORCH-002 was**, because unlike that recurrence this fix has NOT been shipped upstream into the `quality-gated-dev-workflow` package — it is a local patch only, exactly the shape this lesson's own second-order rule warns against carrying ("prefer shipping the fix upstream over carrying a local patch that the next reinstall silently reverts"). Until it lands upstream, a future package reinstall can silently reintroduce this exact denial with no new code, and per this lesson's own standing routine the re-verification command above (INSTALLED hook's path, not the skill-tree default) must be re-run — and the fix re-applied if it did not survive — after every future reinstall. Full finding: [[ORCH-058]]. **RECURRED 2026-09-12, [[ORCH-062]] — miss:4.** Fourth data point, and a FOURTH distinct root cause under the same class: this time the guard's new-file-creation heuristic (not the snapshot-restore heuristic ORCH-058/miss:3 fixed) misparsed a bare shell redirect (`2>`, `2>/dev/null`) or a `for ln in … N; do` loop's trailing numeric literal as a filename being created, denying routine read-only builder commands (`clang-format --dry-run`, guard-script invocations) during REQ-NF-i18n-001/T-208. The Inspector hit this directly while supervising the builder and, per this lesson's own rule, reproduced/classified each instance before approving rather than routing around it — 6 occurrences in ~50 minutes, all confirmed either read-only or an in-scope edit to already-owned files before approval. Not fixed here (no local patch attempted) — flagged upstream per the standing second-order rule. Full finding: [[ORCH-062]].

## LSN-037
level:  guard scope:portable
tags:   op:design type:guard-covers-one-of-several-freed-objects
counters: recur:2 saves:1 miss:1
rule:   a teardown almost never frees ONE thing, and the frames suspended below it include the CONSTRUCTOR.
history:a teardown almost never frees ONE thing, and the frames suspended below it include the CONSTRUCTOR. When you place a lifetime guard, enumerate EVERY object the teardown destroys AND every method of the class that can be on the stack in a nested event loop — **explicitly including the constructor and destructor**, not just post-construction member functions — because guarding only the object/frames the FINDING named leaves the others live, and the fix passes review while the same trigger still crashes. Check, before writing the guard: (0) list every method on the class that runs a nested event loop / blocking `exec()` — `grep` the ctor, dtor, and each slot; the ctor is the easy miss because the standard "fully-formed object stands down" pattern can't apply to a half-built object (its fix is either a self-`QPointer` bail after each blocking site OR two-phase init that keeps blocking work out of the ctor entirely); (i) list what is freed, (ii) which frames are suspended, (iii) for each frame, every member access AFTER the call returns — including implicit ones (an RAII destructor's back-pointer, a member ASSIGNMENT of the call's own result). Each is a separate write into freed memory needing its own guard. Corollary: a member write that consumes the blocking call's result (`member = store->call(...)`) executes BEFORE any guard placed after the call could run — land it in a local first. (2026-08-05/06, O-R025-01: A3-R017b-F1 named the STORE freed mid-`readFile`, DEC-025 was briefed as 'decline to delete the store'; reading the code showed parent teardown frees the DIALOG too — `BlockingCall::~BlockingCall` writes `dialog->blockingCallDepth` first as the loop unwinds, `refreshClicked` resumes into `workouts = store->readdir(...)`. Builder proved both by mutation. **RECURRED 2026-08-06, A3-R025-F1 — miss:1:** DEC-025's own enumeration STILL missed a frame — the CONSTRUCTOR (CloudService.cpp:709-974) runs `store->open()`/`QMessageBox::exec()`/`refreshClicked()` under one `BlockingCall` with no self-bail, a fourth live route to the identical UAF, so 'covered by construction' was false. Third cycle in this same family: DEC-024 close-init → DEC-025 parent-teardown+dialog → A3-R025 constructor. The rule now names the constructor explicitly so the next guard enumerates it up front.) Sibling: [[LSN-033]] — a refusal is only a refusal if the caller can observe it; this is the lifetime-shaped version of the same 'guard on the wrong layer' family (A3-R017b-F4).

## LSN-038
level:  advisory scope:portable
tags:   op:verify type:snapshot-suffix-unrecognized-by-guard
counters: recur:2 saves:0 miss:1
rule:   RECURRED 2026-09-04, orchestrator this time (not the prior DEC-026 case's author): snapshotted CloudService.
history:**RECURRED 2026-09-04**, orchestrator this time (not the prior DEC-026 case's author): snapshotted `CloudService.cpp.orig2` (a second concurrent snapshot, `.orig` already in use for an earlier one) for an independent RED-under-mutation re-verification of `TEST-076`; the `cp` restore was denied and briefly misread as an [[LSN-036]] mechanism bug before checking `lessons.md` found this lesson already covered it exactly. Recovered the correct way per this lesson's own prescription — a targeted `Edit` reversing the single known mutated line, `cmp`-verified byte-identical — so no harm done, but the miss stands: `lessons.md` should have been consulted BEFORE naming the snapshot (Principle 5 pre-flight), not after the denial. Second data point that a non-`.orig`/`.bak`/`.backup` suffix is an easy, repeatable mistake under time pressure; consider whether the guard's `_BACKUP_SUFFIXES` should also accept a numbered/timestamped variant of a recognized suffix (e.g. `.orig2`, `.orig.1`) rather than requiring the operator to always free up the bare `.orig` name — that would be a mechanism widening, not a bypass, and is worth a future two-directional matrix check if this recurs a third time. | an LSN-032 verification snapshot MUST use a suffix the anti-dup guard recognizes as a backup — `_BACKUP_SUFFIXES = (.orig, .bak, .backup)` — or the guard will (CORRECTLY) deny the `cp <snap> <original>` restore as a clobber, indistinguishable from a genuine overwrite. Use `cp file file.orig` (the canonical Verification-Gate form), not an ad-hoc name like `file.ORIG_ORCH`. This is NOT an [[LSN-036]] false-positive — the guard is working as designed (guard_selftest case 39 proves a proper `.orig` cross-dir scratchpad restore passes); the fault is the operator's suffix choice. If a restore is denied, FIRST check the snapshot's suffix against `_BACKUP_SUFFIXES` before suspecting the mechanism; and never route around the denial (a targeted Edit reversing your own known mutation, proven byte-identical by `cmp`, is a legitimate content-edit, not a bypass). (2026-08-06, DEC-026 verification: orchestrator snapshotted `CloudService.cpp.ORIG_ORCH`, the `cp` restore was denied, briefly misread as an LSN-036 recurrence; the bundled self-test passed 46/46 incl. the scratchpad case, isolating the cause to the non-canonical suffix. Sibling: [[LSN-032]] the snapshot mandate, [[LSN-036]] the real false-positive class this was mistaken for.)

## LSN-039
level:  guard scope:portable
tags:   op:verify type:mutation-survives-because-branch-unreachable
counters: recur:1 saves:0 miss:1
rule:   a guard/line whose mutation SURVIVES the suite is not proven dead or wrong — it may be UNREACHABLE by the fixture.
history:a guard/line whose mutation SURVIVES the suite is not proven dead or wrong — it may be UNREACHABLE by the fixture. Before concluding "dead code" (remove/accept) OR "fix covered", confirm the test can structurally REACH the mutated branch: scripted failure modes for error paths, non-empty triggering state for count-gated branches, and the hazard armed at EACH distinct nested-loop/suspension point (not just the first). Mutation testing answers "does the suite catch this?", NOT "is this code needed?" — a green mutation over an unreachable branch is a COVERAGE gap masquerading as a dead-code finding. Check: for each surviving guard mutation, name the fixture input that reaches its branch; if none exists, the finding is "untested", not "dead". Contrast a genuinely-dead guard ([[A3-R025-F2]]: no member access follows) with a reachable-but-unexercised one. (2026-08-06, A3-R026-F2: 4 of 5 `start()` self-bails survived neutering — but because `BlockingStore::open()` had no failure mode, `rideCache` was kept empty, and the teardown was armed only in `open()`; three structurally-distinct unreachable branches, not dead code. Sibling: [[LSN-032]] snapshot-mutate discipline, [[LSN-013]] one case per uncovered class.)

## LSN-040
level:  guard scope:portable
tags:   op:design type:qt-parent-deletes-stack-child
counters: recur:1 saves:0 miss:1
rule:   Qt's QObjectPrivate::deleteChildren() calls delete on EVERY child of a destroyed parent UNCONDITIONALLY — independent of the…
history:Qt's `QObjectPrivate::deleteChildren()` calls `delete` on EVERY child of a destroyed parent UNCONDITIONALLY — independent of the child's `WA_DeleteOnClose`, storage class, or any `self.isNull()`/`QPointer` guard INSIDE the child (the guard sees the death, cannot prevent it). So a QObject in AUTOMATIC (stack) storage parented to a widget that can be independently destroyed (a `WA_DeleteOnClose` window, a `deleteLater` target, or any to-be-deleted parent) is a bad-free (`free() on non-malloc'd`) PLUS a double-destruction when the C++ scope unwinds. Check: for every `Widget foo(parent, ...)` in automatic storage OR any `new`-then-manually-`delete`d QObject, confirm `parent` cannot be destroyed within the object's lifetime; if it can, heap-allocate + `WA_DeleteOnClose` (let Qt own it) or parent to something that outlives the scope. Corollary (test-comment-trust): a safety claim embedded in a comment — "X has no attribute Y and so was never affected" — must be checked against the actual Qt mechanism before it justifies SKIPPING test coverage; here that belief left the second production caller untested for the very bug class its DEC chain exists to close. (2026-08-06, A3-R026-F1: `MainWindow::syncCloud`'s STACK `CloudServiceSyncDialog` parented to the `WA_DeleteOnClose` MainWindow — a fifth route DEC-024/025/026 never covered; the prior [[A3-R017-F1]] disposition wrongly declared it "UNAFFECTED (no WA_DeleteOnClose)". Adversary proved the mechanism by a standalone ASan repro. Sibling: [[LSN-037]] enumerate every freed object/frame — this is the storage-class axis of the same family.)

## LSN-041
level:  guard scope:portable
tags:   op:design type:fix-scoped-to-one-instance-of-a-class-wide-bug
counters: recur:1 saves:1 miss:1
rule:   when a defect is a CLASS (a repeatable code pattern — here: an unguarded nested QEventLoop in a dialog constructed/parented under…
history:when a defect is a CLASS (a repeatable code pattern — here: an unguarded nested `QEventLoop` in a dialog constructed/parented under a `WA_DeleteOnClose` self-deleting ancestor), fixing the ONE instance a finding named does NOT close the class. Before declaring a bug class closed, GREP THE CONTAINING SUBSYSTEM for every sibling with the same shape and enumerate them as findings — the fix effort's own recipe becomes the search pattern. Check, before a "class closed" claim: run the structural grep (`grep -rn 'QEventLoop\|::exec()' <subsystem>` cross-referenced with dialog ctors + `WA_DeleteOnClose` parents) and account for EVERY hit as fixed / out-of-scope-tracked / dormant. A per-instance fix with no sibling scan ships a false "closed". (2026-08-06, A3-R027: four DECs (024/025/026/027) closed the `CloudServiceSyncDialog` UAF one route at a time, but a 60-second grep surfaced `CloudServiceUploadDialog` (F1, live/high-traffic), `AddAuth::doAuth` (F2, live) and `AddSettings::browseFolder` (F3, dormant) — the SAME bug, same files, never scoped. Had the sibling scan run at DEC-024, the class would have been mapped whole instead of discovered one adversary cycle at a time. Sibling: [[LSN-037]]/[[LSN-040]] enumerate-everything family — this is the cross-CLASS-instance axis.)

## LSN-042
level:  advisory scope:portable
tags:   op:test type:dormant-hazard-behind-unset-flag-is-still-a-defect
counters: recur:1 saves:0 miss:0
rule:   a memory-safety (or security) hazard that is unreachable today ONLY because a UI flag / config setting / feature toggle is…
history:a memory-safety (or security) hazard that is unreachable today ONLY because a UI flag / config setting / feature toggle is currently unset is a DEFECT TO RECORD, not a non-finding — the thing making it "safe" is unrelated to the bug and silently makes it live the moment any future code sets that flag. Record it (tracked/dormant) with the exact condition that would arm it. (2026-08-06, A3-R027-F3: `AddSettings::browseFolder`'s stack-dialog-under-self-deleting-ancestor UAF is inert only because no `CloudService` subclass sets `CloudService::Folder` in its settings map; a Dropbox-style folder service added tomorrow arms it. Sibling: [[LSN-041]].)

## LSN-045
level:  guard
tags:   op:decision type:dec-internally-inconsistent-about-its-own-target
counters: recur:1 saves:1 miss:1
rule:   a DEC that states the same code fact in two places must be SELF-CONSISTENT, and its alignment probe must be RUN ONCE against the…
history:a DEC that states the same code fact in two places must be SELF-CONSISTENT, and its alignment probe must be RUN ONCE against the intended end-state before the DEC ships. Distinct from [[LSN-034]]: that guard covers premises never grepped; THIS one covers a fact the author grepped CORRECTLY and then restated WRONGLY somewhere else in the same document — greping harder would not have caught it, re-reading the DEC as a whole would. Check: before dispatch, (1) grep the DEC for every function/file name it names as a change target and confirm they are the SAME target throughout, and (2) execute each alignment-probe line against the CURRENT tree, confirming it fails now in the way that proves it will pass after — a probe that cannot pass even in principle is a probe nobody ran. (2026-08-08, DEC-029: clause 3 said "`MainWindow::uploadCloud` becomes new + WA_DeleteOnClose + if(start())exec()", but uploadCloud does not construct the dialog at all — `CloudService::upload` does, a fact the SAME DEC cites correctly two paragraphs earlier. The probe `grep -c 'QPointer<CloudServiceUploadDialog>' # expect >= 4` was likewise unsatisfiable under the one-QPointer-per-function idiom that same DEC mandates. The builder applied the substance at the real site and disclosed both as CONTRACT-CONFLICTs instead of following the text — the report contract working as designed, and the only reason this cost nothing. See O-R019-01.)

## LSN-046
level:  guard scope:portable
tags:   op:build op:cycle type:self-lifetime-guard-does-not-cover-collaborators
counters: recur:1 saves:0 miss:1
rule:   when a fix closes a use-after-free by guarding an object's OWN lifetime (QPointer self-bail after each nested loop), that guard…
history:when a fix closes a use-after-free by guarding an object's OWN lifetime (`QPointer` self-bail after each nested loop), that guard covers exactly ONE pointer: `this`. Every OTHER pointer the resumed frame dereferences needs its own liveness proof, and "the dialog is a child of X" does NOT imply "everything the dialog touches is owned by X." Check: after each blocking call, LIST every pointer the following statements dereference, and for each one name the object that owns it and the event that destroys it — if any owner is NARROWER-lived than `this`'s Qt parent, that pointer needs its own guard (or the object must be reparented so lifetimes coincide). Tell-tale: a guard suite where every bail is `self.isNull()` and none mention a collaborator. (2026-08-08, A3-R019-F1/F2: five DECs (024,025,026,027,029) and four A3 cycles all asked "can the dialog die under us?" and none asked "can what the dialog POINTS AT die under us?" `CloudServiceUploadDialog`/`CloudServiceSyncDialog` are parented to MainWindow, but `context`/`item` belong to the AthleteTab — and `MainWindow::removeAthleteTab` deletes Context SYNCHRONOUSLY while MainWindow's own WA_DeleteOnClose deletion is DEFERRED, so the dialog reliably outlives its Context. The gap shipped to master in `ae5a7a8ab` under an explicit "class CLOSED" claim.) Sibling: [[LSN-041]] (sibling scans) — this is the same failure one axis over: scan for other OBJECTS with the bug, and also for other POINTERS in the same frame.

## LSN-047
level:  guard scope:portable
tags:   op:verify type:stub-cannot-prove-the-fault-it-is-credited-with
counters: recur:1 saves:1 miss:1
rule:   before writing "RED: dereferences <X> → ASan heap-use-after-free" in a test comment or a ledger row, OPEN the stub/mock that <X>…
history:before writing "RED: dereferences <X> → ASan heap-use-after-free" in a test comment or a ledger row, OPEN the stub/mock that <X> resolves to in that target and confirm its body actually dereferences `this` or its arguments. An inert `{}` / `Q_UNUSED`-only stub cannot fault, so a mutation that "proves" the guard may be dying for an ADJACENT reason (a field read one statement earlier) — the test still passes, the stated causal claim is false, and the specific sub-claim you thought you covered is untested. Check: grep the stub body; if it is empty, either give it a member touch or rewrite the claim to name the read that actually faults. (2026-08-08, A3-R019-F3: TEST-079 guard B was credited with proving `context->mainWindow->saveSilent()` safe; `ImportSeamStubs.cpp:383` is `{Q_UNUSED;Q_UNUSED;}`, as are `Context::metadataFlush` (:212) and `RideItem::notifyRideMetadataChanged` (:312). The mutation died on the freed-Context field reads at CloudService.cpp:444-445 instead — a real UAF, just not the advertised one.)

## LSN-049
level:  guard scope:portable
tags:   op:build op:verify type:fix-newly-reads-a-never-initialised-member
counters: recur:1 saves:1 miss:0
rule:   when a fix makes code READ a member/field it did not read before, that member's initialisation is now load-bearing and must be…
history:when a fix makes code READ a member/field it did not read before, that member's initialisation is now load-bearing and must be checked on EVERY construction path — a field nothing read could stay uninitialised for years without symptom, and the fix is what arms it. An uninitialised pointer is NOT null: handing it to an API is undefined behaviour, which can be strictly WORSE than the bug being fixed. Check, before shipping any fix that introduces a new read: `grep` the field's declaration, `grep` every assignment to it, `grep` every constructor of the owning class, and confirm the assignment DOMINATES the new read on all paths — paying special attention to two-phase construction, where an object is published (emitted, registered, passed to a callback) between its constructor and the later assignment. (2026-08-11, O-R021-02/DEC-030: REQ-021 parented both cloud dialogs to `context->tab`. `Context::Context` (Context.cpp:141) nulls `ride`, `workout`, `videosync` and more but never `tab`, which is assigned only by `AthleteTab`'s ctor (AthleteTab.cpp:35). `MainWindow::openAthleteTab` (MainWindow.cpp:2038) builds a Context, sets `athlete = NULL` and EMITS `openingAthlete` with it, while the AthleteTab is not created until `loadCompleted` — a real publish-before-initialise window. Nothing had read `tab` there before, so it had never mattered. Found by the builder in its own work and disclosed unprompted; fixed with one line.) Sibling: [[LSN-046]] — that one asks whether a pointer's OWNER outlives the read; this one asks whether the pointer was ever SET.

## LSN-050
level:  guard scope:portable
tags:   op:test type:apparatus-cannot-observe-the-thing-asserted
counters: recur:2 saves:1 miss:1
rule:   a test that asserts something is BLOCKED, HIDDEN, or otherwise prevented must first demonstrate that its apparatus can observe…
history:a test that asserts something is BLOCKED, HIDDEN, or otherwise prevented must first demonstrate that its apparatus can observe the UNBLOCKED case — otherwise a passing assertion is indistinguishable from a test harness that cannot see the phenomenon at all. Ship the positive control IN THE SAME SLOT and assert both halves. Check: for every "X does not happen" assertion, add the paired run where X DOES happen and assert it is observed; if you cannot make the control fire, your apparatus is wrong, not the code. (2026-08-11, REQ-021 TEST-085(b): `QTest::mouseClick(QWidget*)` is BLIND to modal blocking on Qt 6.8.2 — measured, a bare modal QDialog let the click through — so the first version of the modality test was a false alarm built on an apparatus that could not see modality at all. The `QWindow` overload goes through `QWindowSystemInterface`, where Qt consults `blockedByModalWindow`. SAVE side, same wave: TEST-081 shipped its control from the start — an ordinary non-window child of the same parent DID go invisible — which is the only reason its "child windows stay visible" verdict was not vacuous, and that verdict GATED the whole DEC-030 option choice.) Any future modality test in this tree must use the QWindow overload. Sibling: [[LSN-047]] (an inert stub cannot prove the fault it is credited with) — same family: the test looked green because nothing was being measured.

## LSN-051
level:  guard scope:portable
tags:   op:design op:cycle type:reparent-changes-the-exposure-set-not-just-the-lifetime
counters: recur:1 saves:0 miss:1
rule:   a fix that changes an object's OWNER/PARENT to a shorter-lived one does not merely close the lifetime bug it targeted — it…
history:a fix that changes an object's OWNER/PARENT to a shorter-lived one does not merely close the lifetime bug it targeted — it CHANGES WHICH FRAMES CAN BE DESTROYED UNDER THEMSELVES. The new exposure set is "every frame of that object that can be SUSPENDED" (nested event loop OR `processEvents`), which is a DIFFERENT and usually LARGER set than "every frame that dereferences the collaborator you were fixing". Self-guards previously proven unnecessary against the OLD parent must be RE-PROVEN against the new one. Check, before accepting any reparent/ownership-transfer fix: enumerate every suspension point in the whole class (not just the repaired function), and for each ask "can the NEW owner die here, and what does the next statement touch?" — including `this`. (2026-08-11, A3-R021-F1/F2: DEC-030 reparented two cloud dialogs from MainWindow to AthleteTab to close a COLLABORATOR (`context`) UAF. It did close it — and simultaneously opened a `this` UAF at four `processEvents()`→`syncNext()/downloadNext()` sites in the modeless completion slots, because MainWindow died only via a posted DeferredDelete that `processEvents` does not deliver, whereas `delete tab` is SYNCHRONOUS from inside ordinary event delivery, which it does. A fifth hole (F2) was EXECUTED as a real ASan heap-use-after-free: `start()` guarded `ctx` AFTER calling `refreshClicked()`, whose own guard was self-only. The scan that produced the REQ enumerated `context->` derefs; the reparent's risk lived in `this` derefs.) Sibling family: [[LSN-041]] other OBJECTS, [[LSN-046]] other POINTERS in a frame, [[LSN-048]] other FRAMES in an object — this is the fourth axis: the SAME frames under a NEW owner.

## LSN-052
level:  guard scope:portable
tags:   op:decision type:decision-premise-silently-invalidated-by-a-later-fix
counters: recur:1 saves:0 miss:1
rule:   when a change moves the TRIGGER of a previously-decided behaviour from a RARE context to a ROUTINE one, the earlier DEC whose…
history:when a change moves the TRIGGER of a previously-decided behaviour from a RARE context to a ROUTINE one, the earlier DEC whose rationale explicitly cited the rare context must be REOPENED by name in the new DEC — never inherited by silence, and never repaired by editing the old comment. A decision is a claim about a context; change the context and the claim is untested, not merely stale. Check: when a DEC changes an ownership, lifetime, threading or triggering relationship, grep the prior DECs governing that code for the words describing the OLD relationship and list each as reopened / still-valid / superseded. (2026-08-11, A3-R021-F4: DEC-025 deliberately LEAKS the sync dialog's store when the destructor runs with a blocking call on the stack, justified in the code verbatim by "this dialog is parented to `context->mainWindow`, which carries WA_DeleteOnClose" and "at parent-teardown time the application is already tearing down, so there is no live event loop left for a reaper to run on." DEC-030's reparent makes parent teardown a ROUTINE single-athlete-tab close after which the app keeps running — so a once-per-exit harmless leak became a once-per-athlete-close accumulating one, and TEST-084 asserts the leak as REQUIRED behaviour. Neither the DEC nor the build noticed; a fresh adversary reading the comment against the new parenting did.) Sibling: [[LSN-051]] — same wave, same root: a reparent is not a local edit.

## LSN-053
level:  guard scope:portable
tags:   op:verify op:decision type:surviving-mutant-read-as-dead-code
counters: recur:1 saves:1 miss:1
rule:   a mutation that SURVIVES the suite means exactly one thing — no current test kills it — and NEVER "the code it mutated is dead".
history:a mutation that SURVIVES the suite means exactly one thing — **no current test kills it** — and NEVER "the code it mutated is dead". Mutation results are a statement about COVERAGE, not about the program. Treating a surviving mutant as proof that a guard is unnecessary inverts the instrument: the weaker your tests, the more guards you will "prove" removable, and the removals land precisely where you had no coverage to catch the consequences. Check, before deleting ANY guard on the strength of a surviving mutant: (1) name the test that WOULD have caught it and confirm that test actually drives the guarded path — if none does, the mutant survived for lack of a test, and the correct action is to WRITE that test, not to delete the guard; (2) read the statements the guard protects and ask what they dereference; a guard whose next statement touches a collaborator is load-bearing by inspection regardless of what the suite says. (2026-08-11, O-R021-04: an A3 finding (A3-R021-F5) and the builder's own earlier mutation run (M8) agreed that 7 of 11 collaborator guards were "NOT load-bearing" because reverting them left the suite at 32/32. The orchestrator briefed a builder to DROP all seven. TWO of them guard the Save/SaveAll branches, which call `context->notifyMetadataFlush()`, `context->ride->notifyRideMetadataChanged()` and `context->mainWindow->saveSilent(context, ...)` on a freed Context — they survived only because NO TEST drove a window-hosted dialog into the dirty-rides prompt. The builder wrote the two missing runs and the drops became ASan aborts; the orchestrator then reproduced the heap-use-after-free in `Context::metadataFlush()` independently by applying its own briefed drop. Dropping them as briefed would have shipped two fresh UAFs of exactly the shape the REQ existed to close.) Sibling: [[LSN-047]] (an inert stub cannot prove the fault it is credited with) and [[LSN-050]] (an apparatus that cannot observe the phenomenon) — all three are the same family: **a green result is only as strong as the thing doing the observing.**

## LSN-058
level:  guard scope:portable
tags:   op:build op:cycle op:verify type:unedited-guard-promoted-from-dead-to-load-bearing
counters: recur:1 saves:1 miss:1
rule:   when a change makes a frame RESUME where it previously RETURNED — a return→continue, a new re-drive, a loop that now iterates…
history:**when a change makes a frame RESUME where it previously RETURNED — a `return`→`continue`, a new re-drive, a loop that now iterates past a suspension — every guard ALREADY STANDING between that suspension point and the new resume changes status from dead code to load-bearing, WITHOUT APPEARING IN THE DIFF.** A mutation matrix built from the diff cannot see them, because the diff never touched them. Check: after any such change, list every statement between the suspension point and the new resume, and for each pre-existing guard, mutate it INDIVIDUALLY and name the slot that kills it; if none does, it is an uncovered load-bearing guard and the slice is not done. Tell-tale: a guard carrying an "UNTESTED-BY-DESIGN / dead code w.r.t. current control flow" comment that the change has just falsified — the comment is now a lie and is the strongest possible hint. (2026-08-14, A3-R027-F1: DEC-032 turned `syncNext`'s parse-failure branch from `return true` into `continue`, promoting the pre-existing `if (self.isNull()) return true;` at `CloudService.cpp:2121` from dead to the only thing standing between a synchronously-destroyed dialog (DEC-030) and a loop that keeps iterating on `this`. Deleting it leaves the suite **47/47 green** — the orchestrator reproduced that survival by execution — while the BYTE-IDENTICAL guard in `uploadNext:2516` is killed by TEST-087's `UploadNextParsePE` frame with `heap-use-after-free … uploadNext() CloudService.cpp:2532`. The asymmetry is a missing fixture frame: `CompletionFrame` has no `SyncNextParsePE`. Third slice running that a guard family was analysed on `uploadNext` and inherited by `syncNext` without the test that makes it real.) Sibling: [[LSN-051]] (a reparent changes the exposure set) — that one is about a NEW owner, this one about a NEW control flow past OLD guards.

## LSN-059
level:  guard scope:portable
tags:   op:verify type:simultaneous-mutation-is-not-per-guard-coverage
counters: recur:1 saves:0 miss:1
rule:   mutating N guards SIMULTANEOUSLY proves only that SOME slot notices the aggregate;
history:**mutating N guards SIMULTANEOUSLY proves only that SOME slot notices the aggregate; it does NOT prove any individual guard is covered.** The simultaneous run answers exactly one question — "does any new guard prop up PRE-EXISTING coverage?" (all baseline slots must stay green) — and it is worth running for that. It cannot substitute for building and running each mutant ALONE. **Corollary, equally important: when two guards can catch the same mutant, verify that the assertion which encodes the CRITERION is the one that kills it**, not an auxiliary premise; otherwise the criterion's coverage silently depends on an observation a later edit may drop. Check: per guard, one build, one run, one named killing assertion — and confirm that assertion is the criterion's own. (2026-08-14, A3-R027 vs the orchestrator's own gate: the orchestrator ran all nine REQ-027 guards mutated at once, got "exactly 4 of 5 new slots fail, all 42 baseline green", and correctly concluded no guard propped up the baseline — but that run said nothing about per-guard coverage, and the adversary's individual runs then found M11 and M12 SURVIVING and M14 surviving entirely outside the diff-derived set. Separately A3-R027-F9: deleting the clause-(d) guard at :2146 is killed by TEST-097 only through `out.rideOpens == 0`; `out.writeFileCalls == 0` — the assertion that encodes clause (d) verbatim — still passes, because the new S-R027-01 guard catches the write.) Sibling: [[LSN-058]] (which guards to mutate) — this one is about HOW to mutate them.

## LSN-060
level:  guard scope:portable
tags:   op:design op:cycle type:suspension-set-omits-the-collection-element
counters: recur:1 saves:0 miss:1
rule:   a suspension-point safety analysis must enumerate the ROW / ITEM / ELEMENT the frame is holding, not only this, its collaborators and its owner.
history:**a suspension-point safety analysis must enumerate the ROW / ITEM / ELEMENT the frame is holding, not only `this`, its collaborators and its owner.** Any function that can CLEAR OR REBUILD the collection is a destroyer of that element, and a loop that captured a pointer into the collection before a suspension is holding a dangling pointer after it. Check: for every suspension point, list each raw pointer live across it — including loop-local pointers into a container — and for each, name what can destroy it and what excludes that; then grep for every function that clears/rebuilds the container and ask whether it is reachable from the events that suspension delivers. Tell-tale: a UI list rebuilt by a "Refresh" control that is never disabled during the operation. (2026-08-14, A3-R027-F2: `CloudServiceSyncDialog::refreshClicked` deletes every `QTreeWidgetItem` in all three lists (`CloudService.cpp:1527-1545`); the sync/upload loops capture `curr` at :1988/:2433 BEFORE `openRideFile`'s nested `QEventLoop` and read it at :2096/:2491 after — a live `heap-use-after-free` on UNMODIFIED production, with `refreshButton` never disabled (:1090/:1225, four references, no `setEnabled` anywhere) and the code's own comment at :2044 already stating a Refresh is deliverable from inside that call. **DEC-025, DEC-030 and DEC-032 each enumerated a suspension set — `this`, `store`, `context` — and not one of them ever included the row item.** REQ-027 then added a WRITE into that same window.) Sibling family: [[LSN-041]] other OBJECTS, [[LSN-046]] other POINTERS in a frame, [[LSN-048]] other FRAMES in an object, [[LSN-051]] the same frames under a NEW owner — this is the fifth axis: the CONTAINER ELEMENT the frame is holding.

## LSN-061
level:  guard scope:portable
tags:   op:delegate op:cycle type:briefed-from-a-reproduction-instead-of-the-finding-row
counters: recur:1 saves:0 miss:1
rule:   when dispatching work against a finding that ALREADY EXISTS in the ledger, brief from the FINDING ROW, not from a fresh reproduction of the symptom.
history:**when dispatching work against a finding that ALREADY EXISTS in the ledger, brief from the FINDING ROW, not from a fresh reproduction of the symptom.** A reproduction shows only the sub-defects that today's data happens to exercise; the recorded row is where the analysis lives, and it routinely enumerates shapes that NO current input exhibits — because the prose that triggered them was since reworded, the data moved, or the author found them by reading rather than by running. Re-deriving the defect from a live run therefore silently NARROWS the task, and the narrowing is invisible at the Verification Gate: the agent satisfies the briefing perfectly, the symptom disappears, and the unbriefed half of the finding stays open behind a green result. Check, before writing any briefing that names a finding id: open that finding's row, enumerate every distinct defect shape it records (they are often lettered (a)/(b)), and paste EACH into the briefing with its own required-outcome direction — then, at the gate, re-run the row's OWN cases rather than the tree. (2026-08-15, ORCH-015: the orchestrator briefed the drift-lint repair from the four false positives the lint was firing that morning, all of one shape — a status word whose SUBJECT is not the id on the line. The recorded ORCH-015 row documents **two** shapes, and the second, the PREDICATE-ADJECTIVE position (`the reaper (DEC-031) is frame-counted and deferred.` — a gap in the ORCH-010 repair itself), did not appear in the day's output at all, so no amount of reproducing from HEAD could have surfaced it. The builder delivered exactly what was asked, to a high standard, and the tree linted clean at exit 0 — and sub-defect (b) was still live, found only when the orchestrator opened the row to write the disposition and ran its recorded example lines. Cost: one re-dispatch, no wrong code, because the row was read BEFORE the finding was marked fixed rather than after.) Corollary: a finding is marked FIXED only when every shape ITS OWN ROW records has a passing case — the tree going quiet is not the criterion. Sibling: [[LSN-034]] (unverified premises in briefings) — that one is about premises that are FALSE, this one about premises that are TRUE but INCOMPLETE; and [[LSN-055]], where the same narrowing happens on the report side.

## LSN-062
level:  **guard→MECHANISM 2026-08-15 scope:portable**
tags:   op:verify op:build op:cycle type:evidence-collected-under-one-pinned-runtime-backend
counters: recur:1 saves:0 miss:1
rule:   NOW BACKED BY MECHANISM, one day after capture: the target is registered with ctest TWICE — testGarminConnectSyncDialogClose…
history:**NOW BACKED BY MECHANISM, one day after capture:** the target is registered with ctest TWICE — `testGarminConnectSyncDialogClose` (`offscreen`) and `testGarminConnectSyncDialogClose_minimal` (`minimal`), same executable, same `garmin-fast` label so `ctest -L` cannot run half the gate. The second registration was proven to WORK by going red on the unfixed tree before the fix landed. This is the third mechanism on this ledger ([[LSN-001]] anti-dup hook, [[LSN-008]] drift lint) and the fastest promotion yet, because the failure mode is invisible to inspection and only an executed second environment can catch it. **The residual the mechanism does NOT close, stated so nobody misreads two backends as all backends:** wayland failed BOTH affected slots where `minimal` failed one, and wayland cannot be gated because it needs a compositor — a divergence visible only there would still escape (→ ORCH-017 vi). | **when the test target's own registration PINS a runtime backend (`set_tests_properties(... ENVIRONMENT "QT_QPA_PLATFORM=offscreen;…")`, a fixed TZ, a pinned locale, a forced single-threaded mode), every gate that runs through that registration inherits the pin — so "ctest 26/26" and "target N/N" are statements about ONE environment, and a defect that only manifests under a different backend is invisible to ALL of them at once.** The gate stack looks independent (builder run, orchestrator re-run, CLV, clean-worktree build) and is not: they all go through the same pinned harness. Check, for any GUI/async/timing-sensitive target: run the binary DIRECTLY with no environment override — exactly as a developer would locally — and then under at least TWO backends of the same class (`offscreen` AND `minimal` are both headless; if they disagree, the phenomenon is backend-sensitive scheduling, NOT "needs a display"). Record which environment each green number was collected in, and never quote a bare "N/N" without it. **Tell-tale, and it is a loud one: a test whose own comment asserts it depends on ORDERING rather than TIMING.** That claim is falsifiable in one command and is exactly the claim that turns out to be wrong. (2026-08-15, A3-R027b-F1/F2: the orchestrator recorded "ASan target 49/49, ctest 26/26" as REQ-027's gate evidence. A fresh adversary ran the *exact command the briefing itself specified*, with no env override, and got **47 passed, 2 failed, deterministically** — `anAbortInsideACompletionSlotMustStopTheNextTransfer` failing with `a further store->readFile was issued after the user aborted (2 reads, expected 1)`, i.e. the precise harm A3-R027-F4's guard exists to prevent. The orchestrator then reproduced it: 47/2 under the ambient session, 47/2 under `minimal`, **49/49 under `offscreen`** — and `offscreen` is what `unittests/Core/garminconnect/CMakeLists.txt:1456` pins for ctest. **One of the two failing slots, `completionSlotsStandDownWhenTheAthleteTabDiesInTheirProcessEvents`, is COMMITTED at HEAD** (REQ-021 wave, `6dc794caf`) — so a shipped test has been failing in the developer's default environment while every gate reported green, including a clean-worktree build gate whose whole purpose was to catch what the working tree hides. The blind spot the clean-checkout gate was built to close had a second dimension nobody had looked at.) Sibling family: [[LSN-047]] (an inert stub cannot prove the fault it is credited with), [[LSN-050]] (an apparatus that cannot observe the phenomenon), [[LSN-053]] (a surviving mutant is a statement about coverage) — same root, new axis: **the ENVIRONMENT is part of the apparatus, and pinning it pins what you can see.**

## LSN-063
level:  guard scope:portable
tags:   op:cycle op:design op:verify type:guard-reinforcing-an-already-distributed-invariant
counters: recur:1 saves:0 miss:1
rule:   a new guard can be individually load-bearing against its own test and still be UNREACHABLE through every production route that…
history:**a new guard can be individually load-bearing against its own test and still be UNREACHABLE through every production route that exists — and the two facts feel identical from inside a mutation matrix, because a guard's dedicated test dies when you break it either way.** Mutation tells you the test covers the guard; it tells you NOTHING about whether any caller can arrive with the condition true. Check, and note it is a TRACE, not a mutation: enumerate every call site into the function and every in-function path that crosses a suspension, and for each, name the statement that establishes the guarded condition's value and whether anything between that statement and the call can pump events. If every route already establishes it, the guard is reinforcement, and the slice must SAY so. **What it must NOT say is "dead code" or "untested by design", and this is the whole point of the lesson:** unreachability because nothing LOCAL can change the condition (a `self.isNull()` guard whose window contains no destructor) is a property a reader verifies in fifteen lines; unreachability because N REMOTE callers each happen to re-check is a distributed invariant no reader of this function can verify and any future caller silently breaks. Labelling the second as the first invites the deletion the guard exists to survive. Write the reachability status, its KIND, and what the guard converts (distributed invariant → local one), and state that its tests are regression locks rather than proofs of a live fix. (2026-08-15, A3-R027c-F1: DEC-035 added abort reads immediately ahead of the transfers at `CloudService.cpp:2030`/`:2268`. Both are killed only by their own new TEST-105/106, on the criterion's own `readFileCalls == 0` — but all four callers (`completedRead:2435`, `failedRead:2495`, `completedWrite:2689`, `downloadClicked:1923`) re-establish `aborted == false` in the same statement block with no pump in between, and `syncNext`'s only suspension-crossing `continue` (`:2195`) re-checks at `:2186`/`:2193`. The adversary proved reachability by trace and proposed the `UNTESTED-BY-DESIGN` relabel; the orchestrator accepted the trace, spot-checked all four sites, and REJECTED the relabel for the reason above. The code's own comment had already called it "true-by-luck", which is why this is a labelling debt rather than a defect.) Sibling: [[LSN-059]] (mutation proves coverage, not reachability — same blind spot, opposite direction) and [[LSN-053]] (a surviving mutant is a statement about coverage).

## LSN-064
level:  guard scope:portable
tags:   op:commit op:verify type:mechanism-repair-uncommitted-so-the-gate-runs-the-old-one
counters: recur:1 saves:0 miss:1
rule:   pre-commit (and any hook framework that stashes unstaged work) runs the COMMITTED copy of the hook, not the one in your working…
history:**`pre-commit` (and any hook framework that stashes unstaged work) runs the COMMITTED copy of the hook, not the one in your working tree — so a mechanism repair you have not committed does not protect the gate it was written for, and the gate goes on silently enforcing the OLD rules while your ledger records the repair as done.** This is strictly worse than the mechanism being absent: the repair is verified green by hand, the finding is dispositioned FIXED, and the gate then fails on the exact false positives the repair removed — or, in the dangerous direction, PASSES something the repair would have caught. Check, and it is one command: after repairing any hook, extract and run the committed copy against the real tree — `git show HEAD:<path-to-hook> > /tmp/h.py && python3 /tmp/h.py .` — and treat a disagreement with your working copy as the repair being UNLANDED, not as a tree defect. Then commit the mechanism before relying on it. Corollary for dispositions: "the lint exits 0 on the real tree" is ambiguous and must be written as WHICH COPY exited 0; a repair is only closed when the committed copy is the repaired one. **When the gate does fire on a repaired-but-uncommitted mechanism, all three shortcuts are forbidden and for different reasons:** `--no-verify` is an agent routing around a hook ([[LSN-036]] class, a mechanism bug of equal weight to a missed catch); rewording the prose to satisfy a lint you know is wrong is precision-by-subtraction ([[LSN-008]]); reverting the repair restores a known-defective mechanism. The only honest move is to land the repair. (2026-08-16, ORCH-021: the docs commit for the DEC-035 wave aborted on four `DEC-030 paired with status 'GREEN'` violations while BOTH working-tree copies of the lint exited 0 on the same tree. The committed hook, md5 `2818e086…` against the working copy's `1b7b256b…`, reproduced all four — they are ORCH-015's own false-positive shape, fixed by a binding-scope repair that had never been committed. Two successive repairs of this lint, ORCH-010 and ORCH-015, were both verified by running the working-tree copy, which cannot see this class at all.) Sibling: [[LSN-062]] (the environment is part of the apparatus) — same root, new axis: **the gate runs a different BUILD of the mechanism than you just tested, and nothing tells you.**

## LSN-065
level:  guard scope:portable
tags:   op:delegate type:briefing-mandates-a-step-outside-the-agent's-tool-grant
counters: recur:1 saves:0 miss:1
rule:   a verification step you MANDATE in a briefing must be executable within that agent's tool grant — otherwise you have made an…
history:**a verification step you MANDATE in a briefing must be executable within that agent's tool grant — otherwise you have made an impossible action the precondition for the whole task, and the agent's options are all bad ones.** Subagents have different tools by definition (that is the point of specialised roles): a read-only researcher has no shell, so `md5sum`, `git`, a test run and a build are all unavailable to it. When the gate cannot fire, the agent must either stop (wasting the dispatch), silently skip it (destroying the gate's meaning), or claim it passed (the worst outcome, and the one you would never detect). Check, before writing any briefing sentence containing "verify", "run", "check that", or "STOP if": name the tool each step needs and confirm the target agent has it — the agent-type definitions in `.claude/agents/qgdw-*.md` list the grants, and a read-only role never has `Bash`. Where a checksum or command output genuinely matters, the ORCHESTRATOR runs it and pastes the value for the agent to compare, or the work goes to a role that has the tool. Corollary, and it is the useful half: **prefer the verification that tests the actual risk over the one that is merely traditional.** (2026-08-16, ORCH-024: the DEC-034 briefing opened with "**Verify that md5 before trusting any line number below** … If your md5 differs, STOP and report that instead", dispatched to `qgdw-scout`, whose grant is Read/Glob/Grep/WebSearch/WebFetch. It reported `No such tool available: Bash`, then substituted a STRUCTURAL verification — opening every symbol definition and offset the briefing named — which passed with zero drift and was strictly better matched to the hazard, since the risk was stale LINE NUMBERS and a checksum only proves the file is the one you hashed. It flagged the gate as UNFIRED rather than skipping or faking it.) Sibling: [[LSN-034]] (briefing premises that are false) — this is the same family one level up: not a false premise but an impossible INSTRUCTION.

## LSN-066
level:  guard scope:portable
tags:   op:commit op:verify op:phase-exit type:inventory-of-uncommitted-work-that-only-looked-at-git-status
counters: recur:1 saves:0 miss:1
rule:   git status does not answer "what uncommitted work exists here" — it answers "what is dirty in this worktree right now".
history:**`git status` does not answer "what uncommitted work exists here" — it answers "what is dirty in this worktree right now".** Work can also be sitting in a `git stash`, on an unmerged local branch, in another `git worktree`, or reachable only from the reflog, and none of it appears in a status listing however carefully you read one. Every accounting built solely from `git status` therefore reports a floor, not a total — and the omission is silent and can persist for months, because nothing ever prompts you to look. Check, whenever you enumerate uncommitted work (a commit gate, a wave close, a handoff note, or any sentence of the form "the N dirty entries are other owners' churn"): run `git stash list`, `git worktree list`, and `git branch -vv --no-merged` alongside `git status`, and state each result explicitly — including "no stashes", so the next reader knows it was checked rather than skipped. **Never resolve what you find by popping it**: a stash based on a months-old commit applied onto a tree carrying newer work from the same owners interleaves two states of the same files. Report it and let its owner decide; `git stash branch <name> <stash>` is the non-destructive inspection route, since it restores onto the stash's OWN base and leaves the current branch untouched. (2026-08-16, ORCH-023: found incidentally while checking whether a stale checksum had a benign cause — `stash@{0}`, created during REQ-002 on `6381b90f4` (2026-05-23), held 18 files and +651/-53 of the Coach owner's work, including +472 lines of `testCoachTools.cpp`. Verified not redundant: five of its files are CLEAN in today's worktree and its blobs for them differ from HEAD. Roughly three months and a dozen wave-close gates had each carefully enumerated "the 52/53/54 dirty entries" and not one had looked in the stash, because the question was always framed as "what is dirty?".) Sibling: the clean-worktree build gate, which exists for the identical reason — **every other gate ran inside the developer's working tree**, and so did every inventory.

## LSN-067
level:  guard scope:portable
tags:   op:delegate op:verify type:mandate-stripped-the-targets-pinned-configuration-not-just-the-gates-pin
counters: recur:1 saves:0 miss:1
rule:   "run it with no environment override" is not a stricter gate — it is a DIFFERENT PROGRAM.
history:**"run it with no environment override" is not a stricter gate — it is a DIFFERENT PROGRAM.** When you mandate stripping an env pin to widen a gate, name the ONE variable that is the gate's dimension and keep everything the target deliberately pins. A test target's sanitizer options, timeouts and fixture paths are its CONFIGURATION; only the thing you are varying is the pin. Check before dispatching: read the target's own `set_tests_properties(... ENVIRONMENT ...)` and list which variables are the DIMENSION being varied vs the target's configuration; strip only the former, and quote the exact env line the agent should run with. Why: a stripped configuration produces a red baseline the agent did not cause, and the agent's options are then to chase a phantom, to silently re-add what you told it to remove, or to stop. **NEW 2026-08-18 from ORCH-028** — mandating "no ctest env override" removed `ASAN_OPTIONS=detect_leaks=0` from a target that leaks BY DESIGN and documents it (`unittests/Core/garminconnect/CMakeLists.txt:1457/:1486`); the true baseline was 61/61 exit 0, the mandated one was exit 1 with 126280 bytes leaked. It is an over-generalisation of [[LSN-062]], whose rule is that the QPA BACKEND is a dimension of the gate — it says nothing about sanitizer options. Sibling of [[LSN-065]]: both are briefing mandates that made the impossible or the wrong thing a precondition, and both were caught by the agent executing them rather than by the orchestrator writing them.

## LSN-068
level:  guard scope:portable
tags:   op:decision op:build op:cycle type:state-armed-at-N-consumed-at-M-invalidated-nowhere
counters: recur:1 saves:0 miss:1
rule:   any state that is ARMED at N sites and CONSUMED at M sites must have its INVALIDATION sites enumerated in the SAME decision entry…
history:**any state that is ARMED at N sites and CONSUMED at M sites must have its INVALIDATION sites enumerated in the SAME decision entry — "armed at four, consumed at three, never disarmed" is an incomplete lifecycle, and the gap is INVISIBLE TO MUTATION TESTING because the defect lives in a site that does not exist.** Check, at decision time and again at the verification gate: write the three lists (arm / consume / invalidate) and name, for each way the guarded thing can DIE or become meaningless, which list entry covers it. If the invalidate list is empty, the DEC is not finished. Why: mutation measures the sites you wrote — every guard here was mutated individually and every mutant died, and the suite was 61/61 under two backends with zero sanitizer output, while four ordinary clicks reached a heap-use-after-free. **Second-order rule, and the sharper half:** when a fix REPLACES positional/re-resolving addressing with a HELD POINTER, it converts every unclosed lifetime route from wrong-data into memory-unsafe — so the invalidation audit is mandatory for exactly the fixes that look like an improvement. **NEW 2026-08-18 from A3-R028-F1 (BLOCKING)** — DEC-036's ticket outlived a `refreshClicked` that freed its row, and `downloadClicked`'s START branch re-synced `batchListGeneration` so the stale ticket passed every guard in the file. Pre-fix that route cost a mislabel through `child(listindex-1)`; post-fix it was a UAF through `inflight.row`. Sibling of [[LSN-060]] (state the suspension set explicitly) — that one asks WHAT must survive, this one asks WHEN it stops being valid.

## LSN-069
level:  guard scope:portable
tags:   op:build op:verify type:an-assertion-whose-value-is-FORCED-is-a-premise-not-a-verdict
counters: recur:2 saves:0 miss:2
rule:   REFINEMENT 1 (2026-08-23, A3-R038-F2) — THE FORCING AGENT NEED NOT BE THE FIXTURE.
history:**REFINEMENT 1 (2026-08-23, A3-R038-F2) — THE FORCING AGENT NEED NOT BE THE FIXTURE. IT CAN BE PRODUCTION'S OWN INVARIANT, and that case is worse, because no amount of fixture review finds it.** DEC-038's `restoreListSorting` restores a per-list recorded `enabled` bit (`CloudService.cpp:1614`). The bit is written at one site (`:1545`) and read at one (`:1614`), and `suspendListSorting`'s own early-out (`:1537`) guarantees it can NEVER be recorded false in any reachable production state. So all 89 slots assert the identical `[1,1,1]` shape and not one of them can discriminate "restores what was recorded" from "always force-enables" — the mutation `states[i]->enabled` → `true` is predicted to survive the entire two-backend suite. **THE CHECK: for any save/restore pair, ask what values the saved field can ACTUALLY take at the moment it is read. If the answer is "one", the restore branch is untested by construction and no assertion over it is a verdict — testing it needs a SYNTHETIC precondition production cannot reach, and that synthetic slot must be labelled as such so nobody later mistakes it for a reachable route.** Distinguish this from [[LSN-063]]: there a guard was covered but unreachable; here the guard is reachable but its interesting branch is unobservable. | **when a test's FIXTURE changes, re-derive whether each surviving COUNTER assertion can still FAIL — an assertion whose value is forced by the fixture's own row/item count is a PREMISE, not a verdict.** Check: for every `QCOMPARE(x, N)` in a slot whose fixture you touched, ask what the maximum possible value of `x` is given the new fixture; if max == N the assertion cannot discriminate, so widen the fixture or re-label the line as a premise. Why: it fails silently in the safest-looking direction — the slot stays green, the comment above it still claims to measure the old thing, and the claim now rests on nothing. **NEW 2026-08-18 from A3-R028-F3** — TEST-087's repaired `CompletedWritePE` premise `QCOMPARE(wrote.writeFileCalls, 1)` sat over a fixture with exactly ONE sync row, so it held whether or not the slot stood down; its sibling `CompletedReadPE` has four rows, which is what made the asymmetry visible. Caught by the A3 only because the Verification Gate had flagged the altered assertion as B-R028-09 and asked the cycle to judge it.

## LSN-070
level:  guard scope:portable
tags:   op:cycle op:build type:reachability-claim-that-does-not-terminate-at-an-emitting-call-site
counters: recur:1 saves:0 miss:1
rule:   a REACHABILITY claim must terminate at a concrete emitting call site with file:line — not at a function that COULD return the triggering value.
history:**a REACHABILITY claim must terminate at a concrete emitting call site with file:line — not at a function that COULD return the triggering value.** "`replyName()` returns `""` for an unmapped reply" is a statement about a function; the claim needs "…and HERE is the call that emits with an unmapped reply". If the trace cannot be terminated, label the guard **"reasoned, not traced"** and keep it as defence-in-depth. Why: an untermin­ated trace reads exactly like a terminated one in a report, and it upgrades a defensible guard into a false claim about production — which the next reader will either trust or delete. **NEW 2026-08-18 from A3-R028-F4** — DEC-036 claimed `completedWrite`'s `isWrite` clause was traced via an empty write id; but every `writeFile` in `src/Cloud/` calls `mapReply(reply, remotename)` first (20 call sites), and the only real empty-id emitter, `LocalFileStore`, emits synchronously from inside `writeFile` and can never be late. **Runs in the OPPOSITE direction to [[LSN-063]]** — that one was captured because a guard was called dead when it was merely uncovered; this one because a guard was called traced when its trace did not terminate. Together they are one rule: say which KIND of claim you are making, and terminate it.

## LSN-071
level:  guard scope:portable
tags:   op:delegate type:briefing-asserted-a-test-apparatus-that-does-not-exist
counters: recur:1 saves:0 miss:1
rule:   before a briefing tells an agent to REUSE an existing test apparatus — a counter, a fixture, a helper, a fake — GREP FOR ITS SYMBOL.
history:**before a briefing tells an agent to REUSE an existing test apparatus — a counter, a fixture, a helper, a fake — GREP FOR ITS SYMBOL.** A ledger that says "clause (b) asserts `save()` is called zero times" is describing a CLAIM, not an apparatus: the assertion may be a labelled PROXY, and "reuse the counter" then sends the agent looking for something that was never built. Check: one `grep -rn "<symbol>" <test dir>` per apparatus the briefing names, and if the hits are prose or a no-op stub, say so IN the briefing and let the agent choose. Why: the agent's options when the named thing is absent are all bad — stop and lose the run, build a second apparatus you told it not to build, or fabricate a count. Why it recurs: the property being asserted and the mechanism asserting it are recorded in the same ledger sentence, and memory merges them. **NEW 2026-08-19 from ORCH-031** — the briefing said "the suite already counts `rideCache->save()`"; there is no counter, only TEST-109's proxy (the tail's "…successfully" sentence, sound because `save()` is called at exactly two unconditional sites one line below it) and a non-virtual no-op stub shared by three targets. **Third falsified briefing premise in three consecutive dispatches, all three about the EVIDENCE apparatus rather than the code** — sibling of [[LSN-067]] (a mandate that stripped the target's pinned configuration) and [[LSN-065]] (a mandate outside the agent's tool grant). The common shape: assertions about HOW something will be verified are made from memory of the ledger, while assertions about the code are made from the file. Hold both to the file.

## LSN-072
level:  guard scope:portable
tags:   op:build op:cycle op:verify type:no-pump-claim-derived-from-a-target-that-stubs-the-collaborator
counters: recur:1 saves:0 miss:1
rule:   a green run on a target that STUBS a collaborator to a no-op cannot support any claim of the form "nothing between X and Y…
history:**a green run on a target that STUBS a collaborator to a no-op cannot support any claim of the form "nothing between X and Y suspends/pumps events" — that claim must be re-derived against PRODUCTION's call graph.** Check: for the span between a liveness guard and the write it protects, list every call, then for each one ask whether THIS TARGET stubs it; if it does, follow the real implementation transitively looking for `QEventLoop`/`exec()`/`processEvents(`. Record in the test file every stubbed seam that DELETED a suspension point. Why: the fixture can drive the branch — so coverage looks complete — while covering a version of the function with the hazard surgically removed. **NEW 2026-08-19 from A3-R028b-F1 (BLOCKING)** — `saveRide()` sits between `completedRead`'s last row-liveness compare and its row write; in production it reaches `autoProcess` → `postProcess` → `FixElevation`'s untimed `QEventLoop` on an HTTPS round trip, and the target stubs `autoProcess`, `setLinkedDefaults`, `addRide` and `RideCache::save` flat. The `.gcblock` fixture DOES drive the branch, and that is exactly what hid it. Sibling of [[LSN-063]]: there mutation could not prove reachability; here a green fixture could not prove absence of suspension.

## LSN-073
level:  guard scope:portable
tags:   op:build op:cycle type:suspension-point-census-taken-from-known-collaborators-not-from-a-grep
counters: recur:1 saves:0 miss:1
rule:   enumerate suspension points by GREPPING processEvents( and QEventLoop/exec() in the function under guard AND in everything it…
history:**enumerate suspension points by GREPPING `processEvents(` and `QEventLoop`/`exec()` in the function under guard AND in everything it calls transitively — never by listing the known nested-loop collaborators.** And never write the census as closed ("the five suspension points"); write it as "the delivery points this suite drives", and carry the residual list. Check: `grep -n 'processEvents(\|QEventLoop\|exec()' ` over the guarded functions, then one level out through their callees. Why: the collaborator-shaped ones (store calls, ride-file readers) are the ones anybody would list; the misses are plain `processEvents()` in the dialog's own code and whole FAMILIES nobody classified as suspending. **NEW 2026-08-19 from A3-R028b-F2 (BLOCKING) and F8** — this ledger's census claimed five delivery points; three consecutive A3s each added one more (the completion tails, the parse-failure branches, the data processors), and the same closed-world phrasing sits in production comments too. Corollary from F2: **a guard at a function's ENTRY answers "was it replaced before I was CALLED"; a resumption needs "was it replaced while I was SUSPENDED" — they are different questions and one does not cover the other.**

## LSN-074
level:  guard scope:portable
tags:   op:decision op:build type:identity-key-narrower-than-the-space-it-must-separate
counters: recur:1 saves:0 miss:1
rule:   a per-transfer/per-request ticket keyed on a STRING must be checked against every collection that can generate that same string.
history:**a per-transfer/per-request ticket keyed on a STRING must be checked against every collection that can generate that same string.** Check, at decision time: name the key, then grep for every site that constructs it and every widget/model whose contents feed it; if two distinct objects can produce an equal key, the ticket cannot separate them and needs a second clause (a monotonic per-dispatch sequence, or the collection's identity folded in). Why: an identity mechanism that is right within one collection reads as right in general, and the measurement that "discharges" it is usually taken within one collection too. **NEW 2026-08-19 from A3-R028b-F3 (BLOCKING)** — DEC-036's write ticket keys on `remotename`, and the SYNC list's upload row and the UPLOAD list's row for the same activity both carry `text(1) == ride->fileName`, so both arm sites compute an identical key; a cross-tab abandoned write is then ACCEPTED against the live batch's row, breaking the "at most one transfer outstanding per live batch" invariant the DEC rests on. **The invariant HAD been discharged by measurement — 0 overlaps across 61 slots with a working positive control — but every slot ran within one tab: the measurement was sound and NARROWER than the claim it supported.** Corollary: when you discharge an invariant by instrumentation, state the SCOPE the runs actually covered.

## LSN-075
level:  guard scope:portable
tags:   op:decision op:build type:invalidation-justified-only-on-safety-never-costed-for-what-it-destroys
counters: recur:1 saves:0 miss:1
rule:   when a repair adds a CONTAINER of correlation state, every site that INVALIDATES that container must be re-derived for what the…
history:**when a repair adds a CONTAINER of correlation state, every site that INVALIDATES that container must be re-derived for what the invalidation COSTS, not only for what it makes safe.** Check, at decision time: for each invalidation site, ask "what discrimination does this destroy, and is there a route where that discrimination was the whole point of the container?" — then split the state if the answer is yes (a row-free tombstone keeps the NAME while dropping the pointer that must die). Why: an invalidation reasoned purely on memory safety is always CORRECT and can still be the defect, so the usual review question ("is this safe?") returns yes and stops. **NEW 2026-08-21 from A3-R028c-F1 (BLOCKING)** — DEC-037 recorded `retiredWrites.clear()` in `refreshClicked` as "MANDATORY" on memory-safety grounds and nobody asked what it cost; on Abort→Refresh→restart it drops the ticket the mechanism exists to retain, re-opening the blocking defect DEC-037 was written to close, on a five-click single-tab route. Sibling of [[LSN-068]]: that one is a lifetime with no invalidation site, this one is an invalidation site with no cost analysis — the two failure modes of the same enumeration.

## LSN-076
level:  guard scope:portable
tags:   op:decision type:disproof-argues-from-a-behaviour-the-accepted-option-changes
counters: recur:1 saves:0 miss:1
rule:   when a decision's DISPROOF cites CURRENT behaviour as a reason to reject an alternative, re-check that the ACCEPTED option does…
history:**when a decision's DISPROOF cites CURRENT behaviour as a reason to reject an alternative, re-check that the ACCEPTED option does not itself change that behaviour before the entry is filed.** Check: list every "we reject X because today the code does Y" clause, then ask whether the chosen option still does Y afterwards; if not, the rejection reasoning is void even when the rejection is right. Why: the disproof is written while reasoning about the alternatives and filed after choosing one — nobody re-reads the rejected branches against the accepted diff. **NEW 2026-08-21 from A3-R028c-F4** — DEC-037 rejected disarm-on-abort partly because it "would silently delete the 'Aborted' label an abandoned write currently writes to its own row", and DEC-037's own retired path then wrote `result` instead of "Aborted" on exactly the route the entry is about. The rejection stands on other grounds; the reasoning does not. Note the direction the A3 corrected this in: the CODE is more compliant with the acceptance criterion, the RECORD is what is wrong.

## LSN-077
level:  guard scope:portable
tags:   op:verify op:cycle type:hand-enumeration-past-the-point-where-it-is-the-right-tool
counters: recur:1 saves:0 miss:1
rule:   once a component carries FIVE-PLUS interacting guard mechanisms, stop enumerating click sequences by hand and install a…
history:**once a component carries FIVE-PLUS interacting guard mechanisms, stop enumerating click sequences by hand and install a CONTINUOUS INVARIANT ORACLE that every existing run evaluates.** Check: name the one-line invariants the design already claims ("at most one transfer outstanding", "tickets conserved", "idempotent restart"), assert them in the harness's observer so they are checked in EVERY run rather than in the one slot someone thought to write, and add a randomized click-sequence driver with the sanitizer as a second oracle. Why: hand enumeration finds the sequences someone imagined; the defects that survive three adversarial cycles are by definition the sequences nobody imagined, and each new mechanism multiplies the state space rather than adding to it. **NEW 2026-08-21 from A3-R028c-F2 (BLOCKING) and the adversary's own standing-question answer** — six mechanisms in one dialog, 71/71 green on two backends, and a two-click burst into a completion tail put two transfers on the wire against one ticket. An `outstanding <= 1` assertion in the observer would have caught it in whichever run happened to drive the burst, instead of requiring someone to guess the route. This is the [[LSN-001]]/[[LSN-008]]/[[LSN-062]] escalation path — advisory to guard to MECHANISM — applied to test oracles rather than to process.

## LSN-078
level:  guard scope:portable
tags:   op:ledger-update op:orient type:budget-line-carries-a-verdict-nobody-recomputes
counters: recur:1 **saves:2** miss:1
rule:   SAVE #2, 2026-08-22: this guard is what made the commit-safety gate re-run the suite instead of trusting CURRENT's "full…
history:**SAVE #2, 2026-08-22:** this guard is what made the commit-safety gate re-run the suite instead of trusting `CURRENT`'s "full offscreen 78/0 · green on both backends" — which was false **on the half that mattered**: the target aborted at test 32 on BOTH backends (ORCH-033). **The "78" was NOT the false part, and saying so was my own error — corrected here so the lesson does not teach a wrong check:** `-functions` reports **73 declared test functions** and QtTest's `Totals:` reports **78 completed cases**, the difference being `initTestCase`, `cleanupTestCase` and data rows. The two are compatible; neither is drift. **The refinement this earns: when two counts of "the tests" disagree, first establish what each one COUNTS — a units mismatch looks exactly like drift, and calling it drift manufactures a finding that costs real time to disprove.** The recorded verdict had almost certainly been produced BEFORE the last edit to the test file and then carried across it, with only the refined slots ("6/0 on each backend") re-run. **The generalisation this save earns: a suite-wide verdict is invalidated by ANY subsequent edit to ANY file the target compiles — re-running the changed slice is not re-running the suite, and a "refinement" is an edit.** | **a telemetry line that records a VERDICT ("cap-ok", "compaction NOT due", "no breach") must carry the command that produced it and be RECOMPUTED — never copied forward — at every edit of the file it measures.** Check: the line states the measurement (`wc -c`/4), the date, and the cap, and the verdict is re-derived from a fresh run before the line is touched; a verdict without its command is prose. Corollary: measure it against the cap the SKILL states, not against the number that happened to be there last time — a value can be over cap on the day it is written and still get recorded as "ok" because it improved. Why: the budget line is edited as a byproduct of other work, and the cheapest edit is to leave the verdict alone and update the prose around it; the verdict then ages into a false negative that actively SUPPRESSES the dispatch it exists to trigger — the mechanism reads as green precisely because nobody ran it. **NEW 2026-08-22, caught by the user asking "is this a good moment to run the librarian?"** — STATE.BUDGETS read "WIKI ~13.5k chars/cap-ok … librarian Job-3 compaction NOT due", carried forward unmeasured from 2026-08-05 through the REQ-027 and REQ-028 waves. Measured: 35,498 chars = ~8,874 tok against a 700-tok cap, a 12.7x breach; and ~13.5k was already 4.8x over when it was labelled cap-ok. Its LSN counters were stale by 21 lessons in the same line. Second-order: **the breach was in REGISTRIES, not the MAP** — the hub had absorbed per-id narrative it is supposed to POINT at, which is [[LSN-035]] committed by the hub itself, so a compaction dispatched on the skill's default assumption (MAP directory-rollup) would have compacted the wrong section. Sibling of [[LSN-008]]: index-vs-detail drift, one level up — there the index disagreed with the detail, here the index ATE it.

## LSN-079
level:  guard scope:portable
tags:   op:build op:verify op:cycle type:observer-embedded-in-the-object-it-observes
counters: recur:1 saves:0 miss:1
rule:   an observer that lives INSIDE the object it observes dies with it — so every delivery point that can fire after the subject's…
history:**an observer that lives INSIDE the object it observes dies with it — so every delivery point that can fire after the subject's destructor must guard the OBSERVER's liveness, not only the subject's.** Check, before a harness ships: for each place the test delivers something asynchronously, ask which of the captured pointers the code under test is allowed to delete, and confirm each is a `QPointer`/guarded handle rather than a raw capture. A `QPointer` on the SUBJECT is not the guard — it proves the thing being watched is alive, not the thing doing the watching. Why: an oracle is written to survive its subject's death (that is its whole point), which makes it feel guarded; but it is allocated as a member of a store or dialog the code under test owns, so the destructor takes the observer with it and the very next delivery reads freed memory. The failure is invisible to review because the guard that IS present reads as the guard that is missing. **NEW 2026-08-22 from ORCH-033 — and the MECHANISM below is the corrected one; the first diagnosis named the wrong lifetime window and would have produced a fix that did not hold.** `TransferOracle` is a member of `BlockingStore`; its `dialog_` is a `QPointer` and survives the dialog. **The queued delivery begins while the store is ALIVE.** It is `CloudService::notifyWriteComplete` — the *base* call — that invokes the dialog's completion slot DIRECTLY; that slot runs `processEvents()`, a teardown delivered there destroys the dialog, and `~CloudServiceSyncDialog` deletes the store **re-entrantly** via `closeAndDeleteStore` (`CloudService.h:325`). Control then returns into a member function of a freed object and the **next statement after the base call** touches `oracle_`. The proof is the crash frame's position: the UAF is at the line AFTER the base call, which a store already dead at entry could never have reached. **The sub-lesson worth as much as the rule: "the object was dead when the callback started" and "the object died inside the callback" produce the same sanitizer report and demand different fixes — read the crash frame's POSITION relative to the re-entrant call, not just its function.** Deterministic on both backends; it halted the suite at test 32 — **an oracle built to make the evidence trustworthy became the thing that destroyed the evidence.** Fixed in `e48f7d123` by guarding BOTH windows: `QPointer` handles on the queued delivery's captures (so a callback posted after an earlier teardown is a no-op) AND a `QPointer` self-guard across each base notification, with nothing touched through `this` once it is null. Same family as [[LSN-060]] (a suspension set must include what the frame HOLDS) applied one layer out: to the harness itself. Sibling of [[LSN-072]] — both are about trusting a test apparatus whose own construction was never audited.

## LSN-080
level:  guard scope:portable
tags:   op:verify op:gate type:a-clean-build-gate-run-in-a-REUSED-build-directory-is-not-a-clean-build
counters: recur:2 saves:0 miss:2
rule:   REFINEMENT 1 (2026-08-23, SAME DAY, SAME FINDING — ORCH-036's SECOND wrong formulation, caught by the USER, not by me).
history:**REFINEMENT 1 (2026-08-23, SAME DAY, SAME FINDING — ORCH-036's SECOND wrong formulation, caught by the USER, not by me).** Having withdrawn the OFF-path claim I then asserted *"HEAD cannot select the libusb-1.0 API under ANY configuration"* on the reasoning that `LIBUSB_V_1` is never `set()` in the project's CMake. Also false: `if(NOT LIBUSB_V_1)` reads an ORDINARY VARIABLE, and `-DLIBUSB_V_1=ON` on the command line satisfies it. MEASURED once I finally ran it: 426 compile entries carry the define, `EzUsb-1.0.c` is compiled, legacy `EzUsb.c` is not. **THE GENERALISED RULE, which is what makes this a lesson rather than a repeat: "the project never SETS X" is not the same claim as "X cannot BE set", and for any build-system variable the second claim is almost always false — build systems exist to be configured from outside. Never characterise a build defect from reading the build files alone: RUN THE CONFIGURATION AND READ WHAT IT PRODUCED (`compile_commands.json`, `CMakeCache.txt`, the actual compiler invocation). Two wrong formulations of one finding in one day, both from inspecting inputs instead of observing outputs.** The correct defect was narrower and more useful than either wrong version: the selection mechanism WORKS, but there is no complete supported dependency wiring behind it. | **A CLEAN-CHECKOUT GATE IS ONLY AS CLEAN AS ITS BUILD DIRECTORY. Re-configuring an EXISTING build tree with different options is NOT a clean build: CMake regenerates, but generated trees the configure no longer owns — AUTOGEN/moc output, previously-globbed sources, stale object files — SURVIVE, and they are compiled.** NEW 2026-08-23 from ORCH-036, where I reconfigured a `GC_HAVE_LIBUSB=ON` build tree to `OFF` and the leftover `moc_FortiusController.cpp` from the ON pass failed to compile. I read that failure as *HEAD does not build from a clean checkout of its own defaults*, raised it BLOCKING with `{RELEASE}` scope, and wrote it into findings.md, STATE.md and WIKI.md before a fresh build directory disproved it — the identical step built fine, and the moc file did not even exist. **THE CHECK: a clean-build gate uses a build directory that did not exist before the gate started. If you change ANY configure option, you need a NEW directory, not a re-configure — and the evidence line must name the directory and say it was fresh.** Sub-rule, because this is what made the false claim so plausible: **the failing artifact will be a GENERATED file that is not in the source tree, so every grep you run to confirm the defect looks consistent with it** — I confirmed `Fortius.h:189` really does declare `LibUsb *usb2;` and concluded too fast. When a build error names a generated file, verify the generator's INPUT LIST (here `AutogenInfo.json`) before believing the file should exist at all. Cf. [[LSN-066]]: the environment you are measuring is rarely as pristine as the claim you are about to make about it.

## LSN-081
level:  guard scope:portable
tags:   op:verify op:gate type:a-test-budget-set-by-the-CALLER-makes-the-verdict-a-function-of-machine-load
counters: recur:1 saves:0 miss:1
rule:   A GATE WHOSE TIME BUDGET LIVES IN THE INVOKING COMMAND RATHER THAN IN THE TEST IS NOT A GATE — its verdict is a function of…
history:**A GATE WHOSE TIME BUDGET LIVES IN THE INVOKING COMMAND RATHER THAN IN THE TEST IS NOT A GATE — its verdict is a function of whatever else the machine is doing.** NEW 2026-08-30 from [[ORCH-047]]. Four heavy registrations in `unittests/Core/garminconnect/CMakeLists.txt` carried LABELS and ENVIRONMENT but **no `TIMEOUT` property**, so each test's allowance was whatever the caller typed (`ctest -L garmin-fast --timeout 180`). Measured on ONE binary at ONE commit: **87 s idle, 179 s loaded, KILLED at 180 s, 142 s loaded** — 1 in 3 loaded runs failed a suite whose 91 assertions all pass. **THE CHECK: for every registered test, the budget must be a property OF THE TEST (`set_tests_properties(... TIMEOUT n)`), sized against its MEASURED worst case under load, not its idle time — and the measurement recorded next to the number.** A CLI `--timeout` is a floor for unmeasured tests, never the budget for a known-slow one; the per-test property overrides it (verified empirically). **THE REASON THIS IS A GUARD AND NOT A NOTE:** the damage is not the false red, it is what the false red teaches. A gate that fails for reasons unrelated to the code trains every future reader to explain a red away — so the one time it goes red for a REAL defect, the habit is already in place to wave it through. Same family as the single-pinned-QPA defect the same file documents: [[LSN-066]] — the environment you measure in is rarely as pristine as the claim you are about to make. Corollary for reports: when a run is killed by a timeout, that is **not** a test failure and must never be recorded as one — it is an unfinished measurement, and the distinction is what makes the defect findable.

## LSN-082
level:  guard scope:portable
tags:   op:verify op:gate op:ledger-update type:a-mechanism-that-reports-an-unreadable-input-as-not-blocking-is-failing-open
counters: recur:1 saves:0 miss:1
rule:   WHEN A CHECK CANNOT READ ITS INPUT, THE ONLY SAFE VERDICT IS FAIL.
history:**WHEN A CHECK CANNOT READ ITS INPUT, THE ONLY SAFE VERDICT IS FAIL. "Reported, not blocking" is not a lenient verdict, it is the ABSENCE of a verdict wearing a verdict's clothes.** NEW 2026-08-30 from [[ORCH-049]]. CLV Check 5 classified 28 unparseable rows as `NEEDS-CLASSIFICATION` and let the overall check pass over them — and several of those rows literally begin `**BLOCKING — …**`. It ALSO silently skipped every row whose severity cell held prose instead of a severity, on the predicate "if the cell does not start with `blocking`, skip": 114 rows left the check without anyone deciding they were harmless. **THE CHECK: for every classifier, enumerate the inputs it CANNOT interpret and name the bucket each falls into; if any such bucket is non-blocking, the mechanism can report clean over a live blocker. Write the unreadable buckets FIRST, before the interesting logic.** Corollary, and the sharper half: **a parser must be tested against the rows it CANNOT read, not only the ones it can** — a suite built from well-formed fixtures cannot detect this class at all. Sub-rule from the same finding: `ORCH-009` had seven columns AND no trailing pipe, which splits into exactly the field count a well-formed six-column row produces, so the old predicate read it as valid and took its severity from the wrong cell — **structural validation must assert the shape it EXPECTS, never merely count separators.** Sibling of [[LSN-008]] (index-vs-detail drift) one level down: there the index disagreed with the detail, here the mechanism could not see the detail at all.

## LSN-083
level:  guard scope:portable
tags:   op:gate op:ledger-update type:a-gate-whose-actions-cannot-change-its-own-pass-criteria
counters: recur:1 saves:0 miss:1
rule:   BEFORE PUBLISHING A GATE, TRACE ITS ACTIONS THROUGH THE MECHANISM THAT COMPUTES ITS PASS CRITERIA.
history:**BEFORE PUBLISHING A GATE, TRACE ITS ACTIONS THROUGH THE MECHANISM THAT COMPUTES ITS PASS CRITERIA. If the actions do not touch an input that criterion reads, the gate is unsatisfiable and every hour spent on it is wasted.** NEW 2026-08-30 from [[ORCH-051]]. A NEXT_GATE stated that adding effect classifications (`BLOCKS: {…}`) to eight findings would reduce Check 5's OUTSTANDING set from 15 to 7. It could not: Check 5 computes OUTSTANDING from **severity + disposition**, and reads the effect set **nowhere**. The gate's own actions were incapable of moving its own number, and nothing in the prose revealed that — because the prose was written from what the concepts MEAN rather than from what the code READS. **THE CHECK: name the exact variable the pass criterion is measured from, then name the file and line the gate's actions edit, then show the path from one to the other. No path, no gate.** **THE UNDERLYING CONFUSION, worth naming on its own: CLASSIFICATION IS NOT DISPOSITION.** An effect set says WHAT a finding blocks; a disposition says WHETHER anyone still owes work on it. They are independent axes and either can be true without the other — so writing `BLOCKS: {RELEASE}` onto an open finding closes nothing, and writing `BLOCKS: {}` closes nothing either. Recording them in one column, or letting one gate the other, produces exactly this class of impossible gate. Sibling of [[LSN-078]]: there a verdict was copied forward without its command; here a verdict was PREDICTED without its mechanism.

## LSN-084
level:  guard scope:portable
tags:   op:mutation-proof op:restore type:git-checkout-on-a-file-with-PRIOR-uncommitted-changes-discards-all-of-them
counters: recur:1 saves:0 miss:1
rule:   git checkout -- <file> DOES NOT UNDO "the edit I just made" — it restores the ENTIRE working-tree file to HEAD, discarding every…
history:**`git checkout -- <file>` DOES NOT UNDO "the edit I just made" — it restores the ENTIRE working-tree file to HEAD, discarding every uncommitted change to it, not only the one intended to be reverted.** NEW 2026-09-05 from a self-inflicted incident during DEC-040 Stage 2 piece 3b's independent mutation-proof cycle. `src/Cloud/CloudService.h` carried ~55 UNCOMMITTED lines from Stage 2 pieces 1+2 (the `CancelToken` class, `setCancelToken`, `cancelToken_`, `kCancelPollMs`) sitting on top of a clean commit. A one-line mutation was applied to `CancelToken::cancelled()` for a mutation-proof, then "restored" with `git checkout -- src/Cloud/CloudService.h` — which silently reverted the file all the way to HEAD, destroying all 55 uncommitted lines, not just the 1 mutated one. The loss went undetected for one command (`git diff --stat` came back EMPTY, read as "restored cleanly" rather than "reverted to the wrong baseline") until the next `git status` on a sibling file exposed it. **THE CHECK: before mutating any file for a proof, ask whether the file's CURRENT working-tree state already differs from HEAD for reasons unrelated to the mutation. If it does, `git checkout --` is not a safe restore for that file — snapshot first (`cp file file.orig`) and restore via `cp`/`mv` (verified by this same project's own piece-2 evidence log, which used exactly that method and was never at risk). `git checkout --` is safe ONLY for a file that is otherwise byte-identical to HEAD before the mutation — verify that with `git diff --stat` BEFORE mutating, not just after "restoring".** Recovery cost here: the class/members were reconstructed from three independent, trustworthy sources — the exact `CancelToken` body preserved verbatim in `decisions.md`'s DEC-040 entry, the exact member names/types/constant read back out of `CloudService.cpp`'s still-intact (never-touched) piece-2 diff which USES them, and placement clues from an evidence log and this session's own prior `grep` output — then re-verified by the full, unchanged 168-test suite passing on both QPA backends. A `git fsck --dangling --unreachable` search for a recoverable blob found none: the file had never been `git add`-ed, so nothing backed it in the object database. Sibling of [[LSN-080]] (a reused build directory silently carries stale state across a "clean" step) and of this session's own [[ORCH-053]] (an unverified "restored/passing" claim hid a real gap) — in both families, the thing that looks like confirmation (`no work to do`, an empty `git diff --stat`) is confirming the WRONG baseline, and the fix is always to name the exact bytes being compared before trusting the comparison. |

## LSN-085
sig:    build / test / fixture-growth-unbounded-in-one-binary
level:  advisory   since:2026-09-06   recur:1   saves:0   miss:0
tags:   op:build, op:test, type:fixture-growth-unbounded-in-one-binary, scope:portable
trigger:about to add another slot/fuzzer iteration to an already-large QTest binary, OR about to
        propose splitting one
rule:   a skill-level fixture-growth cap is PROPOSED, not yet adopted: flag a single QTest binary
        once it passes a slot-count / line-count threshold, and pair any split-by-mechanism
        remediation with a build-system-level target split (a genuine separate compilation unit),
        never a hand-maintained function ALLOWLIST — QTest's selector can INCLUDE named functions
        but cannot EXCLUDE one, so a hand split silently drops every slot added after the split.
check:  before proposing a split for an oversized test binary, confirm the test framework offers
        an EXCLUDE selector or the split is done at the build-target level (a new .cpp with its
        own `add_test`), not a maintained "keep these N functions" include list.
why:    this project's own `testCloudProviderWatchdog.cpp` / `testGarminConnectSyncDialogClose.cpp`
        pair grew to ~8,874+ combined lines, 87+ slots and a 512-seed fuzzer in ONE binary. A split
        was considered and explicitly declined for exactly this reason (ORCH-061, `findings.md`) —
        the remediation would have been a worse defect (silent coverage loss) than the problem
        (one large binary). Nothing in the currently-installed skill
        (`references/orchestration.md`, `references/lessons-memory.md`) states a fixture-growth cap
        or a split convention, so there was no rule to check against and no violation to report —
        this is a proposed NEW rule, not a caught mistake.
origin: ORCH-061 (`findings.md`, librarian Job-3 compaction, 2026-09-06) — a genuine skill-level
        gap identified during ledger compaction, not a project compliance failure.
history:2026-09-06: captured at advisory, first occurrence, recur:1 saves:0 miss:0. Per this
        project's standing convention (QGDW package fixes go upstream), this lesson and ORCH-061
        are NOT applied to the installed skill package
        (`.claude/skills/quality-gated-dev-workflow/**`, `.claude/agents/qgdw-*.md`) by this
        project — they are recorded here as a lesson-miss recommendation for whoever maintains the
        skill package to port a fixture-growth-cap / split-by-mechanism rule into
        `lessons-memory.md`/`orchestration.md` in a future skill update. `scope:portable` because
        the gap is about the WORKFLOW's own reference docs, not this codebase.

## LSN-086
sig:    test-design / integration / a-fake-that-defines-an-api-the-real-library-lacks
level:  guard   since:2026-09-13   recur:1   saves:0   miss:1
tags:   op:test-design, op:verify, op:mutation-proof, type:fake-pins-a-nonexistent-library-api,
        scope:portable
trigger:about to write, extend, or rely on a hand-written fake/stub standing in for a THIRD-PARTY
        library, OR about to carry a decision's open question forward as "pinned by the
        fake/pystub until the real dependency is bundled"
rule:   a fake may only define attributes, methods, and SIGNATURES that the real library actually
        has. Whenever a fake stands in for a real dependency, ship a companion CONTRACT test that
        asserts the real installed library's surface — every attribute the adapter reads, and the
        parameter ORDER of every method it calls — skipped (never failed) when the dependency is
        absent, so dependency-less CI stays green while a machine that HAS the library catches the
        drift. Never let "pinned by the fake until the wheel is bundled" stand as a prose promise
        with no executable check behind it.
check:  for each attribute/method your code reads off a faked dependency, can you name the test
        that would FAIL if the real library never had it? `hasattr` is not enough for calls — an
        argument-ORDER defect passes every existence check, so assert `inspect.signature`.
why:    `src/Python/garminconnect/garmin_client.py` read `self._garmin.full_name_id` at two call
        sites. The real python-garminconnect 0.3.15 `Garmin` object has no such attribute — it has
        `display_name` / `full_name`. The fakes in `tests/test_adapter_login.py` and
        `tests/test_adapter_mfa.py` DEFINED `full_name_id`, so the entire suite agreed with the
        defect and stayed green through every gate. It shipped into a real qmake release build and
        cost FOUR live-account connect attempts (B-STAGE9-09): the login actually SUCCEEDED every
        time, then the adapter raised a bare `AttributeError` from a line sitting OUTSIDE its own
        exception-classification block, which folded to the generic "code: unknown" UI copy and
        made the real cause invisible. Worse, the same audit then found REQ-003's whole two-step
        MFA seam dead for the same reason (B-STAGE9-10): `Garmin(email, password)` never sets
        `return_on_mfa`, so the library can never return the `("needs_mfa", …)` sentinel the
        adapter checks for; and `resume_login(code, pending)` has its arguments REVERSED against
        the real `resume_login(client_state, mfa_code)`, submitting the pending-state object as
        the verification code and discarding the user's OTP.
        THE SHARPEST PART: A3-REQ-002's mutation testing (`archive/a3-req-002.md`) recorded
        mutants M8 and M9 — which swapped `full_name_id` ↔ `display_name`, i.e. the exact
        correction — as KILLED by the happy-path identity assertion. The mutation score did not
        merely miss the bug, it actively CERTIFIED it, because the assertion was checking the fake
        against itself. A mutation score computed entirely against fakes measures fake-fidelity,
        not real-library fidelity, and must never be cited as evidence about a real dependency's
        contract.
        Second-order: this is also why the raw-exception escape mattered more than the typo. Any
        library-shape mismatch outside the classifier is undiagnosable from the UI BY
        CONSTRUCTION — the diagnostic that finally cracked it (B-STAGE9-08's `exceptionType`
        logging, `garmin_auth_unknown exception_type=builtins.AttributeError`) had been built the
        same day for an unrelated reason and paid for itself on first live use.
origin: B-STAGE9-09 + B-STAGE9-10 (`findings.md`, Stage 9 live-account connect re-test attempt #4,
        2026-09-13); DEC-014 OQ1, which had carried this exact risk in writing as a non-blocking
        NOTE since the design phase and came due as a user-visible blocker.
history:2026-09-13: captured at guard, first occurrence, recur:1 saves:0 miss:1 (miss — DEC-014
        OQ1 named the risk explicitly and no executable check was ever attached to it).
