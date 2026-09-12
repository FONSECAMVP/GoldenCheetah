#!/usr/bin/env python3
"""T-208 / REQ-NF-i18n-001 static regression guard (Stage 8).

Both halves of prd.md:113 "All user-facing strings use tr() and appear in
.ts translation files." (verification method "Translation-file diff") made
executable and permanent, same pattern as T-203's sec-source guard:

  Rule I18N-TR-WRAP — every user-facing string literal in the Garmin scope
  (same CPP_GLOBS as garmin_sec_source_guard.py, imported so the two guards
  can never drift apart) must sit inside a tr()/translate() call. The
  extraction side of that requirement was landed by running lupdate over the
  scope and merging the Garmin contexts into the tracked gc_*.ts files; this
  rule keeps the SOURCE side from rotting afterwards. "User-facing" is a
  conservative prose heuristic: a literal containing two whitespace-separated
  word tokens. Two technical literal shapes are exempt (they carry spaces but
  are never shown to a user): date/time format patterns ("yyyy-MM-dd
  HH:mm:ss") and key=value developer-trace format strings (the
  "gc_obs op=%1 ..." qDebug lines from REQ-NF-Obs-001). Known limitation:
  single-word UI literals ("OK") are not caught — every such literal in the
  scope was verified tr()-wrapped when this guard landed.

  Rule I18N-TS-PRESENCE — every tr() literal extracted by the same pass must
  appear as a <source> in EVERY tracked src/Resources/translations/gc_*.ts.
  This is the diff-verification made repeatable: a new user-facing string
  fails the guard until its lupdate entries are merged into all languages
  (lrelease falls back to the English source for entries marked unfinished,
  so merging untranslated entries is safe and is exactly how the entries
  landed here).

Matching is comment-stripped and string-aware (garmin_sec_source_guard.py's
C++ state machine, which preserves string literals while blanking comments).
Adjacent string-literal concatenation across lines ("a" "b") is joined, since
that is what lupdate records as one <source>.

A rule match on a line carrying the marker  T208-ALLOW:<RULE-NAME>  is
skipped. Use it only with a comment explaining why; it keeps the escape
hatch visible to review instead of forcing the guard to be weakened.

Exit 0 = PASS, 1 = findings (each printed as RULE file:line: detail).
"""

from __future__ import annotations

import re
import sys
import xml.etree.ElementTree as ET
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))
from garmin_sec_source_guard import CPP_GLOBS, strip_cpp_comments  # noqa: E402

TS_GLOB = "src/Resources/translations/gc_*.ts"
ALLOW_MARKER = "T208-ALLOW:"

# Literal classes that are never user-facing despite containing whitespace.
DATE_FORMAT = re.compile(r"yyyy|HH:")
TRACE_KV = re.compile(r"=")
# Two word-ish tokens separated by whitespace => prose (=> user-facing).
PROSE = re.compile(r"[A-Za-z][A-Za-z0-9%_'.!\-—]*[ \t]+[A-Za-z][A-Za-z0-9%_'.!\-—]*")


def is_technical(literal: str) -> bool:
    """The documented whitespace-bearing non-UI literal shapes."""
    if DATE_FORMAT.search(literal):
        return True
    tokens = literal.split()
    if TRACE_KV.search(literal) and all(TRACE_KV.search(t) for t in tokens):
        return True
    # Multi-placeholder developer-trace format lines (the REQ-NF-Obs-001
    # "gc_obs op=%1 outcome=%2 error_code=%3 duration_ms=%4" qDebug mirror):
    # two or more %N placeholders AND two or more '=' never occurs in prose.
    return literal.count("%") >= 2 and literal.count("=") >= 2


def c_unescape(literal: str) -> str:
    """Undo the C++ escapes lupdate also undoes when writing <source>."""
    return (
        literal.replace("\\n", "\n")
        .replace("\\t", "\t")
        .replace('\\"', '"')
        .replace("\\\\", "\\")
    )


def extract_literals(code_text: str) -> list[tuple[int, str, bool, int]]:
    """One pass over comment-stripped C++ source.

    Returns (line, joined-literal, wrapped-in-tr, start-line-of-group) for
    every string literal group (adjacent literals concatenated). `wrapped`
    is true when the nearest enclosing '(' belongs to a *tr / *translate
    call, which covers tr(), QObject::tr(), QCoreApplication::translate()
    and QT_TRANSLATE_NOOP() alike.
    """
    findings: list[tuple[int, str, bool, int]] = []
    paren_stack: list[tuple[bool, int]] = []  # (is-tr call?, line opened)
    i, n, line = 0, len(code_text), 1
    while i < n:
        c = code_text[i]
        if c == "\n":
            line += 1
            i += 1
            continue
        if c == "/" and i + 1 < n and code_text[i + 1] == "/":  # pragma: no cover
            i = code_text.find("\n", i)
            i = n if i < 0 else i
            continue
        if c == '"':
            group_line, chunks = line, []
            while True:  # consume the literal, then any adjacent ones
                i += 1
                start = i
                while code_text[i] != '"':
                    i += 2 if code_text[i] == "\\" else 1
                chunks.append(code_text[start:i])
                i += 1
                j = i
                newlines = 0
                while j < n and code_text[j] in " \t\n":
                    if code_text[j] == "\n":
                        newlines += 1
                    j += 1
                if j < n and code_text[j] == '"':
                    # adjacent literal continues the group on a later line:
                    # account for the skipped newlines (group_line stays put,
                    # but the counter must not lag behind for the rest of the
                    # file — the T-208 RED run caught exactly that drift).
                    i, line = j, line + newlines
                    continue
                break
            wrapped = bool(paren_stack) and paren_stack[-1][0]
            findings.append(
                (group_line, c_unescape("".join(chunks)), wrapped, group_line)
            )
            continue
        if c == "(":
            # identifier immediately before '(' decides tr-ness of this call
            k = i - 1
            while k >= 0 and (code_text[k].isalnum() or code_text[k] == "_"):
                k -= 1
            name = code_text[k + 1 : i]
            paren_stack.append(
                (name.endswith("tr") or name.endswith("translate"), line)
            )
            i += 1
            continue
        if c == ")":
            if paren_stack:
                paren_stack.pop()
            i += 1
            continue
        i += 1
    return findings


def is_user_facing(literal: str) -> bool:
    """Prose heuristic with the documented technical exemptions."""
    return PROSE.search(literal) is not None and not is_technical(literal)


def main(argv: list[str]) -> int:
    if len(argv) != 2:
        print(f"usage: {Path(argv[0]).name} <source-root>", file=sys.stderr)
        return 2
    root = Path(argv[1]).resolve()

    files = sorted(
        {p for pattern in CPP_GLOBS for p in root.glob(pattern) if p.is_file()}
    )
    wrap_findings: list[str] = []
    wrapped_literals: set[str] = set()

    for path in files:
        rel = path.relative_to(root)
        raw_text = path.read_text(encoding="utf-8", errors="replace")
        raw_lines = raw_text.splitlines()
        code_map = strip_cpp_comments(raw_text)
        # strip_cpp_comments returns per-line text; rejoin so multi-line
        # tr("a"\n"b") groups and paren stacks scan as one document.
        code_text = "\n".join(code_map[ln] for ln in sorted(code_map))
        for lineno, literal, wrapped, group_line in extract_literals(code_text):
            if not is_user_facing(literal):
                continue
            if wrapped:
                wrapped_literals.add(literal)
                continue
            # The allow marker lives in a COMMENT, which strip_cpp_comments
            # has blanked to spaces — so match it against the raw source
            # lines. Accepted on the finding's line or the one directly above
            # (a long statement cannot always host a trailing marker under
            # the C++ ColumnLimit, so its justification comment sits above).
            # garmin_sec_source_guard.py checks its stripped line and can
            # therefore never see its own T203-ALLOW: hatch; flagged to the
            # Inspector rather than silently copied here.
            window = raw_lines[max(0, lineno - 2) : lineno]
            if any(ALLOW_MARKER + "I18N-TR-WRAP" in ln for ln in window):
                continue
            wrap_findings.append(f"I18N-TR-WRAP {rel}:{lineno}: {literal[:100]!r}")

    ts_findings: list[str] = []
    ts_paths = sorted(root.glob(TS_GLOB))
    for ts in ts_paths:
        tree = ET.parse(str(ts))
        sources = set()
        for m in tree.getroot().iter("message"):
            src = m.find("source")
            if src is not None and src.text is not None:
                sources.add(src.text)
        missing = sorted(s for s in wrapped_literals if s not in sources)
        if missing:
            examples = "; ".join(repr(s[:60]) for s in missing[:5])
            rel = ts.relative_to(root)
            ts_findings.append(
                f"I18N-TS-PRESENCE {rel}: {len(missing)}/{len(wrapped_literals)} "
                f"tr() literals absent (e.g. {examples})"
            )

    findings = wrap_findings + ts_findings
    if findings:
        print(f"garmin-i18n-source-guard: FAIL — {len(findings)} finding(s)")
        for f in findings:
            print(f"  {f}")
        return 1

    print(
        f"garmin-i18n-source-guard: PASS — {len(files)} files scanned, "
        f"{len(wrapped_literals)} tr() literals present in {len(ts_paths)} .ts files, 0 findings"
    )
    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv))
