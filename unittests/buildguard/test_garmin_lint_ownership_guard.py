#!/usr/bin/env python3
"""B-STAGE9-16 / DEC-054 Part B — unit tests for the lint-ownership guard.

The guard under test answers "is every Garmin-owned file linted by something?"
That is a COVERAGE question, and coverage checks are the easiest kind of check
to make vacuous: one that iterates an empty file list, or that derives its
expectation from the same place as the thing it checks, passes green forever
while protecting nothing. `.pre-commit-config.yaml`'s enumeration was itself
green-and-worthless for four months for exactly that reason, so it would be
absurd to defend it with a test carrying the same disease.

Three consequences for how this file is written:

  * Fixtures are REAL `.pre-commit-config.yaml` documents in a tmp dir and REAL
    git repositories with real `git add`ed files. The guard reads the config
    with PyYAML and enumerates files with `git ls-files`, so both are exercised
    end to end rather than mocked. pre-commit's own matching semantics
    (`re.search`, not `re.match`) are asserted directly, because getting that
    wrong would silently invert every verdict.
  * Every positive assertion ("the guard passes here") is paired with the
    mutation of that same fixture that makes it FAIL. A guard that has never
    been seen to go red has not been tested, it has been run.
  * The VACUITY cases are first-class tests, not an afterthought:
    an empty root, a root whose files are all gap-declared, and a config whose
    hooks never receive filenames must all be distinguishable from real
    coverage. Those are the ways this guard could lie while printing PASS.
"""

from __future__ import annotations

import subprocess
import sys
from pathlib import Path

import pytest

sys.path.insert(0, str(Path(__file__).resolve().parent))

import garmin_lint_ownership_guard as guard  # noqa: E402
from garmin_lint_ownership_guard import (  # noqa: E402
    KNOWN_NARROWING_KEYS,
    MANAGED_ROOTS,
    MIN_REASON_CHARS,
    MODELLED_HOOK_KEYS,
    ConfigError,
    Gap,
    Hook,
    ManagedRoot,
    RootReport,
    check_declared_gaps,
    check_model_is_faithful,
    check_not_vacuous,
    check_ownership,
    hooks_owning,
    load_hooks,
    tracked_files,
)

REPO_ROOT = Path(__file__).resolve().parents[2]


# --------------------------------------------------------------------------
# fixtures
# --------------------------------------------------------------------------
def write_config(tmp_path: Path, body: str) -> Path:
    path = tmp_path / ".pre-commit-config.yaml"
    path.write_text(body, encoding="utf-8")
    return path


SIMPLE_CONFIG = """
repos:
  - repo: https://example.invalid/ruff
    rev: v0.6.9
    hooks:
      - id: ruff
        files: '^pkg/.*\\.py$'
"""

# The live config's ruff scope: a hook that declares its OWN files:, which is
# the case where a top-level filter was previously discarded.
HOOK_OWN_FILES = r"^(src/Python/garminconnect|unittests/buildguard)/.*\.py$"


def make_git_repo(tmp_path: Path, files: dict[str, str]) -> Path:
    """A REAL git repo with REAL staged files.

    `git ls-files` reads the INDEX, which is precisely the domain pre-commit
    operates on, so the fixture has to be a real index and not a directory
    walk. An untracked file is deliberately NOT in scope for either.
    """
    root = tmp_path / "repo"
    root.mkdir()
    subprocess.run(["git", "init", "-q"], cwd=root, check=True)
    for rel, body in files.items():
        target = root / rel
        target.parent.mkdir(parents=True, exist_ok=True)
        target.write_text(body, encoding="utf-8")
        subprocess.run(["git", "add", "--", rel], cwd=root, check=True)
    return root


# --------------------------------------------------------------------------
# config parsing — the artifact that actually runs
# --------------------------------------------------------------------------
def test_load_hooks_reads_the_real_files_regex(tmp_path: Path) -> None:
    hooks = load_hooks(write_config(tmp_path, SIMPLE_CONFIG))
    assert [h.hook_id for h in hooks] == ["ruff"]
    assert hooks[0].files == r"^pkg/.*\.py$"


def test_a_config_that_is_not_valid_yaml_is_unusable_not_empty(
    tmp_path: Path,
) -> None:
    """The load-bearing negative: a broken config must NOT read as 'no hooks'.

    'No hooks' would make every file unowned and the guard would go loudly red
    for the wrong reason; worse, an empty-but-valid document would make the
    gap declarations look stale and pass. Unusable input must raise, so main()
    can exit 2 rather than render a verdict it cannot support.
    """
    with pytest.raises(ConfigError):
        load_hooks(write_config(tmp_path, "repos: [ this is not: valid: yaml"))


def test_a_missing_config_is_unusable(tmp_path: Path) -> None:
    with pytest.raises(ConfigError):
        load_hooks(tmp_path / "nope.yaml")


def test_a_config_with_no_repos_key_is_unusable(tmp_path: Path) -> None:
    with pytest.raises(ConfigError):
        load_hooks(write_config(tmp_path, "default_install_hook_types: [pre-commit]\n"))


# --------------------------------------------------------------------------
# matching semantics — pre-commit uses re.SEARCH, and that is load-bearing
# --------------------------------------------------------------------------
def test_matching_is_search_not_match() -> None:
    """pre-commit filters with `re.search`. An unanchored pattern matches mid-path.

    If this guard used `re.match` it would under-report coverage; if it used
    `fullmatch` it would under-report harder. Either way it would go red on a
    file that IS linted, and the fix a reader would reach for is widening a
    regex that was already correct.
    """
    hook = Hook(hook_id="h", name="h", files=r"buildguard/.*\.py$", exclude="^$")
    assert hooks_owning("unittests/buildguard/x.py", [hook]) == ["h"]


def test_an_empty_files_regex_matches_everything_as_pre_commit_does() -> None:
    hook = Hook(hook_id="h", name="h", files="", exclude="^$")
    assert hooks_owning("anything/at/all.py", [hook]) == ["h"]


def test_exclude_is_modelled_and_removes_ownership() -> None:
    """A file matched by `files` but knocked out by `exclude` is NOT owned."""
    hook = Hook(hook_id="h", name="h", files=r"\.py$", exclude=r"^vendor/")
    assert hooks_owning("pkg/a.py", [hook]) == ["h"]
    assert hooks_owning("vendor/a.py", [hook]) == []


def test_default_exclude_matches_nothing() -> None:
    hook = Hook(hook_id="h", name="h", files=r"\.py$", exclude="^$")
    assert hooks_owning("pkg/a.py", [hook]) == ["h"]


def test_a_hook_that_never_receives_filenames_does_not_own_anything() -> None:
    """`pass_filenames: false` is a TRIGGER, not a scope — the anti-vacuity rule.

    ledger-drift-lint matches governance files and then scans the whole tree
    itself; it is never handed the file. Counting such a hook as an owner
    would mean a single broad `pass_filenames: false` hook could satisfy
    ownership for the entire repository while linting none of it — this
    guard's own version of the skip-masking bypass Part A had to close.
    """
    hook = Hook(
        hook_id="ledger", name="ledger", files="", exclude="^$", passes_filenames=False
    )
    assert hooks_owning("anything.py", [hook]) == []


def _hook_config(extra: str) -> str:
    return (
        "repos:\n"
        "  - repo: local\n"
        "    hooks:\n"
        "      - id: h\n"
        "        files: '.*'\n" + extra
    )


def test_a_known_narrowing_key_is_recorded_not_ignored(tmp_path: Path) -> None:
    """`types:` narrows further than `files:`, and this guard does not model it.

    Recording it is the honest branch: silently ignoring it would let the
    guard certify a file as owned when the hook's type filter drops it —
    over-claiming coverage, which is the precise failure mode being abolished.
    """
    hooks = load_hooks(
        write_config(tmp_path, _hook_config("        types: [python]\n"))
    )
    assert hooks[0].unmodelled_keys == ("types",)
    findings = check_model_is_faithful(hooks)
    assert len(findings) == 1
    assert "LINT-MODEL" in findings[0]
    assert "NARROW" in findings[0]


def test_an_INVENTED_key_this_guard_has_never_heard_of_also_fails(
    tmp_path: Path,
) -> None:
    """R1-F1's sibling and the whole point of the allow-list inversion.

    The first version enumerated four known-bad spellings and ignored the
    rest, so a pre-commit release adding a new selection key would slip past:
    the file would be marked owned, LINT-MODEL would pass, and pre-commit
    would drop the file anyway. That is an OPT-IN check — B-STAGE9-16's own
    defect reproduced inside the guard built to abolish it.

    `exclude_tags` does not exist in pre-commit 4.2.0. That is exactly why it
    is the right probe: the guard must fail on a key it has never heard of,
    not merely on the ones its author remembered.
    """
    hooks = load_hooks(
        write_config(tmp_path, _hook_config("        exclude_tags: [slow]\n"))
    )
    assert hooks[0].unmodelled_keys == ("exclude_tags",)
    findings = check_model_is_faithful(hooks)
    assert len(findings) == 1
    assert "LINT-MODEL" in findings[0]
    assert "UNKNOWN" in findings[0]


def test_a_fully_modelled_hook_produces_no_model_finding(tmp_path: Path) -> None:
    """The allow-list must not cry wolf on ordinary, selection-neutral keys."""
    hooks = load_hooks(
        write_config(
            tmp_path,
            _hook_config(
                "        name: h\n"
                "        entry: true\n"
                "        language: system\n"
                "        args: [--fix]\n"
                "        exclude: '^$'\n"
                "        pass_filenames: true\n"
                "        always_run: false\n"
            ),
        )
    )
    assert hooks[0].unmodelled_keys == ()
    assert check_model_is_faithful(hooks) == []


def test_the_live_config_is_fully_modelled() -> None:
    """If this ever fails, the guard has stopped understanding its own input."""
    hooks = load_hooks(REPO_ROOT / ".pre-commit-config.yaml")
    assert check_model_is_faithful(hooks) == []


def test_the_allow_list_excludes_every_UNMODELLED_narrowing_key() -> None:
    """The inversion is only safe if the UNMODELLED narrowing keys stayed OUT.

    `stages` deliberately left KNOWN_NARROWING_KEYS in round 2: it is now
    modelled exactly rather than merely recognised. The three TYPE filters are
    still not modelled and so must still fail closed.

    Note what this test does NOT do: assert membership and stop. Moving a key
    into the allow-list is only safe if something actually ENFORCES it, and an
    allow-list entry with no enforcement behind it would be a hole dressed as a
    model — so the enforcement is asserted separately and directly by
    `test_a_hook_that_does_not_run_at_the_pre_commit_stage_owns_nothing`.
    """
    assert MODELLED_HOOK_KEYS.isdisjoint(KNOWN_NARROWING_KEYS)
    assert {"types", "types_or", "exclude_types"} <= KNOWN_NARROWING_KEYS
    assert "stages" in MODELLED_HOOK_KEYS
    assert "stages" not in KNOWN_NARROWING_KEYS


def test_the_keys_the_guard_actually_evaluates_are_in_the_allow_list() -> None:
    assert {"files", "exclude", "pass_filenames"} <= MODELLED_HOOK_KEYS


# --------------------------------------------------------------------------
# selection is an INTERSECTION of layers, not a fallback chain
# --------------------------------------------------------------------------
def _layered_config(
    top: str, hook_files: str = HOOK_OWN_FILES, hook_exclude: str = ""
) -> str:
    own_exclude = f"        exclude: '{hook_exclude}'\n" if hook_exclude else ""
    return (
        f"{top}"
        f"repos:\n"
        f"  - repo: https://example.invalid/ruff\n"
        f"    rev: v0.6.9\n"
        f"    hooks:\n"
        f"      - id: ruff\n"
        f"        files: '{hook_files}'\n"
        f"{own_exclude}"
    )


def test_a_top_level_files_regex_INTERSECTS_it_is_not_a_default(tmp_path: Path) -> None:
    """A hook declaring its own `files:` must still be cut by the global one."""
    hooks = load_hooks(write_config(tmp_path, _layered_config("files: '^src/'\n")))
    assert hooks_owning("src/Python/garminconnect/a.py", hooks) == ["ruff"]
    assert hooks_owning("unittests/buildguard/a.py", hooks) == []


def test_a_top_level_exclude_removes_ownership_the_hook_would_grant(
    tmp_path: Path,
) -> None:
    """The hook MUST declare its own `exclude:` for this to test anything.

    Without one, the old fallback inherited the global exclude and reached the
    right answer by accident — so a fixture omitting it passes against the
    defective code and proves nothing.
    """
    hooks = load_hooks(
        write_config(
            tmp_path, _layered_config("exclude: '^unittests/'\n", hook_exclude="^$")
        )
    )
    assert hooks[0].exclude == "^$"
    assert hooks[0].global_exclude == "^unittests/"
    assert hooks_owning("src/Python/garminconnect/a.py", hooks) == ["ruff"]
    assert hooks_owning("unittests/buildguard/a.py", hooks) == []


def test_a_top_level_exclude_applies_when_the_hook_declares_none(
    tmp_path: Path,
) -> None:
    hooks = load_hooks(
        write_config(tmp_path, _layered_config("exclude: '^unittests/'\n"))
    )
    assert hooks_owning("unittests/buildguard/a.py", hooks) == []


def test_absent_top_level_filters_constrain_nothing(tmp_path: Path) -> None:
    """The no-false-positive direction: no globals must mean no narrowing."""
    hooks = load_hooks(write_config(tmp_path, _layered_config("")))
    assert hooks[0].global_files == ""
    assert hooks[0].global_exclude == "^$"
    assert hooks_owning("unittests/buildguard/a.py", hooks) == ["ruff"]


def test_the_two_files_layers_are_never_merged_into_one(tmp_path: Path) -> None:
    """The structural guarantee: a hook's own regex is not overwritten."""
    hooks = load_hooks(write_config(tmp_path, _layered_config("files: '^src/'\n")))
    assert hooks[0].files == HOOK_OWN_FILES
    assert hooks[0].global_files == "^src/"


def test_a_hook_with_no_files_of_its_own_still_gets_the_global(
    tmp_path: Path,
) -> None:
    """Hook default is '' (everything); the global must still apply."""
    cfg = (
        "files: '^src/'\n"
        "repos:\n"
        "  - repo: local\n"
        "    hooks:\n"
        "      - id: h\n"
        "        name: h\n"
        "        entry: true\n"
        "        language: system\n"
    )
    hooks = load_hooks(write_config(tmp_path, cfg))
    assert hooks[0].files == ""
    assert hooks_owning("src/a.py", hooks) == ["h"]
    assert hooks_owning("unittests/buildguard/a.py", hooks) == []


def test_an_invalid_top_level_regex_is_unusable_not_ignored(tmp_path: Path) -> None:
    with pytest.raises(ConfigError):
        load_hooks(write_config(tmp_path, _layered_config("files: '^(src/'\n")))


def test_the_finding_names_the_TOP_LEVEL_layer_that_dropped_the_file(
    tmp_path: Path,
) -> None:
    """Pointing a reader at the per-hook regex here would be misdirection."""
    root = make_git_repo(tmp_path, {"pkg/a.py": "x\n"})
    hooks = load_hooks(write_config(tmp_path, _layered_config("files: '^src/'\n")))
    findings, _ = check_ownership(root, [_root()], hooks)
    assert len(findings) == 1
    assert "TOP-LEVEL" in findings[0]
    assert "Widening a per-hook files: regex will NOT help" in findings[0]


# --------------------------------------------------------------------------
# stage selection — modelled at BOTH levels
# --------------------------------------------------------------------------
def test_a_hook_that_does_not_run_at_the_pre_commit_stage_owns_nothing() -> None:
    """Enforcement behind `stages` being in MODELLED_HOOK_KEYS."""
    hook = Hook(hook_id="h", name="h", files="", exclude="^$", stages=("pre-push",))
    assert hooks_owning("anything.py", [hook]) == []


def test_default_stages_pre_push_removes_ownership_from_every_hook(
    tmp_path: Path,
) -> None:
    """A whole commit-time gate disabled without touching one regex."""
    hooks = load_hooks(
        write_config(tmp_path, _layered_config("default_stages: [pre-push]\n"))
    )
    assert hooks[0].stages == ("pre-push",)
    assert hooks_owning("src/Python/garminconnect/a.py", hooks) == []


def test_absent_default_stages_means_every_stage(tmp_path: Path) -> None:
    hooks = load_hooks(write_config(tmp_path, _layered_config("")))
    assert hooks[0].stages == guard.ALL_STAGES
    assert guard.EVALUATED_STAGE in hooks[0].stages


def test_a_per_hook_stages_overrides_the_top_level_default(tmp_path: Path) -> None:
    cfg = (
        "default_stages: [pre-push]\n"
        "repos:\n"
        "  - repo: local\n"
        "    hooks:\n"
        "      - id: h\n"
        "        files: '.*'\n"
        "        stages: [pre-commit]\n"
    )
    hooks = load_hooks(write_config(tmp_path, cfg))
    assert hooks_owning("a.py", hooks) == ["h"]


def test_legacy_stage_spellings_are_migrated_not_misread(tmp_path: Path) -> None:
    """`commit` is the deprecated spelling of `pre-commit`; misreading it
    would emit a FALSE finding against a legitimate config."""
    hooks = load_hooks(
        write_config(tmp_path, _layered_config("default_stages: [commit]\n"))
    )
    assert hooks[0].stages == ("pre-commit",)
    assert hooks_owning("src/Python/garminconnect/a.py", hooks) == ["ruff"]


def test_an_unrecognised_stage_name_fails_closed(tmp_path: Path) -> None:
    hooks = load_hooks(
        write_config(tmp_path, _layered_config("default_stages: [some-new-stage]\n"))
    )
    assert hooks_owning("src/Python/garminconnect/a.py", hooks) == []


def test_all_stages_matches_the_installed_pre_commit() -> None:
    """Pinned against the library so an upgrade cannot rot the tuple silently."""
    clientlib = pytest.importorskip("pre_commit.clientlib")
    assert set(guard.ALL_STAGES) == set(clientlib.STAGES)


def test_the_legacy_stage_aliases_match_the_installed_pre_commit() -> None:
    clientlib = pytest.importorskip("pre_commit.clientlib")
    assert guard.LEGACY_STAGE_ALIASES == clientlib._STAGES


# --------------------------------------------------------------------------
# LINT-MODEL at the TOP and REPO levels — fail closed one level further out
# --------------------------------------------------------------------------
def test_an_unknown_top_level_key_fails_closed(tmp_path: Path) -> None:
    cfg = _layered_config("brand_new_upstream_key: [whatever]\n")
    top = guard.load_config(write_config(tmp_path, cfg)).top_level
    findings = guard.check_top_level_is_modelled(top)
    assert len(findings) == 1
    assert "LINT-MODEL top-level 'brand_new_upstream_key'" in findings[0]


def test_every_modelled_top_level_key_is_accepted(tmp_path: Path) -> None:
    """The allow-list must not cry wolf on the keys it claims to model."""
    cfg = (
        "files: '^src/'\n"
        "exclude: '^$'\n"
        "default_stages: [pre-commit]\n"
        "default_install_hook_types: [pre-commit]\n"
        "default_language_version: {}\n"
        "fail_fast: false\n"
        "minimum_pre_commit_version: '0'\n"
        "ci: {autoupdate_schedule: monthly}\n"
        "repos:\n"
        "  - repo: local\n"
        "    hooks:\n"
        "      - id: h\n"
        "        files: '.*'\n"
    )
    top = guard.load_config(write_config(tmp_path, cfg)).top_level
    assert guard.check_top_level_is_modelled(top) == []


def test_an_unknown_repo_level_key_fails_closed(tmp_path: Path) -> None:
    cfg = (
        "repos:\n"
        "  - repo: local\n"
        "    unexpected_repo_key: true\n"
        "    hooks:\n"
        "      - id: h\n"
        "        files: '.*'\n"
    )
    top = guard.load_config(write_config(tmp_path, cfg)).top_level
    findings = guard.check_top_level_is_modelled(top)
    assert len(findings) == 1
    assert "unexpected_repo_key" in findings[0]


def test_the_live_config_top_level_is_fully_modelled() -> None:
    top = guard.load_config(REPO_ROOT / ".pre-commit-config.yaml").top_level
    assert guard.check_top_level_is_modelled(top) == []


def test_the_top_level_allow_list_matches_the_installed_pre_commit() -> None:
    """The allow-list is pinned to the real schema, so a pre-commit upgrade
    adding a key goes RED here instead of widening the blind spot."""
    cfgv = pytest.importorskip("cfgv")
    clientlib = pytest.importorskip("pre_commit.clientlib")
    allowed: set[str] = set()
    for item in clientlib.CONFIG_SCHEMA.items:
        if isinstance(item, cfgv.WarnAdditionalKeys):
            allowed |= set(item.keys)
    assert set(guard.MODELLED_TOP_LEVEL_KEYS) == allowed


def test_the_repo_level_allow_list_matches_the_installed_pre_commit() -> None:
    cfgv = pytest.importorskip("cfgv")
    clientlib = pytest.importorskip("pre_commit.clientlib")
    allowed: set[str] = set()
    for item in clientlib.CONFIG_REPO_DICT.items:
        if isinstance(item, cfgv.WarnAdditionalKeys):
            allowed |= set(item.keys)
    assert set(guard.MODELLED_REPO_KEYS) == allowed


def test_every_modelled_top_level_key_carries_a_substantive_reason() -> None:
    """Same floor a declared Gap must meet: the reasoning is the point."""
    for key, reason in guard.MODELLED_TOP_LEVEL_KEYS.items():
        assert len(reason.strip()) >= MIN_REASON_CHARS, key


# --------------------------------------------------------------------------
# LINT-VACUOUS — the guard must have examined something (R1-F1)
# --------------------------------------------------------------------------
def test_an_all_empty_root_set_is_a_finding_not_a_pass() -> None:
    """The exact round-1 defect, at the helper level."""
    reports = [RootReport(root=_root())]
    findings = check_not_vacuous(reports)
    assert len(findings) == 1 and "LINT-VACUOUS" in findings[0]


def test_inventoried_but_nothing_owned_is_also_vacuous() -> None:
    """The second prong: inventory alone proves no ownership."""
    report = RootReport(root=_root())
    report.all_files = ["pkg/a.py"]
    report.gapped = ["pkg/a.py"]
    findings = check_not_vacuous([report])
    assert len(findings) == 1 and "NOT ONE is matched" in findings[0]


def test_one_covered_file_is_enough_to_be_non_vacuous() -> None:
    report = RootReport(root=_root())
    report.all_files = ["pkg/a.py"]
    report.covered = ["pkg/a.py"]
    assert check_not_vacuous([report]) == []


def test_a_single_empty_root_alongside_a_covered_one_is_still_fine() -> None:
    """stderrbuf's pre-armed emptiness must keep costing nothing (§6.1 ruling)."""
    empty = RootReport(root=_root(name="armed-early"))
    real = RootReport(root=_root())
    real.all_files = ["pkg/a.py"]
    real.covered = ["pkg/a.py"]
    assert check_not_vacuous([empty, real]) == []


def test_the_live_repo_is_not_vacuous() -> None:
    hooks = load_hooks(REPO_ROOT / ".pre-commit-config.yaml")
    _, reports = check_ownership(REPO_ROOT, MANAGED_ROOTS, hooks)
    assert check_not_vacuous(reports) == []


def test_main_fails_closed_on_a_bootstrap_index(tmp_path: Path) -> None:
    """END TO END, through the command CTest registers — not a helper.

    The round-1 finding was precisely that the non-vacuity assertion lived in
    a test around a helper and so did not protect `main()`. Asserting it
    anywhere else would repeat the mistake.
    """
    root = make_git_repo(tmp_path, {"readme.txt": "x\n"})
    (root / ".pre-commit-config.yaml").write_text(
        (REPO_ROOT / ".pre-commit-config.yaml").read_text(encoding="utf-8"),
        encoding="utf-8",
    )
    subprocess.run(
        ["git", "add", "--", ".pre-commit-config.yaml"], cwd=root, check=True
    )
    assert guard.main(["prog", str(root)]) == 1


# --------------------------------------------------------------------------
# file enumeration — the git index, not a directory walk
# --------------------------------------------------------------------------
def test_tracked_files_reads_the_index_and_ignores_untracked_noise(
    tmp_path: Path,
) -> None:
    """A directory walk would pick up __pycache__ and report phantom findings."""
    root = make_git_repo(tmp_path, {"pkg/a.py": "x = 1\n"})
    (root / "pkg" / "__pycache__").mkdir()
    (root / "pkg" / "__pycache__" / "a.pyc").write_bytes(b"\x00")
    (root / "pkg" / "untracked.py").write_text("y = 2\n", encoding="utf-8")
    assert tracked_files(root, ("pkg/*",)) == ["pkg/a.py"]


def test_tracked_files_sees_a_newly_staged_never_committed_file(
    tmp_path: Path,
) -> None:
    """The index IS pre-commit's input domain, so a `git add`ed file counts.

    This is the case that makes the guard catch the real defect: a brand-new
    directory someone stages for the FIRST time is in scope immediately, not
    only once it is committed. If this read committed trees instead of the
    index, the guard would notice an unowned new file exactly one commit too
    late — which is precisely how `unittests/buildguard/` got in.

    Nothing is committed in this fixture at all, deliberately: HEAD does not
    even exist, and the file must still be found.
    """
    root = make_git_repo(tmp_path, {"pkg/a.py": "x = 1\n"})
    assert tracked_files(root, ("pkg/*",)) == ["pkg/a.py"]

    (root / "pkg" / "brand_new.py").write_text("z = 3\n", encoding="utf-8")
    assert tracked_files(root, ("pkg/*",)) == [
        "pkg/a.py"
    ], "an unstaged file must NOT be in scope — pre-commit never sees it"

    subprocess.run(["git", "add", "--", "pkg/brand_new.py"], cwd=root, check=True)
    assert tracked_files(root, ("pkg/*",)) == ["pkg/a.py", "pkg/brand_new.py"]


def test_not_a_git_repo_is_unusable_never_a_silent_pass(tmp_path: Path) -> None:
    with pytest.raises(ConfigError):
        tracked_files(tmp_path, ("pkg/*",))


# --------------------------------------------------------------------------
# LINT-OWNERSHIP — the core rule, and its red
# --------------------------------------------------------------------------
def _root(**kw: object) -> ManagedRoot:
    base: dict = dict(name="demo", patterns=("pkg/*",), owned=(".py",), gaps=())
    base.update(kw)
    return ManagedRoot(**base)  # type: ignore[arg-type]


def test_a_covered_file_produces_no_finding(tmp_path: Path) -> None:
    root = make_git_repo(tmp_path, {"pkg/a.py": "x = 1\n"})
    hooks = [Hook(hook_id="ruff", name="ruff", files=r"^pkg/.*\.py$", exclude="^$")]
    findings, report = check_ownership(root, [_root()], hooks)
    assert findings == []
    assert report[0].covered == ["pkg/a.py"]


def test_an_unowned_file_is_a_finding(tmp_path: Path) -> None:
    """The whole point: a real managed file matched by no hook must go RED."""
    root = make_git_repo(tmp_path, {"pkg/a.py": "x = 1\n"})
    hooks = [Hook(hook_id="ruff", name="ruff", files=r"^other/.*\.py$", exclude="^$")]
    findings, _ = check_ownership(root, [_root()], hooks)
    assert len(findings) == 1
    assert "LINT-OWNERSHIP" in findings[0]
    assert "pkg/a.py" in findings[0]


def test_narrowing_a_regex_so_one_file_falls_out_goes_red(tmp_path: Path) -> None:
    """The mutation §7 requires, as a unit test: narrow the regex, lose a file."""
    root = make_git_repo(tmp_path, {"pkg/a.py": "x = 1\n", "pkg/b.py": "y = 2\n"})
    wide = [Hook(hook_id="ruff", name="ruff", files=r"^pkg/.*\.py$", exclude="^$")]
    assert check_ownership(root, [_root()], wide)[0] == []

    narrow = [Hook(hook_id="ruff", name="ruff", files=r"^pkg/a\.py$", exclude="^$")]
    findings, _ = check_ownership(root, [_root()], narrow)
    assert len(findings) == 1 and "pkg/b.py" in findings[0]


def test_a_suffix_outside_owned_is_not_demanded(tmp_path: Path) -> None:
    root = make_git_repo(tmp_path, {"pkg/a.py": "x = 1\n", "pkg/notes.md": "hi\n"})
    hooks = [Hook(hook_id="ruff", name="ruff", files=r"^pkg/.*\.py$", exclude="^$")]
    findings, report = check_ownership(root, [_root()], hooks)
    assert findings == []
    assert report[0].undeclared == ["pkg/notes.md"]


def test_a_file_kind_that_is_neither_owned_nor_declared_is_reported(
    tmp_path: Path,
) -> None:
    """Silence is the defect: an unclassified kind must be visible somewhere."""
    root = make_git_repo(tmp_path, {"pkg/a.py": "x\n", "pkg/run.sh": "echo\n"})
    hooks = [Hook(hook_id="ruff", name="ruff", files=r"^pkg/.*\.py$", exclude="^$")]
    _, report = check_ownership(root, [_root()], hooks)
    assert "pkg/run.sh" in report[0].undeclared


# --------------------------------------------------------------------------
# the EMPTY declared root — the vacuity trap in miniature
# --------------------------------------------------------------------------
def test_an_empty_root_does_not_crash_and_does_not_go_red(tmp_path: Path) -> None:
    """DEC-054 arms clang-format for a directory that has no tracked file yet.

    That must not be an error (the cost of arming early is meant to be zero)
    and must not be silently counted as coverage either.
    """
    root = make_git_repo(tmp_path, {"other/a.py": "x\n"})
    findings, report = check_ownership(root, [_root()], [])
    assert findings == []
    assert report[0].is_empty is True


def test_an_empty_root_is_reported_distinctly_from_a_covered_one(
    tmp_path: Path,
) -> None:
    """`covered` must not be able to impersonate `empty` in the output."""
    root = make_git_repo(tmp_path, {"other/a.py": "x\n"})
    _, report = check_ownership(root, [_root()], [])
    assert report[0].covered == []
    line = guard.format_root_report(report[0])
    assert "EMPTY" in line
    assert "0 tracked" in line


def test_a_non_empty_root_never_claims_to_be_empty(tmp_path: Path) -> None:
    root = make_git_repo(tmp_path, {"pkg/a.py": "x\n"})
    hooks = [Hook(hook_id="ruff", name="ruff", files=r"^pkg/.*\.py$", exclude="^$")]
    _, report = check_ownership(root, [_root()], hooks)
    assert report[0].is_empty is False
    assert "EMPTY" not in guard.format_root_report(report[0])


# --------------------------------------------------------------------------
# LINT-GAP — declared gaps must be reasoned, and must still be TRUE
# --------------------------------------------------------------------------
GOOD_REASON = (
    "No shell linter exists in DEC-009's tool set and shellcheck is not "
    "installed on this host, so the arrival state cannot be measured."
)


def test_a_declared_suffix_gap_suppresses_the_finding(tmp_path: Path) -> None:
    root = make_git_repo(tmp_path, {"pkg/run.sh": "echo\n"})
    gaps = (Gap(kind="suffix", subject=".sh", reason=GOOD_REASON),)
    findings, report = check_ownership(root, [_root(owned=(".sh",), gaps=gaps)], [])
    assert findings == []
    assert report[0].gapped == ["pkg/run.sh"]


def test_a_gap_reason_that_says_nothing_is_a_finding() -> None:
    gaps = (Gap(kind="suffix", subject=".sh", reason="slow"),)
    findings = check_declared_gaps([_root(gaps=gaps)], [], {})
    assert any("LINT-GAP" in f and "reason" in f for f in findings)


def test_the_reason_floor_matches_part_as(tmp_path: Path) -> None:
    assert MIN_REASON_CHARS >= 40


def test_a_stale_gap_declaration_is_a_finding() -> None:
    """A gap that is no longer true is a LIE left in the tree.

    If someone widens mypy to unittests/buildguard/, the recorded "mypy
    deliberately does not cover this" must fail rather than quietly outlive
    its own truth. This is GATE-EXCUSE's discipline applied to the lint half:
    the opt-out list is the one place this scheme can still rot, so it is the
    place that is policed.
    """
    gaps = (Gap(kind="tool", subject="mypy", reason=GOOD_REASON),)
    owners = {"pkg/a.py": ["ruff", "mypy"]}
    findings = check_declared_gaps([_root(gaps=gaps)], ["pkg/a.py"], owners)
    assert len(findings) == 1
    assert "LINT-GAP" in findings[0] and "stale" in findings[0].lower()


def test_a_tool_gap_that_is_still_true_is_clean() -> None:
    gaps = (Gap(kind="tool", subject="mypy", reason=GOOD_REASON),)
    owners = {"pkg/a.py": ["ruff"]}
    assert check_declared_gaps([_root(gaps=gaps)], ["pkg/a.py"], owners) == []


def test_a_stale_suffix_gap_is_a_finding() -> None:
    gaps = (Gap(kind="suffix", subject=".py", reason=GOOD_REASON),)
    owners = {"pkg/a.py": ["ruff"]}
    findings = check_declared_gaps([_root(gaps=gaps)], ["pkg/a.py"], owners)
    assert len(findings) == 1 and "stale" in findings[0].lower()


def test_a_stale_path_gap_is_a_finding() -> None:
    gaps = (Gap(kind="path", subject="pkg/a.py", reason=GOOD_REASON),)
    owners = {"pkg/a.py": ["clang-format"]}
    findings = check_declared_gaps([_root(gaps=gaps)], ["pkg/a.py"], owners)
    assert len(findings) == 1 and "stale" in findings[0].lower()


def test_a_path_gap_whose_subject_no_longer_EXISTS_is_stale() -> None:
    """R1-F3: a declaration may not outlive the file it excused.

    Before the fix, an absent subject was simply missing from owners_by_file,
    `not owners_by_file.get(subject)` read as true, and a dead declaration kept
    printing in the PASS output as a live, re-checked exception. Deleting the
    file it named was enough to turn it into residue that nothing could
    detect.
    """
    gaps = (Gap(kind="path", subject="pkg/gone.cpp", reason=GOOD_REASON),)
    findings = check_declared_gaps([_root(gaps=gaps)], ["pkg/a.py"], {"pkg/a.py": []})
    assert len(findings) == 1
    assert "STALE" in findings[0]
    assert "no longer a tracked file" in findings[0]


def test_a_path_gap_whose_subject_exists_and_is_unowned_is_still_true() -> None:
    gaps = (Gap(kind="path", subject="pkg/a.py", reason=GOOD_REASON),)
    assert check_declared_gaps([_root(gaps=gaps)], ["pkg/a.py"], {"pkg/a.py": []}) == []


def test_a_suffix_gap_survives_its_class_being_EMPTY() -> None:
    """The deliberate counterpart to the rule above (R1-F3's second half).

    A suffix gap is a STANDING POLICY about a class of file, not a claim about
    one artifact. Deleting the last `.sh` must not force a policy edit, and
    re-adding one must not require the policy to be re-derived. Asserted so
    the asymmetry with `path` is a decision on the record rather than an
    accident of the code.
    """
    gaps = (Gap(kind="suffix", subject=".sh", reason=GOOD_REASON),)
    assert check_declared_gaps([_root(gaps=gaps)], ["pkg/a.py"], {"pkg/a.py": []}) == []


def test_a_tool_gap_survives_an_empty_root() -> None:
    gaps = (Gap(kind="tool", subject="mypy", reason=GOOD_REASON),)
    assert check_declared_gaps([_root(gaps=gaps)], [], {}) == []


def test_an_unknown_gap_kind_is_rejected() -> None:
    gaps = (Gap(kind="vibes", subject=".sh", reason=GOOD_REASON),)
    findings = check_declared_gaps([_root(gaps=gaps)], [], {})
    assert any("LINT-GAP" in f for f in findings)


# --------------------------------------------------------------------------
# the LIVE config — the artifact that actually runs, not a fixture of it
# --------------------------------------------------------------------------
def test_the_live_config_parses_and_declares_hooks() -> None:
    hooks = load_hooks(REPO_ROOT / ".pre-commit-config.yaml")
    ids = [h.hook_id for h in hooks]
    assert {"clang-format", "ruff", "ruff-format", "mypy"} <= set(ids)


def test_the_live_config_now_covers_buildguard_python() -> None:
    """The regression B-STAGE9-16 Part B exists to fix, asserted directly."""
    hooks = load_hooks(REPO_ROOT / ".pre-commit-config.yaml")
    owners = hooks_owning("unittests/buildguard/garmin_gate_coverage_guard.py", hooks)
    assert "ruff" in owners
    assert "ruff-format" in owners


def test_the_live_config_deliberately_does_not_mypy_buildguard() -> None:
    """DEC-054's sanctioned branch, asserted so a silent widening is caught.

    If someone widens mypy here, this test fails AND the stale-gap rule fires.
    Both point at the same edit, which is the intent: the declaration and the
    config cannot drift apart without something going red.
    """
    hooks = load_hooks(REPO_ROOT / ".pre-commit-config.yaml")
    owners = hooks_owning("unittests/buildguard/garmin_gate_coverage_guard.py", hooks)
    assert "mypy" not in owners


def test_the_live_config_still_does_not_lint_the_legacy_tree() -> None:
    """DEC-010's PRINCIPLE, which this unit widens the enumeration of but keeps."""
    hooks = load_hooks(REPO_ROOT / ".pre-commit-config.yaml")
    for legacy in (
        "src/Gui/MainWindow.cpp",
        "src/Train/KurtInRide.cpp",
        "src/FileIO/RideFile.h",
    ):
        assert hooks_owning(legacy, hooks) == [], legacy


def test_the_guard_passes_against_the_live_repository() -> None:
    """End to end, against the real tree — and NOT vacuously.

    The paired assertion is the one that matters: a PASS here is only
    meaningful if at least one root really examined files.
    """
    hooks = load_hooks(REPO_ROOT / ".pre-commit-config.yaml")
    findings, report = check_ownership(REPO_ROOT, MANAGED_ROOTS, hooks)
    assert findings == [], findings
    assert sum(len(r.covered) for r in report) > 0


def test_the_live_declared_gaps_are_all_still_true() -> None:
    hooks = load_hooks(REPO_ROOT / ".pre-commit-config.yaml")
    _, report = check_ownership(REPO_ROOT, MANAGED_ROOTS, hooks)
    files = [f for r in report for f in r.all_files]
    owners = {f: hooks_owning(f, hooks) for f in files}
    assert check_declared_gaps(MANAGED_ROOTS, files, owners) == []


def test_every_managed_root_gap_carries_a_substantive_reason() -> None:
    for root in MANAGED_ROOTS:
        for gap in root.gaps:
            assert len(gap.reason.strip()) >= MIN_REASON_CHARS, (root.name, gap.subject)


def test_this_guards_own_source_and_test_are_lint_owned() -> None:
    """LINT-SELF's static half: the ownership guard must itself be linted.

    A lint-coverage guard that is not itself linted is the original bug
    wearing a new hat, the same argument Part A's GATE-SELF makes about the
    gate.
    """
    hooks = load_hooks(REPO_ROOT / ".pre-commit-config.yaml")
    for rel in (
        "unittests/buildguard/garmin_lint_ownership_guard.py",
        "unittests/buildguard/test_garmin_lint_ownership_guard.py",
    ):
        assert "ruff" in hooks_owning(rel, hooks), rel
        assert "ruff-format" in hooks_owning(rel, hooks), rel


# --------------------------------------------------------------------------
# main() — exit codes
# --------------------------------------------------------------------------
def _repo_with_live_config(tmp_path: Path, extra_top_level: str = "") -> Path:
    """A scratch repo carrying the REAL config plus managed files.

    Real managed paths, so MANAGED_ROOTS applies and the run is not vacuous.
    The three `path`-gap subjects are present because a declared path gap goes
    STALE without its file (R1-F3) — their absence would redden the control
    for an unrelated reason.
    """
    root = make_git_repo(
        tmp_path,
        {
            "src/Python/garminconnect/client.py": "x = 1\n",
            "unittests/buildguard/garmin_lint_ownership_guard.py": "y = 2\n",
            "unittests/buildguard/test_garmin_lint_ownership_guard.py": "z = 3\n",
            "src/Cloud/PyEmbeddedAdapter.cpp": "// x\n",
            "src/Cloud/AddCloudWizard.cpp": "// x\n",
            "src/Cloud/AddCloudWizard.h": "// x\n",
        },
    )
    body = (REPO_ROOT / ".pre-commit-config.yaml").read_text(encoding="utf-8")
    (root / ".pre-commit-config.yaml").write_text(
        extra_top_level + body, encoding="utf-8"
    )
    subprocess.run(
        ["git", "add", "--", ".pre-commit-config.yaml"], cwd=root, check=True
    )
    return root


def test_main_is_green_on_the_scratch_fixture_without_top_level_filters(
    tmp_path: Path,
) -> None:
    """The control. Without it the RED below proves nothing."""
    assert guard.main(["prog", str(_repo_with_live_config(tmp_path))]) == 0


def test_main_goes_RED_when_a_top_level_files_regex_hides_a_managed_file(
    tmp_path: Path, capsys: pytest.CaptureFixture
) -> None:
    """End to end, through the registered command: the reviewer's scenario."""
    root = _repo_with_live_config(tmp_path, "files: '^src/'\n")
    assert guard.main(["prog", str(root)]) == 1
    out = capsys.readouterr().out
    assert "LINT-OWNERSHIP" in out
    assert "unittests/buildguard/garmin_lint_ownership_guard.py" in out
    assert "TOP-LEVEL" in out


def test_main_goes_RED_when_a_top_level_exclude_hides_a_managed_file(
    tmp_path: Path,
) -> None:
    root = _repo_with_live_config(tmp_path, "exclude: '^unittests/'\n")
    assert guard.main(["prog", str(root)]) == 1


def test_main_goes_RED_when_default_stages_disables_the_commit_gate(
    tmp_path: Path, capsys: pytest.CaptureFixture
) -> None:
    """The gate silently moved off pre-commit; coverage must stop claiming it."""
    root = _repo_with_live_config(tmp_path, "default_stages: [pre-push]\n")
    assert guard.main(["prog", str(root)]) == 1
    assert "LINT-OWNERSHIP" in capsys.readouterr().out


def test_main_goes_RED_on_an_unknown_top_level_key(
    tmp_path: Path, capsys: pytest.CaptureFixture
) -> None:
    root = _repo_with_live_config(tmp_path, "brand_new_upstream_key: [x]\n")
    assert guard.main(["prog", str(root)]) == 1
    assert "LINT-MODEL top-level 'brand_new_upstream_key'" in capsys.readouterr().out


def test_the_vacuity_verdict_is_printed_BEFORE_the_findings_it_explains(
    tmp_path: Path, capsys: pytest.CaptureFixture
) -> None:
    """Item 3: the diagnosis must not arrive behind a wall of stale-gap noise."""
    root = make_git_repo(tmp_path, {"readme.txt": "x\n"})
    (root / ".pre-commit-config.yaml").write_text(
        (REPO_ROOT / ".pre-commit-config.yaml").read_text(encoding="utf-8"),
        encoding="utf-8",
    )
    subprocess.run(
        ["git", "add", "--", ".pre-commit-config.yaml"], cwd=root, check=True
    )
    assert guard.main(["prog", str(root)]) == 1
    out = capsys.readouterr().out
    assert "LINT-VACUOUS" in out and "LINT-GAP" in out
    assert out.index("LINT-VACUOUS") < out.index("LINT-GAP")
    assert out.index("READ THIS FIRST") < out.index("LINT-VACUOUS")


def test_a_non_vacuous_failure_carries_no_vacuity_banner(
    tmp_path: Path, capsys: pytest.CaptureFixture
) -> None:
    """The banner must mean something, so it may not print on every red run."""
    root = _repo_with_live_config(tmp_path, "files: '^src/'\n")
    assert guard.main(["prog", str(root)]) == 1
    assert "READ THIS FIRST" not in capsys.readouterr().out


def test_main_returns_2_on_bad_usage() -> None:
    assert guard.main(["prog"]) == 2


def test_main_returns_2_on_an_unusable_config(tmp_path: Path) -> None:
    root = make_git_repo(tmp_path, {"pkg/a.py": "x\n"})
    assert guard.main(["prog", str(root)]) == 2


def test_main_returns_0_against_the_live_repo(capsys: pytest.CaptureFixture) -> None:
    assert guard.main(["prog", str(REPO_ROOT)]) == 0
    out = capsys.readouterr().out
    assert "PASS" in out


def test_the_pass_line_names_the_gaps_it_is_not_covering(
    capsys: pytest.CaptureFixture,
) -> None:
    """6.4 / 6.5: the guard must SAY mypy and .sh are unowned, not omit them.

    A guard that reported full coverage while mypy silently skipped the
    directory would be lying in the exact register this finding exists to
    abolish.
    """
    guard.main(["prog", str(REPO_ROOT)])
    out = capsys.readouterr().out
    assert "mypy" in out
    assert ".sh" in out
    assert "DECLARED GAP" in out
