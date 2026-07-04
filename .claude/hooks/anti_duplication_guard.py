#!/usr/bin/env python3
"""
anti_duplication_guard.py — PreToolUse hook for the quality-gated-dev-workflow skill.

Deterministically enforces the wiki-memory anti-duplication rule (lesson LSN-007):
the agent must not re-create files or directories that already exist, and new artifacts
must be registered in the WIKI MAP.

Contract (Claude Code PreToolUse hook):
  - Reads a JSON object on stdin: {"tool_name": "...", "tool_input": {...}, "cwd": "..."}
  - Matched against tools: Write, Edit, Bash
  - On stdout, may print:
        {"hookSpecificOutput": {"hookEventName": "PreToolUse",
                                "permissionDecision": "deny" | "ask" | "allow",
                                "permissionDecisionReason": "..."}}
    Exit 0 with NO output  => no decision; normal permission flow continues (the safe default).
    Exit 0 WITH a deny/ask => blocks or prompts, reason shown to Claude.

Design choices (deliberately conservative to avoid false positives):
  * DENY the high-confidence overwrite mistakes: `mkdir` (without -p) of a dir that already
    exists; `touch`, a `>`/`>>` redirect, or `tee` over an existing file; `cp`/`mv` whose
    destination is an existing *file* (not a directory); and `ln`/`ln -sf` whose link name is
    an existing path. For `cp`/`mv`, only the final operand (destination) is ever a
    candidate, and an existing-*directory* destination is treated as legitimate (files land
    inside it), never a clobber.
  * For Write/Edit on an existing file -> ALLOW silently (edits are legitimate; Write is
    Claude Code's normal edit path). We do NOT block edits.
  * For a NEW path not present in WIKI MAP -> ASK with guidance (register it in the MAP and
    use the canonical location). ASK is non-destructive: the user decides. New files created
    via touch/redirect/tee/cp/mv/ln are asked-to-register just like Write.
  * A path resolving OUTSIDE the project root (relpath starts with "..") is left alone — the
    MAP only governs the project.
  * If WIKI.md cannot be found or parsed, the MAP-registration *asks* are suppressed, but
    the high-confidence clobber *deny* (creating a path that already exists) still fires —
    re-creating an existing path is a mistake regardless of whether the brain is set up.
    Malformed/empty input -> SILENT (exit 0, no output). The hook never crashes the turn.

The hook is idempotent, time-bounded, and has no third-party dependencies.
"""

import json
import os
import re
import sys

WIKI_FILENAMES = ("WIKI.md",)


# ----------------------------- output helpers -----------------------------

def emit(decision: str, reason: str) -> None:
    """Print a PreToolUse decision and exit 0 (the documented success path)."""
    print(json.dumps({
        "hookSpecificOutput": {
            "hookEventName": "PreToolUse",
            "permissionDecision": decision,            # "deny" | "ask" | "allow"
            "permissionDecisionReason": reason,
        }
    }))
    sys.exit(0)


def passthrough() -> None:
    """No decision: stay silent so the normal permission flow applies."""
    sys.exit(0)


# ----------------------------- wiki discovery -----------------------------

def find_wiki(start_dir: str) -> str | None:
    """Walk upward from start_dir looking for WIKI.md (the project brain)."""
    d = os.path.abspath(start_dir or ".")
    while True:
        for name in WIKI_FILENAMES:
            cand = os.path.join(d, name)
            if os.path.isfile(cand):
                return cand
        parent = os.path.dirname(d)
        if parent == d:
            return None
        d = parent


def load_map_paths(wiki_path: str) -> tuple[set[str], str]:
    """
    Parse the '## MAP' section of WIKI.md into a set of registered path tokens.
    Returns (paths, project_root). Paths are the first whitespace-delimited token on each
    MAP line (files or dirs, e.g. 'src/' or 'prd.md'). Best-effort and forgiving.
    """
    root = os.path.dirname(os.path.abspath(wiki_path))
    paths: set[str] = set()
    try:
        with open(wiki_path, "r", encoding="utf-8", errors="replace") as fh:
            text = fh.read()
    except OSError:
        return paths, root

    # Grab everything under a '## MAP' heading up to the next '## ' heading.
    m = re.search(r"^##\s*MAP\b.*?$(.*?)(?=^##\s|\Z)", text,
                  flags=re.MULTILINE | re.DOTALL)
    section = m.group(1) if m else ""
    for line in section.splitlines():
        line = line.strip()
        if not line or line.startswith("#"):
            continue
        token = line.split()[0]
        # strip trailing punctuation/separators but keep a trailing slash (dir marker)
        token = token.rstrip(",;")
        if token in ("—", "-", "|"):
            continue
        paths.add(token.rstrip("/") + ("/" if token.endswith("/") else ""))
    return paths, root


def in_map(path_tokens: set[str], rel: str) -> bool:
    """Is rel (or any parent dir of it) registered in the MAP?"""
    rel_norm = rel.lstrip("./")
    if rel_norm in path_tokens or (rel_norm + "/") in path_tokens:
        return True
    # parent-dir coverage: a file under a registered dir counts as mapped
    parts = rel_norm.split("/")
    for i in range(1, len(parts)):
        prefix = "/".join(parts[:i]) + "/"
        if prefix in path_tokens:
            return True
    return False


# ----------------------------- path extraction -----------------------------

import shlex

MKDIR_RE = re.compile(r"\bmkdir\b([^\n;&|]*)")
TOUCH_RE = re.compile(r"\btouch\b([^\n;&|]*)")
REDIRECT_CREATE_RE = re.compile(r"(?<![0-9])>>?\s*([^\s;&|>]+)")  # `> file` and `>> file`
TEE_RE = re.compile(r"\btee\b([^\n;&|]*)")            # tee writes/truncates each FILE arg
CP_MV_RE = re.compile(r"\b(cp|mv)\b([^\n;&|]*)")       # last operand is the destination
LN_RE = re.compile(r"\bln\b([^\n;&|]*)")               # last operand is the link name


def _tokens(argstr: str):
    """Tokenize an argument string, honoring quotes; fall back to naive split."""
    try:
        return shlex.split(argstr)
    except ValueError:
        return argstr.split()


def _positional(toks):
    """Return only positional (non-flag) tokens."""
    return [t for t in toks if not t.startswith("-")]


def shell_targets(command: str):
    """
    Yield (kind, raw_target) tuples that represent *creation/overwrite* intents.
    kind in {"mkdir","mkdir_p","touch","redirect","tee","copy_dest","move_dest","link_dest"}.
    Best-effort and conservative: for cp/mv/ln only the final operand (the destination) is
    a clobber candidate; sources are never flagged.
    """
    for chunk in re.split(r"(?:&&|\|\||;|\n)", command):
        chunk = chunk.strip()
        if not chunk:
            continue

        mk = MKDIR_RE.search(chunk)
        if mk:
            toks = _tokens(mk.group(1))
            has_p = any(re.fullmatch(r"-\w*p\w*", t) for t in toks if t.startswith("-"))
            for tok in _positional(toks):
                yield ("mkdir_p" if has_p else "mkdir", tok)

        tre = TOUCH_RE.search(chunk)
        if tre:
            for tok in _positional(_tokens(tre.group(1))):
                yield ("touch", tok)

        for rmatch in REDIRECT_CREATE_RE.finditer(chunk):
            yield ("redirect", rmatch.group(1).strip("'\""))

        te = TEE_RE.search(chunk)
        if te:
            # every positional arg to tee is written/truncated
            for tok in _positional(_tokens(te.group(1))):
                yield ("tee", tok)

        cm = CP_MV_RE.search(chunk)
        if cm:
            verb = cm.group(1)
            pos = _positional(_tokens(cm.group(2)))
            if len(pos) >= 2:  # need at least one source + a destination
                yield ("copy_dest" if verb == "cp" else "move_dest", pos[-1])

        lm = LN_RE.search(chunk)
        if lm:
            pos = _positional(_tokens(lm.group(1)))
            # `ln [opts] target link_name` -> link_name is the path that gets created
            if len(pos) >= 2:
                yield ("link_dest", pos[-1])


# ----------------------------- main logic -----------------------------

def main() -> None:
    try:
        raw = sys.stdin.read()
        event = json.loads(raw) if raw.strip() else {}
    except (json.JSONDecodeError, ValueError):
        passthrough()
        return

    tool = event.get("tool_name", "")
    tin = event.get("tool_input", {}) or {}
    cwd = event.get("cwd") or os.getcwd()

    wiki = find_wiki(cwd)
    map_paths, root = (set(), cwd)
    if wiki:
        map_paths, root = load_map_paths(wiki)

    def rel_to_root(p: str) -> str:
        ap = p if os.path.isabs(p) else os.path.join(cwd, p)
        try:
            return os.path.relpath(ap, root)
        except ValueError:
            return p

    def abspath(p: str) -> str:
        return p if os.path.isabs(p) else os.path.join(cwd, p)

    # ---- Write / Edit: only the file_path matters ----
    if tool in ("Write", "Edit", "MultiEdit"):
        fp = tin.get("file_path") or tin.get("path") or ""
        if not fp:
            passthrough()
            return
        ap = abspath(fp)
        rel = rel_to_root(fp)
        exists = os.path.exists(ap)

        if tool in ("Edit", "MultiEdit"):
            passthrough()           # edits of existing files are always fine
            return

        # tool == "Write"
        if exists:
            # Legitimate edit-via-Write. Allow, but if it's untracked, nudge (ask).
            if wiki and not in_map(map_paths, rel):
                emit("ask",
                     f"[anti-duplication guard] '{rel}' exists on disk but is NOT in the "
                     f"WIKI MAP. If this is an edit, proceed and add a MAP line. If you "
                     f"think you are creating a NEW file, STOP — it already exists; open it "
                     f"instead of duplicating.")
            passthrough()
            return
        else:
            # New file. Enforce MAP registration discipline (non-blocking ask).
            if wiki and not in_map(map_paths, rel):
                emit("ask",
                     f"[anti-duplication guard] Creating new file '{rel}', which is not yet "
                     f"in the WIKI MAP. Confirm it is at the canonical location "
                     f"(wiki/conventions.md) and add a MAP line as a byproduct of this "
                     f"creation (Principle 9).")
            passthrough()
            return

    # ---- Bash: look for creation intents ----
    if tool == "Bash":
        command = tin.get("command", "") or ""
        # Evaluate ALL creation targets, then let the strongest decision win (deny > ask).
        pending_ask = None
        # kinds that clobber/recreate an existing path (deny when target exists):
        clobber_kinds = ("mkdir", "touch", "redirect", "tee",
                         "copy_dest", "move_dest", "link_dest")
        # kinds that create a new file (ask-to-register when new + unmapped):
        newfile_kinds = ("touch", "redirect", "tee", "copy_dest", "move_dest", "link_dest")

        for kind, target in shell_targets(command):
            if not target:
                continue
            ap = abspath(target)
            rel = rel_to_root(target)
            # A path outside the project root is not governed by this MAP — leave it alone.
            if rel.startswith(".." + os.sep) or rel == "..":
                continue
            exists = os.path.exists(ap)

            # cp/mv into an EXISTING DIRECTORY is legitimate (files land inside it),
            # not a clobber. Only an existing *file* destination is an overwrite.
            if kind in ("copy_dest", "move_dest") and os.path.isdir(ap):
                continue

            # HIGH-CONFIDENCE DUPLICATION: re-creating/overwriting an existing path.
            # This deny is unconditional — clobbering an existing path is the core mistake,
            # whether or not a WIKI is present.
            if exists and kind in clobber_kinds:
                verb = {"copy_dest": "cp", "move_dest": "mv", "link_dest": "ln",
                        "tee": "tee", "touch": "touch"}.get(kind, "")
                vnote = f" (`{verb}` would overwrite it)" if verb else ""
                if wiki:
                    known = in_map(map_paths, rel)
                    where = " (registered in WIKI MAP)" if known else ""
                    reason = (f"[anti-duplication guard] '{rel}' already exists{where}{vnote}. "
                              f"This is the recurring re-creation/clobber mistake (lesson "
                              f"LSN-007). Do NOT overwrite it — open/use the existing path. If "
                              f"you truly need a different artifact, choose a distinct canonical "
                              f"path and register it in the WIKI MAP first.")
                else:
                    reason = (f"[anti-duplication guard] '{rel}' already exists{vnote}. Do NOT "
                              f"overwrite it — open/use the existing path instead of "
                              f"duplicating it.")
                emit("deny", reason)     # deny wins immediately
                return

            # mkdir -p on an existing dir is idempotent but signals confusion -> ask.
            if exists and kind == "mkdir_p" and pending_ask is None:
                pending_ask = (f"[anti-duplication guard] '{rel}' already exists; `mkdir -p` is "
                               f"a no-op here. You may be losing the thread — re-read WIKI.md "
                               f"to re-orient before continuing, rather than re-creating "
                               f"structure.")
                continue

            # Creating a new dir that isn't mapped -> gentle ask to register it.
            if ((not exists) and kind in ("mkdir", "mkdir_p")
                    and wiki and not in_map(map_paths, rel) and pending_ask is None):
                pending_ask = (f"[anti-duplication guard] Creating new directory '{rel}', not "
                               f"in the WIKI MAP. Add a MAP line for it as a byproduct "
                               f"(Principle 9), at the canonical location from "
                               f"wiki/conventions.md.")
                continue

            # Creating a new FILE that isn't mapped -> gentle ask to register it.
            if ((not exists) and kind in newfile_kinds
                    and wiki and not in_map(map_paths, rel) and pending_ask is None):
                pending_ask = (f"[anti-duplication guard] Creating new file '{rel}', not in the "
                               f"WIKI MAP. Confirm the canonical location (wiki/conventions.md) "
                               f"and add a MAP line as a byproduct (Principle 9).")
                continue

        if pending_ask is not None:
            emit("ask", pending_ask)
        passthrough()
        return

    # Any other tool: not our concern.
    passthrough()


if __name__ == "__main__":
    main()
