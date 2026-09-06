# Workflow Ledger Index

> **Migrated to the wiki-memory edition (2026-07-04).** The project brain now lives at
> the repo root: read `WIKI.md` first, then `STATE.md` (the condensed project cursor).
> This index remains the authority on which per-feature ledger is active vs closed.

This project uses the `quality-gated-dev-workflow` skill, which keeps a per-feature
ledger directory. **DEC-015 removed the per-ledger `state.md`: the SOLE live cursor is
the root `STATE.md`. A session reads `WIKI.md` → root `STATE.md`, and this index only to
learn which per-feature ledger is ACTIVE** (its `traceability.md`/`decisions.md`/`design.md`
hold that ledger's spine).

## Active ledger
- **`.claude/workflow-garminconnect/`** — Garmin Connect integration
  (python-garminconnect). Current phase and next gate live in the root `STATE.md`
  (per-id status → this ledger's `traceability.md`). This is the active ledger.
  **Layout note (2026-08-30):** `cycles/` and `validations/` now contain ONLY an
  `archive/` subdirectory. The `active/` directories were removed because every run
  in them had closed — 3 REQ-002 cycles (2026-05/07) and VAL-004…012 (2026-05…07).
  Each archived file carries a HISTORICAL SNAPSHOT header; **their original verdicts
  are unedited**. A future in-flight run re-creates `active/` for that run only.
  "Which user journeys actually work" is answered by `traceability.md`'s
  `## Phase-1 capability matrix`, not by this index and not by `STATE.md`.

## Historical / shipped ledgers (provenance only)
- **`.claude/workflow-aicoach/`** — AI Coach tool-use feature. Shipped
  (`src/Coach/`, TEST-001…020). Kept for provenance only. Uses the older flat
  `cycles/` + `validations/` layout — a closed record, not an active workspace.

## Convention going forward
- One ledger per feature: `.claude/workflow-<feature>/`.
- When a feature ships, note it here under "Historical" so a future session does
  not mistake a closed ledger for the active one.
