# Workflow Ledger Index

This project uses the `quality-gated-dev-workflow` skill, which keeps a per-feature
ledger directory. **The skill's default lookup ("read `state.md` first") assumes a
single ledger — so always start from the ACTIVE ledger named below, not the bare
`.claude/workflow/` path (which no longer exists).**

## Active ledger
- **`.claude/workflow-garminconnect/`** — Garmin Connect integration
  (python-garminconnect). Current phase and next gate live in its `state.md`.
  This is the ledger a new session should read first.

## Historical / shipped ledgers (do NOT rebuild `state.md` from these)
- **`.claude/workflow-aicoach/`** — AI Coach tool-use feature. Shipped
  (`src/Coach/`, TEST-001…020). Kept for provenance only. Uses the older flat
  `cycles/` + `validations/` layout and has no `state.md` by design — it is a
  closed record, not an active workspace.

## Convention going forward
- One ledger per feature: `.claude/workflow-<feature>/`.
- When a feature ships, note it here under "Historical" so a future session does
  not mistake a closed ledger for the active one.
