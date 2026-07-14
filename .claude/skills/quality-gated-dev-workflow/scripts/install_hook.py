#!/usr/bin/env python3
"""
install_hook.py — install the anti-duplication PreToolUse hook into a project.

Usage:
    python3 install_hook.py [project_dir]      # default: current working directory

What it does (idempotently — safe to re-run):
  1. Copies anti_duplication_guard.py -> <project>/.claude/hooks/anti_duplication_guard.py
  2. Merges a PreToolUse hook entry into <project>/.claude/settings.json without
     disturbing any existing settings or hooks.

It never overwrites unrelated config and never adds a duplicate hook entry.
Stdlib only; requires python3 on PATH.
"""

import json
import os
import shutil
import sys

MATCHER = "Write|Edit|MultiEdit|Bash"
COMMAND = 'python3 "${CLAUDE_PROJECT_DIR}/.claude/hooks/anti_duplication_guard.py"'


def copy_lint(project, here):
    """Copy the DEC-015 ledger-drift lint next to the guard in .claude/hooks/.
    Idempotent. The pre-commit-framework local hook + the CLV step both call this
    installed copy at a stable path. Returns True on success, False if source missing."""
    src_lint = os.path.join(here, "ledger_drift_lint.py")
    if not os.path.isfile(src_lint):
        print(f"warn: no ledger_drift_lint.py next to installer ({src_lint}); "
              f"skipping lint install", file=sys.stderr)
        return False
    hooks_dir = os.path.join(project, ".claude", "hooks")
    os.makedirs(hooks_dir, exist_ok=True)
    dst_lint = os.path.join(hooks_dir, "ledger_drift_lint.py")
    shutil.copyfile(src_lint, dst_lint)
    os.chmod(dst_lint, 0o755)
    print(f"✓ ledger-drift lint → {os.path.relpath(dst_lint, project)} "
          f"(wire as a pre-commit local hook; see .pre-commit-config.yaml)")
    return True


def main() -> int:
    # Optional: --mode {full|deny-only|off}. Default full. deny-only = never interrupt
    # the dev loop for new files; only block real clobbers.
    argv = [a for a in sys.argv[1:]]
    mode = "full"
    lint_only = False
    rest = []
    i = 0
    while i < len(argv):
        if argv[i] == "--mode" and i + 1 < len(argv):
            mode = argv[i + 1].strip().lower()
            i += 2
            continue
        if argv[i] == "--lint-only":
            lint_only = True
            i += 1
            continue
        rest.append(argv[i])
        i += 1
    if mode not in ("full", "deny-only", "off"):
        print(f"error: --mode must be full|deny-only|off (got {mode})", file=sys.stderr)
        return 1

    project = os.path.abspath(rest[0] if rest else ".")
    if not os.path.isdir(project):
        print(f"error: not a directory: {project}", file=sys.stderr)
        return 1

    here = os.path.dirname(os.path.abspath(__file__))

    # --lint-only: install just the DEC-015 lint (no guard/agents/settings churn).
    if lint_only:
        return 0 if copy_lint(project, here) else 1

    # Build the hook command, prefixing the mode env var when not the default.
    global COMMAND
    if mode != "full":
        COMMAND = ('env QGDW_GUARD_MODE=' + mode +
                   ' python3 "${CLAUDE_PROJECT_DIR}/.claude/hooks/anti_duplication_guard.py"')

    src_script = os.path.join(here, "anti_duplication_guard.py")
    if not os.path.isfile(src_script):
        print(f"error: cannot find guard script next to installer: {src_script}",
              file=sys.stderr)
        return 1

    hooks_dir = os.path.join(project, ".claude", "hooks")
    os.makedirs(hooks_dir, exist_ok=True)
    dst_script = os.path.join(hooks_dir, "anti_duplication_guard.py")
    shutil.copyfile(src_script, dst_script)
    os.chmod(dst_script, 0o755)
    print(f"✓ guard script → {os.path.relpath(dst_script, project)}")

    # DEC-015 ledger-drift lint, installed alongside the guard.
    copy_lint(project, here)

    # Install subagent definitions (agents/ ships next to scripts/ in the skill)
    src_agents = os.path.join(os.path.dirname(here), "agents")
    if os.path.isdir(src_agents):
        dst_agents = os.path.join(project, ".claude", "agents")
        os.makedirs(dst_agents, exist_ok=True)
        n = 0
        for fn in sorted(os.listdir(src_agents)):
            if fn.endswith(".md"):
                shutil.copyfile(os.path.join(src_agents, fn),
                                os.path.join(dst_agents, fn))
                n += 1
        print(f"✓ {n} subagent(s) → {os.path.relpath(dst_agents, project)}/ "
              f"(restart the session to load them)")

    settings_path = os.path.join(project, ".claude", "settings.json")
    settings = {}
    if os.path.isfile(settings_path):
        try:
            with open(settings_path, "r", encoding="utf-8") as fh:
                settings = json.load(fh) or {}
        except (json.JSONDecodeError, OSError) as e:
            print(f"error: could not read existing settings.json ({e}); "
                  f"not touching it. Add the hook manually.", file=sys.stderr)
            return 1

    hooks = settings.setdefault("hooks", {})
    pretool = hooks.setdefault("PreToolUse", [])

    # Find any existing guard hook entry (matched by referencing our script name), so we can
    # UPGRADE a stale/unquoted command from older installs instead of leaving it broken.
    def is_guard(h):
        return isinstance(h, dict) and "anti_duplication_guard.py" in h.get("command", "")

    existing = [h for group in pretool if isinstance(group, dict)
                for h in group.get("hooks", []) if is_guard(h)]

    if existing:
        changed = False
        for h in existing:
            if h.get("command") != COMMAND:
                h["command"] = COMMAND          # repair old unquoted/path-split command
                changed = True
        if changed:
            with open(settings_path, "w", encoding="utf-8") as fh:
                json.dump(settings, fh, indent=2)
                fh.write("\n")
            print("✓ upgraded existing guard hook to the quoted-path command "
                  "(fixes paths containing spaces)")
        else:
            print("✓ hook already present and correct in settings.json (no change)")
    else:
        pretool.append({
            "matcher": MATCHER,
            "hooks": [{"type": "command", "command": COMMAND}],
        })
        with open(settings_path, "w", encoding="utf-8") as fh:
            json.dump(settings, fh, indent=2)
            fh.write("\n")
        print(f"✓ hook entry merged into {os.path.relpath(settings_path, project)}")

    print("\nDone. Restart Claude Code (or run /hooks) to load the guard.")
    print("Verify python3 is available:  python3 --version")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
