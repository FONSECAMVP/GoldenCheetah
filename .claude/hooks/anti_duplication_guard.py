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
  * Relative Bash targets are resolved against the EFFECTIVE working directory at that point
    in the command, not blindly against the event's `cwd`: a literal `cd` earlier in the
    chain moves the base (`cd docs && touch X` targets `<cwd>/docs/X`). Command
    substitutions inherit the surrounding directory, and a `cd` inside `$( )` does not leak
    back out. When a directory change cannot be resolved deterministically (`cd "$VAR"`,
    `cd "$(...)"`, `cd -`, `pushd`, or a `cd` whose target is not an existing directory) the
    effective cwd becomes UNKNOWN: relative targets after it are never denied on a guessed
    path — they raise an ASK — while absolute targets keep being evaluated normally.
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

# Behavior modes via the QGDW_GUARD_MODE environment variable:
#   "full"       (default) — deny clobbers, ask to register new unmapped artifacts
#   "deny-only"  — only deny real clobbers (overwriting an existing path); every "ask"
#                  becomes a silent passthrough so the dev loop is never interrupted for
#                  merely-unregistered new files
#   "off"        — disable the guard entirely (always passthrough)
_MODE = os.environ.get("QGDW_GUARD_MODE", "full").strip().lower()


def emit(decision: str, reason: str) -> None:
    """Print a PreToolUse decision and exit 0 (the documented success path)."""
    if _MODE == "off":
        sys.exit(0)
    if decision == "ask" and _MODE == "deny-only":
        # Suppress advisory asks in deny-only mode: stay silent, let normal flow proceed.
        sys.exit(0)
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

# Verb regexes are matched against QUOTE-MASKED text (so a verb inside a quoted string —
# e.g. a grep PATTERN containing "ln " — is DATA, never a command). Argument substrings are
# then sliced from the ORIGINAL text at the same span, so quoted paths survive intact.
MKDIR_RE = re.compile(r"\bmkdir\b([^\n;&|]*)")
TOUCH_RE = re.compile(r"\btouch\b([^\n;&|]*)")
# capture the operator so >> (append, content-preserving) is distinguished from > (truncate)
REDIRECT_RE = re.compile(r"(?<![0-9<>])(>>|>)(?!>)\s*([^\s;&|<>]+)")
TEE_RE = re.compile(r"\btee\b([^\n;&|]*)")
CP_MV_RE = re.compile(r"\b(cp|mv)\b([^\n;&|]*)")
LN_RE = re.compile(r"\bln\b([^\n;&|]*)")

_BACKUP_SUFFIXES = (".orig", ".bak", ".backup")


def strip_heredocs(command: str) -> str:
    """Remove heredoc BODIES (they are data, not commands). The line containing the
    << marker is kept (its redirect target still gets scanned); everything up to and
    including the terminator line is dropped."""
    out, lines, i = [], command.split("\n"), 0
    while i < len(lines):
        line = lines[i]
        out.append(line)
        m = re.search(r"<<-?\s*['\"]?(\w+)['\"]?", line)
        if m:
            term = m.group(1)
            i += 1
            while i < len(lines) and lines[i].strip() != term:
                i += 1
            i += 1  # skip the terminator line itself
            continue
        i += 1
    return "\n".join(out)


def _mask_quoted(s: str) -> tuple[str, list[int]]:
    """Replace the CONTENT of single/double-quoted regions with spaces (same length, so
    match spans map 1:1 back onto the original). Quote chars themselves are kept.

    Returns (masked, depths) where depths[i] is the command-substitution NESTING LEVEL of
    character i (0 = the outer shell). Callers use it to scope directory state: a chunk at
    depth d+1 runs in a subshell that INHERITS the enclosing directory but whose own `cd`
    dies with it.

    Command substitutions are handled as their own parsing context. Per POSIX, the text
    inside `$( )` is parsed as a NEW command: its quotes are independent of any enclosing
    double quotes. A flat left-to-right scan gets this wrong — the first `"` inside the
    substitution closes the outer double quote, quote state desyncs, and quoted DATA
    downstream is then read as executable text. So on `$(` the current quote state is
    pushed and parsing restarts unquoted; the matching `)` pops it back.

    The substitution's delimiters are emitted as newlines (length-preserving, and `\\n`
    already bounds every verb regex and chunk split). That keeps a genuinely executed
    nested command scannable as its own chunk, while stopping an argument capture from
    running past the closing paren.
    """
    out = list(s)
    n, i, q, stack = len(s), 0, None, []
    depths = [0] * n
    while i < n:
        ch = s[i]
        depths[i] = len(stack)
        if q == "'":
            # single quotes are literal: no escapes, no substitution
            if ch == "'":
                q = None
            else:
                out[i] = " " if ch != "\n" else "\n"
            i += 1
        elif q == '"':
            if ch == "\\" and i + 1 < n:
                out[i] = " "
                depths[i + 1] = len(stack)
                if s[i + 1] != "\n":
                    out[i + 1] = " "
                i += 2
            elif ch == '"':
                q = None
                i += 1
            elif ch == "$" and i + 1 < n and s[i + 1] == "(":
                depths[i + 1] = len(stack)      # the `$(` pair belongs to the OUTER level
                stack.append(q); q = None
                out[i] = out[i + 1] = "\n"
                i += 2
            else:
                out[i] = " " if ch != "\n" else "\n"
                i += 1
        else:
            if ch == "$" and i + 1 < n and s[i + 1] == "(":
                depths[i + 1] = len(stack)
                stack.append(q); q = None
                out[i] = out[i + 1] = "\n"
                i += 2
                continue
            if ch == ")" and stack:
                q = stack.pop()
                out[i] = "\n"
                i += 1
                continue
            if ch in ("'", '"'):
                q = ch
            i += 1
    return "".join(out), depths


def _tokens(argstr: str):
    try:
        return shlex.split(argstr)
    except ValueError:
        return argstr.split()


def _positional(toks):
    return [t for t in toks if not t.startswith("-")]


def _has_flag(toks, letter):
    return any(re.fullmatch(r"-\w*%s\w*" % letter, t) for t in toks if t.startswith("-"))


# A chunk is a directory change only when it STARTS with the verb (matched on masked text,
# so `echo "cd /tmp"` is data). `pushd`/`popd` are recognised only to mark the state unknown,
# as is a cd opening a construct we do not model — a `( )` subshell or a `{ }` group — since
# whether the move survives the construct is exactly what we would have to guess.
CD_RE = re.compile(r"^(\s*[({]?\s*)(cd|pushd|popd)\b([^\n]*)$")

# Any of these in a cd operand means the literal path is not knowable without executing
# something: parameter/command substitution, a backquote, a glob, or ~ (HOME-dependent).
_DYNAMIC_OPERAND = re.compile(r"[$`*?~\[\]]")

_UNSET = object()               # "this chunk performed no directory change"


def _resolve_cd(argstr: str, base):
    """Effective directory after a `cd` whose operand text is `argstr` (sliced from the
    ORIGINAL command, so quoted literals — including paths with spaces — survive).

    Returns an absolute, normalised directory, or None when the resulting shell state is
    NOT deterministically knowable. None is returned for: a dynamic operand (`cd "$VAR"`,
    `cd "$(...)"`, backquotes, globs, `~`), `cd -`, bare `cd` (HOME), the two-operand
    substitution form, an unknown flag, an operand we cannot tokenise (e.g. the text was
    cut short by a substitution), an unresolved base, and a target that is not an existing
    directory (the `cd` would fail, so what runs next — and where — is a guess).
    """
    try:
        toks = shlex.split(argstr)
    except ValueError:
        return None
    operands, end_of_flags = [], False
    for t in toks:
        if end_of_flags:
            operands.append(t)
        elif t == "--":
            end_of_flags = True
        elif t.startswith("-") and t != "-":
            if not re.fullmatch(r"-[LP@e]+", t):
                return None                     # unknown flag: do not guess
        else:
            operands.append(t)
    if len(operands) != 1:
        return None
    target = operands[0]
    if target == "-" or _DYNAMIC_OPERAND.search(target):
        return None
    if not os.path.isabs(target):
        if base is None:
            return None                         # relative move from an unknown place
        target = os.path.join(base, target)
    ap = os.path.normpath(target)
    return ap if os.path.isdir(ap) else None


_BACKUP_WORD_RE = re.compile(r"^(?P<base>.+)[.\-](?:orig|bak|backup)$")


def _is_snapshot_restore(srcs, dest):
    """cp/mv <anywhere>/<name>.orig <path>/<name> is the sanctioned snapshot-restore
    pattern (LSN-032): restoring a file from its own backup copy is NOT a clobber. The
    backup may live in a scratchpad dir (/tmp/.../X.cpp.orig -> src/.../X.cpp), so the
    match is on BASENAME + backup suffix, not on sibling paths.

    The basename may also carry EXTRA decoration between the destination's own name and
    the backup word (`X.cpp.orch-mutation-orig`, not just `X.cpp.orig`) — a session
    reasonably names a snapshot for traceability when several may exist at once. The
    destination's basename must still be a literal prefix (an unrelated file's backup,
    e.g. `OtherFile.cpp.orig`, still cannot restore onto `CloudService.cpp`), and the
    source must still end in exactly one of the three known words — only the middle
    decoration is tolerated (ORCH-058: the exact-match version denied this same LSN-084/
    A3-mutation-proof convention it exists to sanction)."""
    if len(srcs) != 1:
        return False
    sb = os.path.basename(srcs[0])
    db = os.path.basename(dest.rstrip("/"))
    m = _BACKUP_WORD_RE.match(sb)
    if not m:
        return False
    base = m.group("base")
    return base == db or base.startswith(db + ".") or base.startswith(db + "-")


def shell_targets(command: str, base_cwd=None):
    """
    Yield (kind, raw_target, eff_cwd) tuples for creation/overwrite intents.
    kinds: mkdir, mkdir_p, touch, redirect (truncating >), append (>> — content-preserving,
    never a clobber), tee, tee_append (tee -a), copy_dest, move_dest, link_dest.
    Quote-masked verb detection; original-text argument extraction; heredoc bodies ignored;
    snapshot-restore (cp/mv from a .orig/.bak/.backup of the same path) exempted.

    `eff_cwd` is the directory a RELATIVE target resolves against at that point in the
    command — `base_cwd` moved by every literal `cd` seen so far — or None when a
    non-deterministic directory change makes it unknowable. Substitution frames inherit
    the enclosing directory and are discarded on close, so an inner `cd` never leaks out.
    A redirection/creation written on the `cd` command itself still resolves against the
    directory BEFORE the move, which is the shell's own order.
    """
    cmd = strip_heredocs(command)
    masked, depths = _mask_quoted(cmd)

    # chunk on separators found in MASKED text (a ';' inside quotes is data) and slice
    # both strings by the same spans so indices stay aligned.
    bounds, prev = [], 0
    for m in re.finditer(r"(?:&&|\|\||;|\n)", masked):
        bounds.append((prev, m.start())); prev = m.end()
    bounds.append((prev, len(masked)))

    # cwd_stack[d] = effective directory of substitution depth d (None = unknown)
    cwd_stack = [base_cwd]

    for a, b in bounds:
        mchunk, ochunk = masked[a:b], cmd[a:b]
        if not mchunk.strip():
            continue

        # Align the directory stack with this chunk's substitution depth: opening `$( )`
        # inherits the surrounding directory, closing it discards the subshell's frame.
        depth = depths[a] if a < len(depths) else 0
        while len(cwd_stack) <= depth:
            cwd_stack.append(cwd_stack[-1])
        del cwd_stack[depth + 1:]
        eff = cwd_stack[depth]

        cdm = CD_RE.match(mchunk)
        moved_to = _UNSET
        if cdm:
            plain = cdm.group(1).strip() == "" and cdm.group(2) == "cd"
            moved_to = (_resolve_cd(ochunk[cdm.start(3):cdm.end(3)], eff)
                        if plain else None)     # pushd/popd/subshell/group: unknowable

        mk = MKDIR_RE.search(mchunk)
        if mk:
            toks = _tokens(ochunk[mk.start(1):mk.end(1)])
            kind = "mkdir_p" if _has_flag(toks, "p") else "mkdir"
            for tok in _positional(toks):
                yield (kind, tok, eff)

        tre = TOUCH_RE.search(mchunk)
        if tre:
            for tok in _positional(_tokens(ochunk[tre.start(1):tre.end(1)])):
                yield ("touch", tok, eff)

        for rm in REDIRECT_RE.finditer(mchunk):
            target = ochunk[rm.start(2):rm.end(2)].strip("'\"")
            yield ("append" if rm.group(1) == ">>" else "redirect", target, eff)

        te = TEE_RE.search(mchunk)
        if te:
            toks = _tokens(ochunk[te.start(1):te.end(1)])
            kind = "tee_append" if _has_flag(toks, "a") else "tee"
            for tok in _positional(toks):
                yield (kind, tok, eff)

        cm = CP_MV_RE.search(mchunk)
        if cm:
            verb = cm.group(1)
            pos = _positional(_tokens(ochunk[cm.start(2):cm.end(2)]))
            if len(pos) >= 2 and not _is_snapshot_restore(pos[:-1], pos[-1]):
                yield ("copy_dest" if verb == "cp" else "move_dest", pos[-1], eff)

        lm = LN_RE.search(mchunk)
        if lm:
            pos = _positional(_tokens(ochunk[lm.start(1):lm.end(1)]))
            if len(pos) >= 2:
                yield ("link_dest", pos[-1], eff)

        # Commit the directory change AFTER this chunk's own targets were resolved.
        if moved_to is not _UNSET:
            cwd_stack[depth] = moved_to


# ----------------------------- main logic -----------------------------

def inside_root(ap: str, root: str) -> bool:
    """True when `ap` IS the project root or a descendant of it.

    Containment is decided before any policy is applied: a path that merely *looks*
    like a project path (a temp snapshot, another checkout) is not governed by this
    project's MAP or vendor rules.
    """
    try:
        rel = os.path.relpath(os.path.normpath(ap), root)
    except ValueError:              # e.g. different drives on Windows
        return False
    return rel == os.curdir or not (rel == os.pardir or
                                    rel.startswith(os.pardir + os.sep))


# Skill packages that ship their own installer/reinstall mechanism and are therefore
# genuinely replaced wholesale on update (see install_hook.py). A sibling skill
# directory that isn't one of these (e.g. a project-authored skill living right next
# to it) is NOT vendor territory just for being under `.claude/skills/`.
VENDOR_SKILL_PACKAGES = ("quality-gated-dev-workflow",)


def is_vendor_path(ap: str, root: str) -> bool:
    """Vendor territory is ROOT-ANCHORED: exactly `<root>/.claude/skills/<pkg>` for a
    known vendor package (`VENDOR_SKILL_PACKAGES`) or a descendant of it — never the
    whole `.claude/skills/` tree and never an arbitrary absolute-path substring.

    `/tmp/snap/.claude/skills/quality-gated-dev-workflow/x.md` (outside the project)
    and `<root>/.claude/skills/some-other-skill/x.md` (inside, but not a vendor
    package) are both NOT vendor territory; `<root>/.claude/skills/
    quality-gated-dev-workflow/...` still is.
    """
    if not inside_root(ap, root):
        return False
    try:
        rel = os.path.relpath(os.path.normpath(ap), root)
    except ValueError:
        return False
    skills_dir = os.path.join(".claude", "skills")
    for pkg in VENDOR_SKILL_PACKAGES:
        vendor = os.path.join(skills_dir, pkg)
        if rel == vendor or rel.startswith(vendor + os.sep):
            return True
    return False


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

        # CONTAINMENT FIRST: a target outside the project root is governed by neither
        # this MAP nor the vendor rule, however its absolute path happens to read.
        if not inside_root(ap, root):
            passthrough()
            return

        # VENDOR TERRITORY: <root>/.claude/skills is replaced wholesale on skill updates.
        # New project-owned files created there WILL be destroyed by the next update.
        in_skill_tree = is_vendor_path(ap, root)
        if in_skill_tree:
            if not exists:
                emit("deny",
                     f"[anti-duplication guard] '{rel}' is inside .claude/skills/ — vendor "
                     f"territory that is REPLACED wholesale on every skill update; a file "
                     f"created here will be destroyed. Put project-owned tooling at a "
                     f"project path (e.g. scripts/ or tools/ at the project root), register "
                     f"it in the WIKI MAP, and reference it from there.")
                return
            else:
                emit("ask",
                     f"[anti-duplication guard] '{rel}' is a skill file — local edits are "
                     f"LOST on the next skill update. Proceed only if intentional, and "
                     f"record the change so it can be folded upstream (or copy the file to "
                     f"a project path instead).")
                return

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
        unresolved_ask = None
        # kinds that clobber/recreate an existing path (deny when target exists):
        clobber_kinds = ("mkdir", "touch", "redirect", "tee",
                         "copy_dest", "move_dest", "link_dest")
        # append (>> / tee -a) preserves content: NEVER a clobber; only ask-to-register new
        # kinds that create a new file (ask-to-register when new + unmapped):
        newfile_kinds = ("touch", "redirect", "append", "tee", "tee_append",
                         "copy_dest", "move_dest", "link_dest")

        for kind, target, eff in shell_targets(command, cwd):
            if not target:
                continue
            if os.path.isabs(target):
                ap = os.path.normpath(target)
            elif eff is None:
                # The effective cwd at this point is NOT deterministically knowable, so
                # this relative name could denote anything. Never deny a guessed path:
                # ask, and keep evaluating the rest (absolute targets are unaffected).
                if unresolved_ask is None:
                    unresolved_ask = (
                        f"[anti-duplication guard] This command changes directory in a way "
                        f"the guard cannot resolve deterministically (e.g. `cd \"$VAR\"`, "
                        f"`cd \"$(...)\"`, `cd -`, `pushd`, or a `cd` into a directory that "
                        f"does not exist), so the effective working directory where "
                        f"'{target}' would be created is unknown and the target cannot be "
                        f"classified safely — it may or may not clobber an existing project "
                        f"path. Use an absolute path, or run the `cd` as its own step, so "
                        f"the target can be checked against the tree and the WIKI MAP.")
                continue
            else:
                ap = os.path.normpath(os.path.join(eff, target))
            rel = rel_to_root(ap)
            # CONTAINMENT FIRST: a path outside the project root is governed by neither
            # this MAP nor the vendor rule — leave it alone.
            if not inside_root(ap, root):
                continue
            exists = os.path.exists(ap)

            # VENDOR TERRITORY (root-anchored): creating anything new under
            # <root>/.claude/skills is denied — the tree is replaced wholesale on skill
            # updates and the file would be lost.
            if (is_vendor_path(ap, root) and not exists
                    and kind in ("mkdir", "mkdir_p", "touch", "redirect", "tee",
                                 "copy_dest", "move_dest", "link_dest")):
                emit("deny",
                     f"[anti-duplication guard] '{rel}' is inside .claude/skills/ — vendor "
                     f"territory replaced wholesale on skill updates; anything created here "
                     f"will be destroyed. Use a project path (scripts/ or tools/ at the "
                     f"project root) and register it in the WIKI MAP.")
                return

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

        # An unknown effective cwd is the more important thing to surface: it means a
        # target could not be classified at all, not merely that it is unregistered.
        if unresolved_ask is not None:
            emit("ask", unresolved_ask)
        if pending_ask is not None:
            emit("ask", pending_ask)
        passthrough()
        return

    # Any other tool: not our concern.
    passthrough()


if __name__ == "__main__":
    main()
