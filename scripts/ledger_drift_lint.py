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

"Paired" is narrower than "co-occurring": the status must be in ASSIGNMENT SHAPE (ORCH-010,
see _is_adjectival) and must share a BINDING SCOPE with the id (ORCH-015, see _scope_map) —
an id cited incidentally in a neighbouring sentence or a different quotation is not a pairing.

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

def _id_occurrences(line):
    """Return [(id_token, id_class, start_offset)] for every id-token OCCURRENCE."""
    out = []
    for rx, cls in ID_PATTERNS:
        for m in rx.finditer(line):
            out.append((m.group(0), cls, m.start()))
    out.sort(key=lambda t: t[2])
    return out


def find_ids(line):
    """Return list of (id_token, id_class) for every distinct id token on the line."""
    found = {}
    for tok, cls, _start in _id_occurrences(line):
        found.setdefault(tok, cls)
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


def _status_occurrences(line):
    """Return [(status_token, start_offset)] for every ASSIGNMENT-SHAPED occurrence.

    Occurrences that are merely adjectival (see _is_adjectival) are prose and are dropped."""
    out = []
    for tok, rx in STATUS_PATTERNS:
        for m in rx.finditer(line):
            if not _is_adjectival(line, m.end()):
                out.append((tok, m.start()))
    out.sort(key=lambda t: t[1])
    return out


def find_statuses(line):
    """Return the distinct status tokens ASSIGNED on the line (in STATUS_TOKENS order).

    A token that appears only adjectivally (see _is_adjectival) is prose and is not returned.
    A token appearing more than once counts if ANY occurrence is in assignment shape."""
    assigned = {tok for tok, _start in _status_occurrences(line)}
    return [tok for tok, _rx in STATUS_PATTERNS if tok in assigned]


# --- Binding scope (ORCH-015, 2026-08-15).
#
# ORCH-010 fixed the SHAPE question ("is this word being used as a status at all?"). It left
# the BINDING question open: scan_file still flagged a line whenever SOME id and SOME
# assignment-shaped status appeared anywhere on it. Four naturally-occurring STATE.md lines
# showed the gap — the status was genuinely an assignment, but its subject was NOT the id;
# the id was an incidental citation that happened to land on the same wrapped line, e.g.
#     "... on `this`. **I deleted that line myself: 47/47 STILL GREEN.** The"
# (GREEN's subject is the TEST SUITE, one sentence away from the DEC-030 earlier on the line).
#
# The rule this encodes: an id and a status bind only when they are in the SAME BINDING SCOPE,
# where a scope is the pair (sentence, quotation-region) the token sits in.
#   (a) SENTENCE — a sentence-final punctuation mark (. ! ? ;) followed by whitespace or the end
#       of the line ends the scope. A status in the NEXT sentence has a different subject.
#   (b) QUOTATION — text inside a backtick code span or a quoted string is QUOTED MATERIAL: it
#       is being cited, not asserted. A status inside a quotation binds only ids inside that
#       SAME quotation. This is same-REGION matching, not a blanket amnesty for quoted text:
#       "`DEC-030 STILL GREEN`" (both inside one span) still flags, and so does an unquoted
#       assignment elsewhere on a line that also contains a quoted mention.
#
# EXCEPTION — a MARKDOWN TABLE ROW is ONE RECORD, so the whole row is a single scope and
# neither boundary applies inside it. A ledger row is precisely "this id | ... | this status",
# and the description cells in between routinely contain full sentences and code spans:
#     "| TEST-003 | ... page-side (in-flight disables Next). 9 tests, **GREEN**. |"
#     "| REQ-004  | ... | `_uncommitted_` |"
# Scoping inside such a row would blind the lint to the single most common drift shape there is
# (measured: 5 of the 49 real pairings in traceability.md are exactly this). Prose lines, where
# the incidental-citation problem actually occurs, are never table rows.
#
# Direction of failure, as with ORCH-010, is deliberately "flags too much": boundaries are only
# recognised in their unambiguous forms (a sentence break needs the trailing whitespace/EOL; an
# unbalanced quote character opens nothing), so ambiguity keeps a pairing INSIDE one scope.
SENTENCE_END_CHARS = frozenset(".!?;")

TABLE_ROW_PREFIX = "|"
BLOCKQUOTE_CHARS = "> \t"

# Quotation openers -> their closer. Straight double quote and backtick are self-closing;
# typographic quotes are directional. Single quotes/apostrophes are NOT here: "don't" would
# open a bogus region.
QUOTE_PAIRS = {'"': '"', "`": "`", "“": "”"}


def _quoted_spans(line):
    """Return [(start, end)] half-open spans of quoted/code material, outermost, non-nested.

    An opener with no closer on the line is not a quotation and opens nothing."""
    spans = []
    i, n = 0, len(line)
    while i < n:
        closer = QUOTE_PAIRS.get(line[i])
        if closer is not None:
            j = line.find(closer, i + 1)
            if j != -1:
                spans.append((i, j + 1))
                i = j + 1
                continue
        i += 1
    return spans


def _is_table_row(line):
    """True if the line is a markdown table row (optionally blockquoted): one record."""
    return line.lstrip(BLOCKQUOTE_CHARS).startswith(TABLE_ROW_PREFIX)


def _scope_map(line):
    """Return scope[i] = (sentence_index, quotation_region) for each character of `line`.

    Region 0 is the unquoted body of the line; regions 1..n are the successive quoted spans.
    Sentence breaks are only counted OUTSIDE quotations, so a quoted sentence break cannot
    split a pairing that the quotation itself already holds together.
    A table row is one record and therefore one scope (see the ORCH-015 note above)."""
    if _is_table_row(line):
        return [(0, 0)] * len(line)

    region = [0] * len(line)
    for k, (start, end) in enumerate(_quoted_spans(line), 1):
        for i in range(start, end):
            region[i] = k

    scope = []
    sentence = 0
    for i, ch in enumerate(line):
        scope.append((sentence, region[i]))
        if region[i] == 0 and ch in SENTENCE_END_CHARS:
            nxt = line[i + 1] if i + 1 < len(line) else ""
            if nxt == "" or nxt.isspace():
                sentence += 1
    return scope


# --- Parenthetical citation (ORCH-015b, 2026-08-15).
#
# ORCH-010 taught the lint the ATTRIBUTIVE position (a status word modifying a following noun,
# "deferred reaper"). It never handled the PREDICATE position, where the status word ends the
# phrase and so still reads as an assignment. Binding scope (above) does not reach that case
# either — the id and the status are in the same sentence AND the same quotation region:
#     "the reaper (DEC-031) is frame-counted and deferred."      <- prose, must NOT fire
#     "the store leak (DEC-025) is deferred."                     <- prose, must NOT fire
#     "DEC-031 was ACCEPTED and its slice is GREEN"               <- drift, MUST fire
#
# All three are predicate constructions, so the status side cannot separate them. The ID's
# ROLE can: in the first two the id is in PARENTHESES. A parenthesised id is a CITATION —
# an aside naming the decision that governs the neighbouring noun phrase — and a citation is
# never the grammatical subject. In the third the id is bare and heads the clause.
#
# The rule is DIRECTIONAL, and the direction is the point:
#   · an id INSIDE an aside does NOT bind a status outside it — the id is a citation;
#   · a status INSIDE an aside DOES bind an id outside it — a parenthetical predicates about
#     the phrase it is attached to, which is exactly the common drift shape "REQ-019 (DEFERRED)".
# Inside one aside, tokens bind each other normally ("(DEC-031 DEFERRED)" fires).
#
# HONEST LIMIT: this closes the case where the id is a parenthetical citation, NOT the general
# predicate problem. "the reaper for DEC-031 is deferred." and "DEC-031's reaper is deferred."
# still fire, because separating a predicate's true subject from any other id in the same clause
# needs a parser, and a heuristic that guessed would start MISSING drift such as
# "DEC-031 is deferred." Erring toward flagging is the standing choice here (see ORCH-010).
def _paren_spans(line):
    """Return [(start, end)] half-open spans of OUTERMOST parenthetical asides.

    Nested parens are absorbed into the outermost span. An unclosed '(' opens no aside, so
    ambiguity keeps the tokens in one scope and the line keeps flagging."""
    spans = []
    depth = 0
    start = -1
    for i, ch in enumerate(line):
        if ch == "(":
            if depth == 0:
                start = i
            depth += 1
        elif ch == ")" and depth:
            depth -= 1
            if depth == 0:
                spans.append((start, i + 1))
    return spans


def _is_parenthetical_citation(id_at, status_at, spans):
    """True if the id sits in an aside that the status is outside of (id = mere citation)."""
    for start, end in spans:
        if start <= id_at < end:
            return not (start <= status_at < end)
    return False


def find_bound_pairs(line):
    """Return distinct [(id_token, id_class, status_token)] where the status BINDS the id.

    Binding requires BOTH:
      · the id occurrence and the assignment-shaped status occurrence share a binding scope
        (sentence + quotation region, or the whole row for a table row) — see _scope_map; and
      · the id is not a parenthetical citation relative to that status — see above.
    Any qualifying occurrence pair is enough."""
    ids = _id_occurrences(line)
    if not ids:
        return []
    statuses = _status_occurrences(line)
    if not statuses:
        return []

    scope = _scope_map(line)
    parens = _paren_spans(line)
    pairs, seen = [], set()
    for id_token, id_class, id_at in ids:
        for status_token, status_at in statuses:
            if scope[id_at] != scope[status_at]:
                continue
            if _is_parenthetical_citation(id_at, status_at, parens):
                continue
            key = (id_token, status_token)
            if key in seen:
                continue
            seen.add(key)
            pairs.append((id_token, id_class, status_token))
    return pairs


# ----------------------------- per-file scan -----------------------------

def scan_file(full_path, rel_path):
    """Yield Finding tuples for one in-scope file, honoring provenance-block exemptions.

    Only NON-canonical governance files reach here (canonical traceability.md/decisions.md are
    out of scope), so ANY bound id/status pairing on a line is a violation — no per-class home
    comparison is needed. The finding message still names the id-class's canonical home via
    CANONICAL_HOME so the fix is obvious ("move it to traceability.md")."""
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

        for id_token, id_class, status in find_bound_pairs(line):
            out.append(Finding(rel_path, lineno, id_token, status,
                               id_class, CANONICAL_HOME[id_class]))
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
