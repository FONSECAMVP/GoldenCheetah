#!/usr/bin/env python3
"""Two-directional regression matrix for the anti-duplication guard's COMMAND-POSITION parsing.

Owned by ORCH-043 (the ORCH-040 mechanism repair). Lives in `scripts/` — the project-owned
canonical location for mechanisms (ORCH-004) — deliberately NOT in the skill tree, which is
vendor territory replaced wholesale on update. The guard it exercises is the INSTALLED copy at
`.claude/hooks/anti_duplication_guard.py`, i.e. the one `.claude/settings.json` actually runs.

THE DEFECT UNDER REGRESSION. The verb regexes were anchored with `\\b<verb>\\b`. In
`grep -ln PATTERN FILE` the `-` is a non-word character, so `\\bln\\b` matched the FLAG CLUSTER
`-ln`, and the guard then parsed a read-only grep as an `ln` clobber of the last path on the
line, denying a legitimate operation. `grep -rln` never reproduced it — the `r` supplies a word
character, so `\\b` did not match — which is why the bug survived: the common spelling was safe.

BOTH DIRECTIONS ARE REQUIRED. A repair that only stops the false positive could be achieved by
disabling `ln` detection altogether, which would silently remove a real guard. So every case
below is paired: the legitimate flag cluster must PASS, and the genuine clobber it resembles
must still be DENIED.

Run:  python3 -m pytest scripts/test_anti_duplication_guard_flags.py -q
  or: python3 scripts/test_anti_duplication_guard_flags.py     (exit code = failure count)
"""

import json
import os
import subprocess
import sys
import tempfile

REPO_ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
GUARD = os.path.join(REPO_ROOT, ".claude", "hooks", "anti_duplication_guard.py")


def _make_project():
    """A minimal mapped project: WIKI.md whose MAP registers src/, plus an existing file
    under it. `src/existing.txt` is the clobber target every negative case aims at."""
    d = tempfile.mkdtemp(prefix="qgdw_flags_")
    os.makedirs(os.path.join(d, "src"), exist_ok=True)
    with open(os.path.join(d, "src", "existing.txt"), "w") as fh:
        fh.write("payload\n")
    with open(os.path.join(d, "src", "other.txt"), "w") as fh:
        fh.write("payload\n")
    with open(os.path.join(d, "WIKI.md"), "w") as fh:
        fh.write(
            "# PROJECT WIKI\nroot: %s\nphase: 2\n\n"
            "## MAP\nsrc/ code\nWIKI.md brain\n\n"
            "## REGISTRIES\nREQ 001-001 next:002\n\n"
            "## PAGES\nwiki/conventions.md conv\n" % d
        )
    return d


PROJECT = _make_project()


def decision(command, cwd=PROJECT):
    """Run the guard exactly as the harness does and return its permissionDecision.
    'passthrough' means the guard emitted nothing, i.e. the command is allowed to run."""
    event = json.dumps(
        {"tool_name": "Bash", "tool_input": {"command": command}, "cwd": cwd}
    )
    proc = subprocess.run(
        [sys.executable, GUARD], input=event, capture_output=True, text=True, timeout=30
    )
    out = proc.stdout.strip()
    if not out:
        return "passthrough"
    try:
        return json.loads(out)["hookSpecificOutput"]["permissionDecision"]
    except Exception:
        return "BADOUTPUT:" + out[:80]


# (command, expected, note). "passthrough" = must be allowed; "deny" = must still be blocked.
CASES = [
    # ---- DIRECTION 1: legitimate FLAG CLUSTERS must pass -------------------------------
    (
        'grep -ln "QNetworkAccessManager" src/existing.txt',
        "passthrough",
        "THE REPORTED FALSE POSITIVE: -ln is a grep flag cluster, not the ln binary",
    ),
    (
        'grep -lni "pat" src/existing.txt',
        "passthrough",
        "longer cluster still beginning with ln",
    ),
    (
        'grep -rln "pat" src/',
        "passthrough",
        "the spelling that never reproduced (r removes the word boundary) — must stay green",
    ),
    (
        "ls -ln src/existing.txt",
        "passthrough",
        "a REAL ls flag cluster meaning numeric-uid long listing",
    ),
    (
        'grep -cp "pat" src/existing.txt',
        "passthrough",
        "same defect class for the cp verb",
    ),
    (
        'grep -mv "pat" src/existing.txt',
        "passthrough",
        "same defect class for the mv verb",
    ),
    (
        'grep -tee "pat" src/existing.txt',
        "passthrough",
        "same defect class for the tee verb",
    ),
    (
        'grep --line-number "pat" src/existing.txt',
        "passthrough",
        "long-form flag must be unaffected",
    ),
    # ---- DIRECTION 2: genuine CLOBBERS must still be denied ----------------------------
    (
        "ln src/other.txt src/existing.txt",
        "deny",
        "hard link whose link NAME is an existing file",
    ),
    (
        "ln -s src/other.txt src/existing.txt",
        "deny",
        "symlink over an existing file",
    ),
    (
        "ln -f src/other.txt src/existing.txt",
        "deny",
        "forced link over an existing file",
    ),
    (
        "ln -sf src/other.txt src/existing.txt",
        "deny",
        "the -sf spelling ORCH-040 names explicitly",
    ),
    (
        "/usr/bin/ln -sf src/other.txt src/existing.txt",
        "deny",
        "path-qualified binary must still be recognised (preceded by / not -)",
    ),
    (
        "cp src/other.txt src/existing.txt",
        "deny",
        "cp clobber — the negative anchor for the -cp case above",
    ),
    (
        "mv src/other.txt src/existing.txt",
        "deny",
        "mv clobber — the negative anchor for the -mv case above",
    ),
]


def _check(command, expected, note):
    got = decision(command)
    if expected == "deny":
        ok = got == "deny"
    else:
        # 'ask' is also a block for our purposes: the operation did not pass cleanly.
        ok = got == "passthrough"
    return ok, got


try:  # pytest is optional; the __main__ runner below works without it
    import pytest

    @pytest.mark.parametrize("command,expected,note", CASES)
    def test_command_position_parsing(command, expected, note):
        ok, got = _check(command, expected, note)
        assert ok, "%s\n  command : %s\n  expected: %s\n  got     : %s" % (
            note,
            command,
            expected,
            got,
        )

except ImportError:  # pragma: no cover
    pass


def main():
    failures = 0
    print("guard under test: %s" % GUARD)
    print("fixture project : %s\n" % PROJECT)
    for command, expected, note in CASES:
        ok, got = _check(command, expected, note)
        if not ok:
            failures += 1
        print(
            "%-4s | %-11s | %-11s | %s"
            % ("PASS" if ok else "FAIL", expected, got, note)
        )
        print("       %s" % command)
    print("\nSUMMARY_JSON=%s" % json.dumps({"cases": len(CASES), "failures": failures}))
    return failures


if __name__ == "__main__":
    sys.exit(main())
