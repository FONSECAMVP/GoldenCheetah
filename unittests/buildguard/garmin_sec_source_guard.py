#!/usr/bin/env python3
"""T-203 / REQ-NF-Sec-001 + REQ-NF-Sec-003 static regression guard (Stage 8).

Makes the two manual grep audits from prd.md executable and permanent:

  REQ-NF-Sec-001 (prd.md:104) "Password never persisted; only tokens."
      dod.md:61 names the verification "password never written to any file
      (grep test)". This guard forbids a password identifier from appearing on
      the same (comment-stripped) line as a persistence API
      (QSettings::setValue, file write/open, AtomicFile::writeOver, json dump)
      and forbids the classic "remember password" settings key outright.

  REQ-NF-Sec-003 (prd.md:106) "verify=False is forbidden anywhere in the code
      path." garmin_client.py never passes verify at all — HTTPS validation is
      the curl_cffi/requests default — so the guard forbids every
      disable-verification idiom in the Garmin source tree: python
      (verify=False/0, CERT_NONE, _create_unverified_context,
      check_hostname=False) and Qt (VerifyNone, ignoreSslErrors).

Scope is the Garmin feature's production tree only (see SCOPE below), NOT the
unittests/ scaffolding: a violation here is a code-path regression, which is
what both prd rows govern. A test-fixture writing bytes to a temp dir is a
test-hygiene matter, not this invariant.

Matching is comment-stripped (tokenize for Python, a string-aware state
machine for C++) so prose like "verify=False is forbidden" in a docstring or
a "// never call setValue() with the password" comment does not false-positive.
String literals are preserved (the GARMIN_PASSWORD key rule needs them).
Known limitation: C++ raw strings R"(...)" are not modelled; the Garmin tree
uses none (checked when this guard landed).

A rule match on a line carrying the marker  T203-ALLOW:<RULE-NAME>  is skipped.
Use it only with a comment explaining why; it keeps the escape hatch visible
to review instead of forcing the guard to be weakened or deleted.

Exit 0 = PASS, 1 = findings (each printed as RULE file:line: <code line>).
"""

from __future__ import annotations

import ast
import io
import re
import sys
import tokenize
from pathlib import Path

# --- scope -------------------------------------------------------------------
# C++: the Garmin feature's own translation units, by explicit pattern.
CPP_GLOBS = [
    "src/Cloud/Garmin*.cpp",
    "src/Cloud/Garmin*.h",
    "src/Cloud/IGarmin*.h",
    "src/Cloud/PyEmbeddedAdapter.cpp",
    "src/Cloud/PyEmbeddedAdapter.h",
    # The wizard owns the first-connect UX where the password is entered and
    # the persist lambda runs; it is Garmin-shared, not Garmin-named.
    "src/Cloud/AddCloudWizard.cpp",
    "src/Cloud/AddCloudWizard.h",
]
# Python: the whole garmin module dir (garmin_client is the ONLY importer of
# garminconnect/curl_cffi per DES-012), recursively.
PY_ROOT = "src/Python/garminconnect"

# --- rules -------------------------------------------------------------------
# Sec-003: every known disable-verification idiom.
SEC3_RULES: dict[str, str] = {
    "SEC3-VERIFY-FALSE": r"verify\s*=\s*(?:False|0)\b",
    "SEC3-CERT-NONE": r"\bCERT_NONE\b",
    "SEC3-UNVERIFIED-CONTEXT": r"_create_unverified_context",
    "SEC3-CHECK-HOSTNAME-FALSE": r"check_hostname\s*=\s*False",
    "SEC3-QT-VERIFY-NONE": r"\bVerifyNone\b",
    "SEC3-QT-IGNORE-SSL-ERRORS": r"\bignoreSslErrors\b",
}

# Sec-001: password identifier touching a persistence API on one code line.
# 'password' is matched as a substring (catches m_password, PASSWORD_KEY);
# 'pwd' needs word boundaries ('passwd' is covered by the substring rule).
PASSWORD_ID = r"password|(?<!\w)pwd(?!\w)"
PERSIST_API = r"setValue\s*\(|\bwriteOver\s*\(|\bwrite\s*\(|\.write[a-z_]*\s*\(|\bopen\s*\(|\bdump\s*\("
# NOTE: both identifier sets carry top-level '|' alternations — the (?: ...)
# wrappers are load-bearing (a bare f"{PASSWORD_ID}(?=...)" would bind the
# lookahead to the last alternative only and let 'password' match anywhere).
SEC1_PERSIST_RULE = (
    "SEC1-PASSWORD-TO-PERSISTENCE",
    rf"(?:{PASSWORD_ID})(?=.*(?:{PERSIST_API}))|(?:{PERSIST_API})(?=.*(?:{PASSWORD_ID}))",
)
# Sec-001: the classic remember-password settings key, forbidden outright.
SEC1_KEY_RULE = ("SEC1-PASSWORD-SETTINGS-KEY", r"GARMIN_PASSWORD")

RULES: dict[str, str] = {**SEC3_RULES, SEC1_PERSIST_RULE[0]: SEC1_PERSIST_RULE[1], SEC1_KEY_RULE[0]: SEC1_KEY_RULE[1]}
ALLOW_MARKER = "T203-ALLOW:"


# --- comment stripping ---------------------------------------------------------
def strip_python_comments(text: str) -> dict[int, str]:
    """Return {1-based line -> code-only text} with comments removed.

    tokenize is used so strings (incl. docstrings and f-strings) survive
    verbatim while COMMENT tokens are cut at their exact start column. Every
    line starts in the result — only comment suffixes are cut — so a tokenizer
    failure below can never silently exclude code lines from the scan.
    """
    lines = text.splitlines()
    code: dict[int, str] = {i + 1: ln for i, ln in enumerate(lines)}
    try:
        for tok in tokenize.generate_tokens(io.StringIO(text).readline):
            if tok.type == tokenize.COMMENT:
                lineno, col = tok.start
                code[lineno] = code[lineno][:col]
    except tokenize.TokenError:
        # Unterminated construct: fall back to raw text rather than passing
        # silently — the guard must never fail-open on a file it can't parse.
        return code
    return code


def strip_cpp_comments(text: str) -> dict[int, str]:
    """Return {1-based line -> code-only text} for C/C++.

    A small state machine: line and block comments are blanked (spaces, to
    keep columns), string/char literals pass through with escape handling, and
    the common raw-string forms R"(delim)" are terminated correctly.
    """
    out: list[str] = []
    state = "code"  # code | line | block | str | char | raw
    delim = ""
    cur: list[str] = []
    i = 0
    n = len(text)
    while i < n:
        c = text[i]
        nxt = text[i + 1] if i + 1 < n else ""
        if c == "\n":
            out.append("".join(cur))
            cur = []
            if state == "line":
                state = "code"
            i += 1
            continue
        if state == "code":
            if c == "/" and nxt == "/":
                state = "line"
                cur.append("  ")
                i += 2
                continue
            if c == "/" and nxt == "*":
                state = "block"
                cur.append("  ")
                i += 2
                continue
            prev = cur[-1] if cur and cur[-1].strip() else (out[-1].rstrip()[-1:] if out and out[-1].rstrip() else "")
            if c == '"' and prev in ("R", "r"):
                # crude raw-string detect: scan delimiter up to '('
                j = i + 1
                d = ""
                while j < n and text[j] != "(" and text[j] != "\n" and len(d) < 16:
                    d += text[j]
                    j += 1
                if j < n and text[j] == "(":
                    state = "raw"
                    delim = d
                    cur.append(text[i : j + 1])
                    i = j + 1
                    continue
                state = "str"
                cur.append(c)
                i += 1
                continue
            if c == '"':
                state = "str"
                cur.append(c)
                i += 1
                continue
            if c == "'":
                state = "char"
                cur.append(c)
                i += 1
                continue
            cur.append(c)
            i += 1
        elif state in ("str", "char"):
            cur.append(c)
            if c == "\\":
                if nxt:
                    cur.append(nxt)
                    i += 2
                    continue
            elif (state == "str" and c == '"') or (state == "char" and c == "'"):
                state = "code"
            i += 1
        elif state == "raw":
            cur.append(c)
            if c == ")" and text.startswith(f"{delim}\"", i + 1):
                tail = f"{delim}\""
                cur.append(tail)
                i += len(tail) + 1
                state = "code"
                continue
            i += 1
        else:  # line / block comment
            if state == "block" and c == "*" and nxt == "/":
                cur.append("  ")
                i += 2
                state = "code"
                continue
            cur.append(" ")
            i += 1
    out.append("".join(cur))
    return {idx + 1: ln for idx, ln in enumerate(out)}


# --- scanning ------------------------------------------------------------------
def blank_docstrings(text: str, code: dict[int, str]) -> dict[int, str]:
    """Blank module/class/function docstring lines in the code map.

    Docstrings are prose, not code: a docstring saying "the C++ open() path
    with NO password" must not read as a persistence flow. Only docstring
    constants are blanked — ordinary string literals survive (the settings-key
    rule needs their content). An unparseable file is returned unblanked, which
    can only over-scan, never under-scan.
    """
    try:
        tree = ast.parse(text)
    except SyntaxError:
        return code
    for node in ast.walk(tree):
        if not isinstance(node, (ast.Module, ast.ClassDef, ast.FunctionDef, ast.AsyncFunctionDef)):
            continue
        body = node.body
        if not body:
            continue
        first = body[0]
        if isinstance(first, ast.Expr) and isinstance(first.value, ast.Constant) and isinstance(first.value.value, str):
            for ln in range(first.lineno, getattr(first, "end_lineno", first.lineno) + 1):
                code[ln] = ""
    return code


def in_scope_files(root: Path) -> list[Path]:
    files: list[Path] = []
    for pattern in CPP_GLOBS:
        files.extend(sorted(root.glob(pattern)))
    pyroot = root / PY_ROOT
    if pyroot.is_dir():
        # Skip virtualenvs/dot-dirs: .venv/site-packages is third-party code
        # vendored in-place, not this feature's production tree.
        files.extend(
            sorted(
                p
                for p in pyroot.rglob("*.py")
                if not any(part.startswith(".") or part == "__pycache__" for part in p.parts)
            )
        )
    return files


def main(argv: list[str]) -> int:
    if len(argv) != 2:
        print(f"usage: {Path(argv[0]).name} <source-root>", file=sys.stderr)
        return 2
    root = Path(argv[1]).resolve()
    findings: list[tuple[str, Path, int, str]] = []
    n_files = 0
    n_rules_applied = 0

    for path in in_scope_files(root):
        if not path.is_file():
            continue
        n_files += 1
        raw_lines = path.read_text(encoding="utf-8", errors="replace").splitlines()
        text = path.read_text(encoding="utf-8", errors="replace")
        code = strip_python_comments(text) if path.suffix == ".py" else strip_cpp_comments(text)
        if path.suffix == ".py":
            code = blank_docstrings(text, code)
        for lineno, codeline in sorted(code.items()):
            if ALLOW_MARKER in codeline:
                continue
            for rule, pattern in RULES.items():
                n_rules_applied += 1
                flags = re.IGNORECASE if rule == "SEC1-PASSWORD-TO-PERSISTENCE" else 0
                if re.search(pattern, codeline, flags):
                    findings.append((rule, path, lineno, codeline.strip()))

    if findings:
        print(f"garmin-sec-source-guard: FAIL — {len(findings)} finding(s)")
        for rule, path, lineno, line in findings:
            print(f"  {rule} {path.relative_to(root)}:{lineno}: {line[:160]}")
        return 1

    print(f"garmin-sec-source-guard: PASS — {n_files} files scanned, {len(RULES)} rules, 0 findings")
    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv))
