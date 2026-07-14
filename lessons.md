# Lessons Memory — GoldenCheetah            (index first, cold entries below)

LSN-001 | op:create-file type:duplication            | guard    | recur:n/a saves:0 miss:0 | check WIKI MAP before mkdir/touch/write — enforced by mechanism: .claude/hooks/anti_duplication_guard.py
LSN-002 | op:id-alloc type:cross-ledger-collision     | guard    | recur:2  saves:0 miss:0  | never merge ID ranges across ledgers; qualify with ledger prefix (garmin:/coach:)
LSN-003 | op:prd type:underspecified-nfr              | guard    | recur:11 saves:0 miss:0  | every NF-* requirement needs a quantified acceptance value before it enters an A1 cycle
LSN-004 | op:design type:missing-seam                 | guard    | recur:1  saves:0 miss:0  | any call into a mutable third-party library needs its adapter/interface seam defined before first implementation, not retrofitted after review
LSN-005 | op:design type:security-invariant-on-read   | guard    | recur:1  saves:0 miss:0  | security invariants enforced on write (perms, format) must also be validated on read, not assumed
LSN-006 | op:code type:error-handling                 | advisory | recur:1  saves:0 miss:0  | exception handlers at adapter/boundary layers must classify by type before a broad except, never swallow-and-misroute
LSN-007 | op:commit type:hook-mutation-unverified      | guard    | recur:1  saves:0 miss:0  | if a pre-commit hook modifies files, all prior build/test evidence is void — rebuild + re-run affected tests before accepting the commit; protect semantic include order with clang-format off markers
LSN-008 | op:ledger-update type:index-vs-detail-drift  | MECHANISM | recur:7  saves:5 miss:6  | **PROMOTED TO MECHANISM 2026-07-13 (DEC-015 → `.claude/hooks/ledger_drift_lint.py`, TEST-017).** Root cause retired by SSOT: per-id lifecycle status is now single-homed in `traceability.md` (REQ/DES/TEST/VAL) + `decisions.md` (DEC) and STRIPPED from the non-canonical set (STATE.md/WIKI.md/wiki/*/design.md); the per-ledger `state.md` was deleted (one cursor). The absence-check lint (pre-commit + CLV) deterministically fails any id+status pairing outside a canonical home — the cross-file duplication class that drove all 7 recurrences can no longer occur. **Residual (NOT covered by the lint, stays a CLV concern):** drift INSIDE the canonical files themselves and in pointer/index files the lint doesn't scan — see [[LSN-015]]. Design principle: [[LSN-014]]. Sibling: [[LSN-011]] (design-note false-done, subsumed by the design.md status-strip).
LSN-011 | op:ledger-update type:design-note-false-done | guard    | recur:1  saves:1 miss:0  | any DEC-cascade item written into a design.md "DEC-NNN refinement" note that is still open/deferred in findings.md MUST be in TARGET/DEFERRED tense, never present-tense "already true" — grep DEC-refinement notes for completion verbs (no longer/now/without/stops) and confirm each is not an open findings.md defer row, before the CLV gate
LSN-009 | op:test type:loose-timeout-bound-survivor   | advisory | recur:1  saves:1 miss:0  | a bounded-teardown/timeout assertion must be tight enough to FAIL if the graceful fast-path is skipped (assert « the fast-path ceiling, not < the sum of all fallback ceilings) — a loose bound cannot distinguish "worked" from "fell through to the last resort every time"
LSN-010 | op:commit type:commit-column-staleness      | advisory | recur:1  saves:1 miss:0  | a feature commit that bundles its OWN ledger byproduct necessarily records "uncommitted"/`_pending_` (the hash doesn't exist yet); it MUST be followed immediately by a ledger-record step that fills the traceability Commit column with the just-created hash and flips uncommitted→committed banners, BEFORE the CLV gate. Mechanically checkable: `git log -1` HEAD hash vs the REQ row's Commit-column string.
LSN-012 | op:verify type:builder-lint-dirty-green      | advisory | recur:1  saves:1 miss:0  | a builder GREEN report is not verified until the DEC-009 style gate (ruff/clang-format/mypy) has run on the changeset — the orchestrator Evidence check runs it BEFORE commit, not deferred to the commit hook. A builder with the tooling available (ran pytest via the repo .venv) can still leave lint-dirty code (dead locals, F841) a passing test suite won't surface.
LSN-013 | op:test type:mask-coverage-gap              | advisory | recur:1  saves:1 miss:0  | a test guarding a security predicate that ORs multiple bit/flag classes (file perms Read/Write/Exec × Group/Other; capability/permission bitmasks) must exercise ≥1 case per bit class NOT already covered by another — two Read-class widened modes (0640/0644) do NOT pin a Read|Write|Exec mask; a mutant dropping the Write*/Exec* bits survives. One case per uncovered class (e.g. 0620 write-only, 0601 exec-only) kills it.
LSN-014 | op:ledger-design type:status-duplication   | guard scope:portable | recur:n/a saves:0 miss:0 | ONE logical fact = ONE storage location. A per-id lifecycle status lives in exactly one canonical file and is never restated in cursors/design/wiki; and a project has exactly ONE live cursor (no per-ledger cursor + root cursor duplicate). "Condensed copy" and "full copy" of the same status are two things that drift — collapse them. Enforce absence elsewhere with a deterministic lint; keep design docs in INTENT tense (target shape), not live status. This is the SSOT design that retired [[LSN-008]]; see also [[LSN-015]] for verifying the migration that establishes it.
LSN-015 | op:cascade type:named-target-unverified    | guard scope:portable | recur:1  saves:0 miss:1  | when a DEC/cascade NAMES specific files to edit or repoint, grep-verify EACH named target was actually changed before declaring the cascade done — never infer completeness from the changeset file-list (a path-based grep also misses RELATIVE references, e.g. a pointer to `state.md` rather than `.claude/workflow-x/state.md`). Corollary: a deterministic lint scoped to a NON-canonical set proves nothing about content INSIDE the canonical files or in pointer/index files it doesn't scan — those need an explicit CLV check. Both misses (dangling `state.md` pointers in conventions.md/workflow-INDEX.md + an un-dropped DEC-index Status column) were caught by the DEC-015 closing CLV, not the lint — which is exactly why the CLV gate exists alongside the mechanism.

---

## LSN-001
sig:    create-file / duplication / any-path
level:  guard      since:P0(bootstrap)   recur:n/a   saves:0   miss:0
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
