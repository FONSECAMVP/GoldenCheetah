# Lessons Memory — GoldenCheetah            (index first, cold entries below)

LSN-001 | op:create-file type:duplication            | guard    | recur:n/a saves:0 miss:0 | check WIKI MAP before mkdir/touch/write — enforced by mechanism: .claude/hooks/anti_duplication_guard.py
LSN-002 | op:id-alloc type:cross-ledger-collision     | guard    | recur:2  saves:0 miss:0  | never merge ID ranges across ledgers; qualify with ledger prefix (garmin:/coach:)
LSN-003 | op:prd type:underspecified-nfr              | guard    | recur:11 saves:0 miss:0  | every NF-* requirement needs a quantified acceptance value before it enters an A1 cycle
LSN-004 | op:design type:missing-seam                 | guard    | recur:1  saves:0 miss:0  | any call into a mutable third-party library needs its adapter/interface seam defined before first implementation, not retrofitted after review
LSN-005 | op:design type:security-invariant-on-read   | guard    | recur:1  saves:0 miss:0  | security invariants enforced on write (perms, format) must also be validated on read, not assumed
LSN-006 | op:code type:error-handling                 | advisory | recur:1  saves:0 miss:0  | exception handlers at adapter/boundary layers must classify by type before a broad except, never swallow-and-misroute
LSN-007 | op:commit type:hook-mutation-unverified      | guard    | recur:1  saves:0 miss:0  | if a pre-commit hook modifies files, all prior build/test evidence is void — rebuild + re-run affected tests before accepting the commit; protect semantic include order with clang-format off markers

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
