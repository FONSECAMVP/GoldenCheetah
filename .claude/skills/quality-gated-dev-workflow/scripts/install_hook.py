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


def main() -> int:
    project = os.path.abspath(sys.argv[1] if len(sys.argv) > 1 else ".")
    if not os.path.isdir(project):
        print(f"error: not a directory: {project}", file=sys.stderr)
        return 1

    here = os.path.dirname(os.path.abspath(__file__))
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
