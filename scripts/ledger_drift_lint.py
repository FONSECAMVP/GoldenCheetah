#!/usr/bin/env python3
"""
ledger_drift_lint.py — deterministic "ledger status drift" lint for the
quality-gated-dev-workflow skill.

Purpose (retires lesson LSN-008): lifecycle status for a project-id used to be duplicated
across ~7 governance files and kept drifting out of sync. The migration collapses status
prose into a single canonical home per id-CLASS; this lint is the ABSENCE backstop that
guards against the prose reappearing. The canonical ledgers (traceability.md, decisions.md)
are the SOURCE OF TRUTH and are fully EXEMPT — they legitimately co-locate ids with status
words (a REQ row citing its governing DEC/DES/TEST; decisions.md cascade-notes). The lint
scans only the NON-canonical governance set and fails (nonzero exit) whenever a project-id
token is paired, ON THE SAME LINE, with a lifecycle-status token in one of those files.

It is deliberately HIGH-PRECISION (few false positives): the status-token list is short and
high-signal, provenance/appendix blocks are exempt, and archival/quote files are skipped.

CLI:
    python3 ledger_drift_lint.py [root]      # root defaults to cwd
    exit 0 = no drift
    exit 1 = at least one violation (one finding line per co-occurrence on stdout)
    exit 2 = usage / IO error

Finding line format:
    PATH:LINENO: <id> paired with status '<token>' — status for <ID-CLASS> belongs only in <canonical file>

Importable API (for the CLV step / install_hook wiring):
    scan(root) -> list[Finding]
    format_finding(Finding) -> str
    main(argv=None) -> int

Stdlib only; no third-party dependencies (matches sibling anti_duplication_guard.py).
"""

import collections
import os
import re
import sys

# ============================ EXTENSIBLE CONSTANTS ============================
# Everything a maintainer might tune lives here at the top, as plain data.

# --- The invariant: the ONLY file (by basename) where each id-CLASS may pair an id with a
#     status token. A pairing anywhere else — including a FOREIGN class in a file that is some
#     OTHER class's home (e.g. DEC in traceability.md) — is a violation.
CANONICAL_HOME = {
    "REQ": "traceability.md",
    "DES": "traceability.md",
    "TEST": "traceability.md",
    "VAL": "traceability.md",
    "DEC": "decisions.md",
}

# --- Project-id tokens (case-sensitive on the prefix). Order matters only so a more specific
#     pattern (REQ-NF-...) is listed before its shorter sibling; both TEST-### and T-### map
#     to id-CLASS "TEST". \b anchors keep T-### from matching inside TEST-### and keep ids
#     from matching mid-word.
ID_PATTERNS = [
    (re.compile(r"\bREQ-NF-[A-Za-z]+-\d{3}"), "REQ"),
    (re.compile(r"\bREQ-\d{3}"), "REQ"),
    (re.compile(r"\bDEC-\d{3}"), "DEC"),
    (re.compile(r"\bDES-\d{3}[a-z]?"), "DES"),
    (re.compile(r"\bVAL-\d{3}"), "VAL"),
    (re.compile(r"\bTEST-\d{3}"), "TEST"),
    (re.compile(r"\bT-\d{3}"), "TEST"),
]

# --- Lifecycle-status tokens (module constant; deliberately high-precision).
#     Case-sensitive: both "deferred" and "DEFERRED" are listed intentionally.
#     DELIBERATELY EXCLUDES the word "committed": it is entangled with commit-hash provenance
#     (e.g. "committed `3edb705cb`") and would generate false positives. The high-signal
#     tokens below are a sufficient backstop without it.
#     "in progress" was ALSO dropped (DEC-015 migration, 2026-07-12): not in the controlled
#     vocabulary (drafted → GREEN → committed → CLOSED / deferred) and it collided with a spec'd
#     domain message ("sync already in progress", the REQ-NF-Perf-002 rejection text quoted in
#     design.md). Dropping it keeps precision without losing real LSN-008 drift coverage.
STATUS_TOKENS = [
    "GREEN",
    "CLOSED",
    "DEFERRED",
    "deferred",
    "drafted",
    "in build",
    "_pending_",
    "_uncommitted_",
]

# --- Scope / exclusion configuration.
SCOPE_ROOT_FILES = ("STATE.md", "WIKI.md")           # only at the scanned root
SCOPE_WIKI_DIR = "wiki"                                # wiki/*.md
SCOPE_LEDGER_DIR_PREFIX = "workflow-"                  # .claude/workflow-*/...
SCOPE_LEDGER_PARENT = ".claude"
# NOTE: traceability.md and decisions.md are the canonical SOURCE OF TRUTH and are
# DELIBERATELY NOT SCANNED. Those matrices/cascade-notes legitimately co-locate ids with
# status words (a REQ row citing its governing DEC/DES/TEST alongside the REQ's status;
# decisions.md cascade-notes referencing the REQ/DES a decision affects). That co-location
# is the ledger doing its job, not drift. We enforce status-absence only in the NON-canonical
# governance set below. CANONICAL_HOME is still used to phrase the finding message.
SCOPE_LEDGER_FILES = ("design.md",)

EXCLUDED_DIR_NAMES = ("archive", "cycles", "validations")   # at any depth
EXCLUDED_FILE_NAMES = ("lessons.md", "findings.md")         # they quote history by design

# A line containing this literal marker opens an exempt block that runs until (and excluding)
# the next line beginning with a level-2 heading ("## "). Used by the per-slice "artifacts"
# appendix tables in traceability.md, which legitimately quote dated historical statuses.
PROVENANCE_MARKER = "<!-- provenance: dated"
HEADING2_PREFIX = "## "

# =============================================================================

Finding = collections.namedtuple(
    "Finding", "path lineno id_token status_token id_class canonical")


# Precompile the status matchers once. Alphabetic tokens are word-boundary anchored so
# "GREEN" won't hit "EVERGREEN"; multi-word / underscore literals are matched as escaped
# substrings (their surrounding spaces/underscores already make them high-precision).
def _status_regex(token):
    if re.fullmatch(r"[A-Za-z]+", token):
        return re.compile(r"\b" + re.escape(token) + r"\b")
    return re.compile(re.escape(token))


STATUS_PATTERNS = [(tok, _status_regex(tok)) for tok in STATUS_TOKENS]


# ----------------------------- scope / exclusion -----------------------------

def is_excluded(parts):
    """parts = path split relative to root. True if this file must never be flagged."""
    if parts[-1] in EXCLUDED_FILE_NAMES:
        return True
    for d in parts[:-1]:
        if d in EXCLUDED_DIR_NAMES:
            return True
    return False


def in_scope(parts):
    """True if this file's basename+location is one of the governance files we scan."""
    name = parts[-1]
    parent = parts[-2] if len(parts) >= 2 else ""
    grandparent = parts[-3] if len(parts) >= 3 else ""
    # root STATE.md / WIKI.md
    if len(parts) == 1 and name in SCOPE_ROOT_FILES:
        return True
    # wiki/*.md
    if name.endswith(".md") and parent == SCOPE_WIKI_DIR:
        return True
    # .claude/workflow-*/{traceability,decisions,design}.md
    if (name in SCOPE_LEDGER_FILES
            and parent.startswith(SCOPE_LEDGER_DIR_PREFIX)
            and grandparent == SCOPE_LEDGER_PARENT):
        return True
    return False


# ----------------------------- token detection -----------------------------

def find_ids(line):
    """Return list of (id_token, id_class) for every distinct id token on the line."""
    found = {}
    for rx, cls in ID_PATTERNS:
        for m in rx.finditer(line):
            found.setdefault(m.group(0), cls)
    return list(found.items())


# --- Assignment-shape discrimination (ORCH-010, 2026-08-12).
#
# The original rule was pure CO-OCCURRENCE: any id token plus any status token on one line was
# a violation. That is the same error [[LSN-034]] warns about — co-occurrence asserted as
# membership — and it made ordinary English unusable in the governance files. Two tokens had
# already been DELETED to buy precision ("committed", "in progress"); deleting a third
# ("deferred") would have cost a real vocabulary word. Detecting the SHAPE is the right fix.
#
# The discriminator: a status word immediately followed by a lowercase noun is being used as an
# ADJECTIVE and is prose, not an assignment —
#     "frame-counted deferred reaper"   "deferred-deletion semantics"   "drafted proposal"
# whereas a status word that ends the phrase, hits punctuation/a table cell, or is followed by a
# function word is an assignment —
#     "| REQ-007 | GREEN |"   "DES-013 — DEFERRED"   "REQ-019 DEFERRED to REQ-021"
#
# Deliberately CONSERVATIVE: it only ever suppresses when the very next thing is a lowercase
# non-function word. Anything else (punctuation, end of line, a capitalised word, a digit) still
# flags, so the failure mode stays "flags too much", never "misses drift".
STATUS_FOLLOWERS = frozenset((
    "to", "in", "on", "at", "by", "for", "from", "until", "pending", "because", "since",
    "as", "and", "or", "after", "before", "when", "while", "with", "per", "via", "than",
))

# A hyphen or run of spaces, then a lowercase word: the adjectival shape.
_FOLLOWER_RE = re.compile(r"(?:-|\s+)([a-z][a-zA-Z]*)")


def _is_adjectival(line, end):
    """True if the status token ending at `end` is modifying a following lowercase noun."""
    m = _FOLLOWER_RE.match(line, end)
    if not m:
        return False                      # punctuation / EOL / capitalised → assignment shape
    return m.group(1) not in STATUS_FOLLOWERS


def find_statuses(line):
    """Return the distinct status tokens ASSIGNED on the line (in STATUS_TOKENS order).

    A token that appears only adjectivally (see _is_adjectival) is prose and is not returned.
    A token appearing more than once counts if ANY occurrence is in assignment shape."""
    out = []
    for tok, rx in STATUS_PATTERNS:
        for m in rx.finditer(line):
            if not _is_adjectival(line, m.end()):
                out.append(tok)
                break
    return out


# ----------------------------- per-file scan -----------------------------

def scan_file(full_path, rel_path):
    """Yield Finding tuples for one in-scope file, honoring provenance-block exemptions.

    Only NON-canonical governance files reach here (canonical traceability.md/decisions.md are
    out of scope), so ANY id-token + status-token co-occurrence on a line is a violation — no
    per-class home comparison is needed. The finding message still names the id-class's
    canonical home via CANONICAL_HOME so the fix is obvious ("move it to traceability.md")."""
    out = []
    try:
        with open(full_path, "r", encoding="utf-8", errors="replace") as fh:
            lines = fh.readlines()
    except OSError:
        return out  # unreadable file: skip rather than abort the whole run

    in_provenance = False
    for lineno, raw in enumerate(lines, 1):
        line = raw.rstrip("\n")

        if in_provenance:
            # Block runs until (and EXCLUDING) the next level-2 heading, which is processed.
            if line.startswith(HEADING2_PREFIX):
                in_provenance = False
            else:
                continue
        # The marker line itself opens the block and is skipped.
        if PROVENANCE_MARKER in line:
            in_provenance = True
            continue

        ids = find_ids(line)
        if not ids:
            continue
        statuses = find_statuses(line)
        if not statuses:
            continue

        for id_token, id_class in ids:
            canonical = CANONICAL_HOME[id_class]
            for status in statuses:
                out.append(Finding(rel_path, lineno, id_token, status,
                                   id_class, canonical))
    return out


def scan(root):
    """Walk `root`, scan every in-scope governance file, and return findings sorted
    deterministically by (path, lineno, id_token, status_token)."""
    root = os.path.abspath(root)
    findings = []
    for dirpath, dirnames, filenames in os.walk(root):
        for fn in filenames:
            full = os.path.join(dirpath, fn)
            rel = os.path.relpath(full, root)
            parts = rel.split(os.sep)
            if is_excluded(parts):
                continue
            if not in_scope(parts):
                continue
            findings.extend(scan_file(full, rel))
    findings.sort(key=lambda f: (f.path, f.lineno, f.id_token, f.status_token))
    return findings


# ----------------------------- reporting / CLI -----------------------------

def format_finding(f):
    return (f"{f.path}:{f.lineno}: {f.id_token} paired with status "
            f"'{f.status_token}' — status for {f.id_class} belongs only in {f.canonical}")


def main(argv=None):
    if argv is None:
        argv = sys.argv[1:]
    if len(argv) > 1:
        print("usage: ledger_drift_lint.py [root]", file=sys.stderr)
        return 2
    root = argv[0] if argv else os.getcwd()
    if not os.path.isdir(root):
        print(f"error: not a directory: {root}", file=sys.stderr)
        return 2
    try:
        findings = scan(root)
    except OSError as e:
        print(f"error: {e}", file=sys.stderr)
        return 2

    for f in findings:
        print(format_finding(f))
    return 1 if findings else 0


if __name__ == "__main__":
    raise SystemExit(main())
