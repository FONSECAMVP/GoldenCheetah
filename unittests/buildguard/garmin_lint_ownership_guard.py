#!/usr/bin/env python3
"""B-STAGE9-16 / DEC-054 Part B static guard: every managed file is linted.

THE DEFECT THIS EXISTS TO ABOLISH, stated as a mechanism rather than a rule.
`.pre-commit-config.yaml` scopes every hook with an enumerated `files:` regex.
That enumeration was accurate on 2026-05-17 and then never grew. So when
`unittests/buildguard/` appeared it was linted by NOTHING — and because a
skipped hook and a passing hook look identical in a terminal, four months of
commits reported success. Commits `d9ba4faad` and `8611fb2d0` each staged new
buildguard Python and every single hook printed "(no files to check)Skipped".
`8611fb2d0` alone was ~1,950 unlinted lines.

That is the same shape as the ctest half DEC-054's Part A fixed: ENROLMENT WAS
THE VERIFICATION STEP, so forgetting to enrol produced a GREEN result instead
of a loud one. Part A inverted the ctest default to default-include. The lint
half cannot take that route — DEC-010 deliberately scoped pre-commit to new
Garmin paths so as not to retrofit the legacy GoldenCheetah tree, and THAT
REASONING IS STILL CORRECT. A whole-tree widening is not the fix.

So the enumeration stays, and this guard makes forgetting to grow it LOUD:

  LINT-OWNERSHIP  every tracked file under a declared managed root, whose file
                  kind is declared OWNED for that root, is matched by at least
                  one hook's `files:` regex in the live config.
  LINT-GAP        every DECLARED gap (a file kind, a specific path, or a whole
                  tool deliberately left unowned) carries a substantive reason
                  AND IS STILL TRUE. A gap that has quietly become false is a
                  lie left in the tree, so widening a regex without deleting
                  its gap declaration fails here.
  LINT-SELF       this guard's own source and tests are lint-owned, and its
                  ctest registration is inside the routine gate and carries no
                  `gate-exclude`. A lint-coverage guard that is itself unlinted
                  or ungated is the original bug wearing a new hat.
  LINT-MODEL      every key in the config — at all three nesting levels
                  (top-level, per-repo, per-hook) — is one this guard has
                  reasoned about. ALLOW-LIST: any unknown key fails, because an
                  unknown key may narrow selection and assuming otherwise
                  over-certifies coverage.
  LINT-VACUOUS    the run actually EXAMINED something: across all roots, at
                  least one managed file was inventoried and at least one was
                  proven owned. Without this the guard could print
                  `PASS — 0/0` on an empty index and exit 0 — a green no-op.
                  Individually empty roots stay fine; the rule is on the
                  aggregate.

WHICH ARTIFACTS THIS READS, and why not the ones that merely describe them.
Two inputs, both chosen to be the thing that actually runs:

  * `.pre-commit-config.yaml` itself, parsed with PyYAML, with the real
    `files:`/`exclude:` regexes evaluated by Python's `re`. NOT a second
    hand-written list of "what is covered" — that would reintroduce the exact
    two-lists-that-drift-apart defect being fixed here. pre-commit's own
    filter is `re.search(include, f) and not re.search(exclude, f)`, so this
    guard uses `re.search` too; `re.match` would silently under-report and
    send a reader off to widen a regex that was already correct.
  * `git ls-files` for the file inventory, i.e. THE INDEX. That is precisely
    pre-commit's input domain: it runs on staged files, so a `git add`ed file
    is in scope immediately and a file git has never heard of is in scope for
    neither. A directory walk was the obvious alternative and is wrong — it
    would sweep in `__pycache__/`, `.mypy_cache/` and `.pytest_cache/`, all of
    which exist in `unittests/buildguard/` right now, and report phantom
    findings for files no hook will ever be handed.

SELECTION IS AN INTERSECTION, NOT A FALLBACK CHAIN. pre-commit narrows the
candidate list in three passes, each of which can only remove files: top-level
`files:`/`exclude:` (`commands/run.py:290-292` via `Classifier.from_config`,
`:112-126`), then the hook's own (`filenames_for_hook`, `:100-110`), then the
type filters it does not model (below). `Hook` therefore holds the top-level
and per-hook regexes in separate fields and applies both. Stage selection is
part of this: a hook not running at EVALUATED_STAGE is dropped from the run
before selection (`commands/run.py:427`), and per-hook `stages:` falls back to
top-level `default_stages:` (`repository.py:123-124`) — a real fallback, unlike
`files:`. The tests pin each of these against the installed pre-commit rather
than asserting them here.

WHAT IS NOT MODELLED, declared rather than hidden. pre-commit intersects
`files:` with the hook's TYPE filters, and the upstream hook definitions carry
their own `types`/`types_or` (ruff ships `types_or: [python, pyi]`). This guard
evaluates only THIS repository's config, so what it proves is "the config
declares this file in scope" — which is exactly the thing DEC-010's stale
enumeration got wrong — and NOT "the tool definitely processed this file".
Read the promise that narrowly. Two mitigations keep the gap from widening:
LINT-MODEL fails if a narrowing key appears in this repo's own config, and the
end-to-end proof that a hook really runs on a buildguard file is a
`pre-commit run --files ...` transcript recorded with the change, not an
inference from this guard.

A hook with `pass_filenames: false` (ledger-drift-lint) OWNS NOTHING here. Its
`files:` regex is a TRIGGER — it decides when the hook runs, then the hook
scans the whole tree itself — so the file is never handed to it. Counting such
a hook as an owner would let one broad `pass_filenames: false` entry satisfy
ownership for the entire repository while linting none of it, which is this
guard's own version of the result-masking bypass Part A had to close.

Usage: garmin_lint_ownership_guard.py <source-root> [--build-dir DIR]
Exit 0 = PASS, 1 = findings (each printed as RULE detail), 2 = usage /
unusable config or index. Without --build-dir the LINT-SELF registry check
cannot run, and the PASS line says so rather than implying it passed.
"""

from __future__ import annotations

import fnmatch
import re
import subprocess
import sys
from dataclasses import dataclass, field
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))

CONFIG_FILENAME = ".pre-commit-config.yaml"

# Same floor as Part A's GATE-EXCLUDE_REASON, and for the same reason: it is a
# guard against `reason: "n/a"`, not a quality bar. No static check can decide
# whether a reason is TRUE; review is the main protection and this is the
# minimum that makes review possible.
MIN_REASON_CHARS = 40

# DEC-010's gate is the commit-time one; this is the stage the verdict is FOR.
EVALUATED_STAGE = "pre-commit"

# Used for one thing only: what an ABSENT `default_stages:` means.
ALL_STAGES = (
    "commit-msg",
    "post-checkout",
    "post-commit",
    "post-merge",
    "post-rewrite",
    "pre-commit",
    "pre-merge-commit",
    "pre-push",
    "pre-rebase",
    "prepare-commit-msg",
    "manual",
)

# pre-commit still accepts and migrates the pre-3.x stage spellings.
LEGACY_STAGE_ALIASES = {
    "commit": "pre-commit",
    "merge-commit": "pre-merge-commit",
    "push": "pre-push",
}


def transform_stage(stage: str) -> str:
    """pre-commit's own `clientlib.transform_stage`, reproduced exactly."""
    return LEGACY_STAGE_ALIASES.get(stage, stage)


# An ALLOW-LIST of per-hook keys this guard understands, and the reason it is
# an allow-list rather than the deny-list it started as.
#
# Round 1 review caught the first version enumerating the four narrowing keys
# it knew about ("types", "types_or", "exclude_types", "stages") and ignoring
# everything else. That is an OPT-IN check: it catches only what someone
# remembered to enumerate, so the day pre-commit adds a new selection key, a
# config using it would sail through — the guard would mark a file owned,
# LINT-MODEL would pass, and pre-commit would drop the file anyway. In other
# words it reproduced B-STAGE9-16's OWN defect inside the guard built to
# abolish it, and it would have gone stale in exactly the way DEC-010's
# enumeration did.
#
# So the gate is inverted: every key BELOW is one whose effect on FILE
# SELECTION this guard has reasoned about and knows to be nil. ANY other
# per-hook key — new, unknown, or narrowing — is a LINT-MODEL finding. Fail
# closed on the unknown.
MODELLED_HOOK_KEYS = frozenset(
    {
        # modelled directly by hooks_owning()
        "files",
        "exclude",
        "pass_filenames",
        # modelled by hooks_owning() via Hook.stages
        "stages",
        # identity / metadata: no effect on which files are selected
        "id",
        "alias",
        "name",
        "description",
        "minimum_pre_commit_version",
        # what the hook RUNS, not WHICH FILES reach it
        "entry",
        "language",
        "language_version",
        "additional_dependencies",
        "args",
        # execution/reporting behaviour, selection-neutral
        "verbose",
        "log_file",
        "require_serial",
        "fail_fast",
        # widens rather than narrows (runs even with no matching files), so it
        # cannot cause this guard to OVER-certify, which is the failure
        # direction LINT-MODEL exists to prevent
        "always_run",
    }
)

# NOT a gate — the gate is MODELLED_HOOK_KEYS above. Used only to sharpen the
# message when an unknown key is one we recognise as narrowing.
KNOWN_NARROWING_KEYS = frozenset({"types", "types_or", "exclude_types"})

# Top-level allow-list, same fail-closed principle as MODELLED_HOOK_KEYS. The
# reasons are DATA, not commentary: they are held to MIN_REASON_CHARS by
# test_every_modelled_top_level_key_carries_a_substantive_reason, and the key
# set is pinned against the installed pre-commit schema by
# test_the_top_level_allow_list_matches_the_installed_pre_commit. The question
# asked of each key: can it remove a file from the set handed to a hook at
# EVALUATED_STAGE?
MODELLED_TOP_LEVEL_KEYS: dict[str, str] = {
    "repos": (
        "MODELLED — the hooks themselves, parsed into Hook objects and held to "
        "MODELLED_HOOK_KEYS individually."
    ),
    "files": (
        "MODELLED as an intersection. Applied to the whole candidate list "
        "before any hook is consulted, so a file it drops reaches nothing; "
        "carried as Hook.global_files and applied in ADDITION to the hook's "
        "own files:, never instead of it."
    ),
    "exclude": (
        "MODELLED as an intersection, same mechanism as files: above. A file "
        "it matches is removed before any hook sees it, whatever that hook's "
        "own regexes say."
    ),
    "default_stages": (
        "MODELLED — the sharp one: it can disable the whole commit-time gate "
        "without touching a regex. A hook with no stages: of its own inherits "
        "this, so [pre-push] means no hook runs at EVALUATED_STAGE while the "
        "coverage arithmetic still looks perfect. Schema default is every "
        "stage, so absence is safe."
    ),
    "default_install_hook_types": (
        "SELECTION-NEUTRAL. It governs which git hook files `pre-commit "
        "install` writes, not which files a hook receives; it relocates WHEN "
        "the gate fires, and `pre-commit run --files` ignores it entirely. "
        "Residual, deliberately not handled here: a value omitting pre-commit "
        "leaves a plain `git commit` ungated, which is a when-does-the-gate-"
        "fire question rather than a which-files-are-in-scope one. See "
        "B-STAGE9-16 Part B report."
    ),
    "default_language_version": (
        "SELECTION-NEUTRAL. Pins the interpreter a language's hooks run under, "
        "changing what the tool DOES to a file, never which files reach it. A "
        "wrong version fails loudly rather than skipping silently."
    ),
    "fail_fast": (
        "SELECTION-NEUTRAL, and safe in the direction that matters: it stops "
        "the run after a hook has ALREADY gone red, a loud state. It does not "
        "alter any hook's input set."
    ),
    "minimum_pre_commit_version": (
        "SELECTION-NEUTRAL. A version assertion at config load; too-old "
        "pre-commit aborts fatally rather than running a narrowed hook set."
    ),
    "ci": (
        "SELECTION-NEUTRAL for this guard's claim. pre-commit itself never "
        "reads it; the schema accepts it so pre-commit.ci config does not warn "
        "as an unknown key. It can carry that service's skip: list, which "
        "changes what a third party runs, not what this config declares in "
        "scope. This repo does not use pre-commit.ci."
    ),
}

# The third and last nesting level. No key here affects selection today; it is
# closed so the allow-list does not stop one level short again.
MODELLED_REPO_KEYS = frozenset({"repo", "rev", "hooks"})

# This guard's own ctest registrations, checked by LINT-SELF against the live
# registry the same way Part A checks its own.
SELF_TEST_NAMES = (
    "testGarminLintOwnershipGuard",
    "testGarminLintOwnershipGuardUnits",
)

SELF_SOURCE_FILES = (
    "unittests/buildguard/garmin_lint_ownership_guard.py",
    "unittests/buildguard/test_garmin_lint_ownership_guard.py",
)


class ConfigError(RuntimeError):
    """The config or the index could not be read. Never a silent pass."""


# --------------------------------------------------------------------------
# the declaration
# --------------------------------------------------------------------------
@dataclass(frozen=True)
class Gap:
    """A DELIBERATE, VISIBLE, REASONED hole in lint coverage.

    DEC-054's shape for the ctest half, applied to the lint half: an explicit
    opt-out is fine, silence is the defect. Three kinds:

      suffix  a file kind under this root that no hook covers (".sh")
      path    one specific tracked file left unowned
      tool    one hook id that deliberately does not cover this root ("mypy")

    Every kind is checked for STALENESS, so a declaration cannot outlive its
    own truth.
    """

    kind: str
    subject: str
    reason: str


@dataclass(frozen=True)
class ManagedRoot:
    """A set of Garmin-owned paths and the lint ownership claimed for them.

    `patterns` are fnmatch globs evaluated against repo-relative tracked
    paths. `owned` lists the FILE KINDS that MUST be matched by some hook — a
    file's kind is its suffix, or its bare name when it has none (".gitignore").
    """

    name: str
    patterns: tuple[str, ...]
    owned: tuple[str, ...]
    gaps: tuple[Gap, ...] = ()
    note: str = ""


def file_kind(path: str) -> str:
    """Suffix, or the bare filename when there is none.

    `CMakeLists.txt` -> `.txt`; `.gitignore` -> `.gitignore`. One key space so
    a declaration can name either without a second mechanism.
    """
    suffix = Path(path).suffix
    return suffix if suffix else Path(path).name


# The Garmin-owned C++ production scope is IMPORTED, not restated. The T-203
# security guard already maintains exactly this list ("which files are the
# Garmin feature's own C++?"), and the i18n guard already imports it for the
# same reason: two copies of one list are a drift defect waiting to happen.
# Importing it also means a NEW Garmin C++ file automatically acquires a lint
# obligation here the moment it is added there — the enumeration grows itself,
# which is the whole point of this unit.
from garmin_sec_source_guard import CPP_GLOBS, PY_ROOT  # noqa: E402

_NO_CMAKE_LINTER = (
    "DEC-009's tool set contains no CMake linter, and adding one is out of "
    "scope for B-STAGE9-16. These files are reviewed as part of the test "
    "registration they perform."
)
_NO_PLAIN_TEXT_LINTER = (
    "Not a source file: no linter in DEC-009's tool set applies, and none is "
    "wanted. Declared explicitly so the kind is accounted for rather than "
    "silently skipped."
)

MANAGED_ROOTS: tuple[ManagedRoot, ...] = (
    ManagedRoot(
        name="garmin-python-production",
        patterns=(f"{PY_ROOT}/*",),
        owned=(".py",),
        gaps=(
            Gap(kind="suffix", subject=".toml", reason=_NO_PLAIN_TEXT_LINTER),
            Gap(kind="suffix", subject=".gitignore", reason=_NO_PLAIN_TEXT_LINTER),
        ),
    ),
    ManagedRoot(
        name="garmin-cpp-production",
        patterns=tuple(CPP_GLOBS),
        owned=(".cpp", ".h"),
        note="scope imported from garmin_sec_source_guard.CPP_GLOBS",
        gaps=(
            Gap(
                kind="path",
                subject="src/Cloud/PyEmbeddedAdapter.cpp",
                reason=(
                    "Garmin-owned and SHOULD be clang-formatted, but it is RED "
                    "ON ARRIVAL: measured with the pinned clang-format 18.1.8 "
                    "and --style=file, its aligned-column string array at "
                    "lines 199-200 violates the style. DEC-054 refuses to "
                    "adopt a gate that is red on arrival, and reformatting "
                    "src/ was outside B-STAGE9-16 Part B's allowed paths, so "
                    "this is a DECLARED gap with a follow-up owed, not a "
                    "silent one. Its header PyEmbeddedAdapter.h IS covered."
                ),
            ),
            Gap(
                kind="path",
                subject="src/Cloud/AddCloudWizard.cpp",
                reason=(
                    "LEGACY GoldenCheetah file that the Garmin feature only "
                    "extends; it predates this project. DEC-010 scoped "
                    "pre-commit to NEW Garmin paths precisely so as not to "
                    "retrofit the legacy tree, and that principle stands. "
                    "Measured red against clang-format 18.1.8, as expected "
                    "for legacy formatting."
                ),
            ),
            Gap(
                kind="path",
                subject="src/Cloud/AddCloudWizard.h",
                reason=(
                    "LEGACY GoldenCheetah file, same reasoning as its .cpp: "
                    "DEC-010's no-retrofit principle, and measured red "
                    "against clang-format 18.1.8."
                ),
            ),
        ),
    ),
    ManagedRoot(
        name="garmin-cpp-unittests",
        patterns=("unittests/Core/garminconnect/*",),
        owned=(".cpp", ".h"),
        gaps=(
            Gap(kind="suffix", subject=".txt", reason=_NO_CMAKE_LINTER),
            Gap(kind="suffix", subject=".gitignore", reason=_NO_PLAIN_TEXT_LINTER),
            Gap(
                kind="suffix",
                subject=".py",
                reason=(
                    "The pystubs/ Python test doubles are RED ON ARRIVAL for "
                    "both ruff hooks: measured E402 at "
                    "pystubs/garmin_client.py:250, which is a DELIBERATE "
                    "mid-file import fixture (see the comment above it) that "
                    "a lint fix would destroy, plus the file is unformatted. "
                    "Bringing test doubles under ruff needs a per-file ignore "
                    "decision of its own; declared here rather than landing a "
                    "red hook, which DEC-054 forbids."
                ),
            ),
        ),
    ),
    ManagedRoot(
        name="garmin-cpp-stderrbuf",
        patterns=("unittests/Core/stderrbuf/*",),
        owned=(".cpp", ".h"),
        note=(
            "ARMED EARLY AND DELIBERATELY EMPTY. B-STAGE9-12's deliverable is "
            "untracked, so this root has no tracked file and clang-format's "
            "regex matches nothing here today. Cost of arming early is exactly "
            "zero and there is no red-on-arrival risk; the alternative is "
            "remembering to widen a regex later, which is the failure mode "
            "this entire guard exists to abolish. Reported as EMPTY, never as "
            "covered, so 'armed' can never be mistaken for 'proven'."
        ),
    ),
    ManagedRoot(
        name="garmin-buildguard",
        patterns=("unittests/buildguard/*",),
        owned=(".py",),
        gaps=(
            Gap(kind="suffix", subject=".txt", reason=_NO_CMAKE_LINTER),
            Gap(
                kind="suffix",
                subject=".sh",
                reason=(
                    "garmin_flag_build_guard.sh is Garmin-owned, but DEC-009's "
                    "tool set contains NO SHELL LINTER at all, so 'covered by "
                    "some hook' is not achievable without adding one. "
                    "shellcheck is NOT INSTALLED on this host, so the arrival "
                    "state of such a hook cannot be measured, and DEC-054 "
                    "forbids adopting a gate whose arrival state is unknown. "
                    "Adding an unmeasurable hook purely to satisfy a coverage "
                    "guard would be the worst of both worlds. Follow-up owed."
                ),
            ),
            Gap(
                kind="tool",
                subject="mypy",
                reason=(
                    "NO TYPE COVERAGE IS CLAIMED for this directory, on "
                    "DEC-054's own sanctioned branch: mypy here was required "
                    "'only if a dedicated config for pytest-style guard tests "
                    "is demonstrated clean'. It is not. MEASURED with the "
                    "pinned mypy 1.11.2 and the hook's own config: 43 errors "
                    "in 4 of 5 files (25 in test_garmin_gate_coverage_guard.py "
                    "alone), overwhelmingly untyped pytest decorators and "
                    "unparameterised dict - systemic, a typing project of its "
                    "own, not a line item. The REJECTED alternative was to "
                    "widen mypy but draw its regex to exclude the files that "
                    "fail it: a gate shaped around its own blind spot, green "
                    "only because it was aimed away from the problem. An "
                    "honestly declared gap beats a gerrymandered green."
                ),
            ),
        ),
    ),
)


# --------------------------------------------------------------------------
# reading the config that actually runs
# --------------------------------------------------------------------------
@dataclass(frozen=True)
class Hook:
    """One hook, with every layer pre-commit applies to decide its input.

    The two `files:` layers are separate fields and are never merged, so
    hooks_owning() cannot treat the top level as a fallback.

      files / exclude               this hook's own regexes, with pre-commit's
                                    schema defaults ('' and '^$')
      global_files / global_exclude the top-level regexes, applied in addition
      stages                        effective stages, after per-hook `stages:`
                                    falls back to `default_stages:` and legacy
                                    spellings are migrated
    """

    hook_id: str
    name: str
    files: str
    exclude: str
    passes_filenames: bool = True
    unmodelled_keys: tuple[str, ...] = ()
    global_files: str = ""
    global_exclude: str = "^$"
    stages: tuple[str, ...] = (EVALUATED_STAGE,)


@dataclass(frozen=True)
class TopLevel:
    """The config's top-level keys, as they bear on file selection."""

    files: str
    exclude: str
    default_stages: tuple[str, ...]
    unmodelled_keys: tuple[str, ...] = ()
    unmodelled_repo_keys: tuple[tuple[str, str], ...] = ()


@dataclass(frozen=True)
class ParsedConfig:
    hooks: tuple[Hook, ...]
    top_level: TopLevel


def _effective_stages(
    raw_hook_stages: object, default_stages: tuple[str, ...]
) -> tuple[str, ...]:
    """An absent or empty per-hook `stages:` inherits `default_stages:`; a
    non-empty one replaces it outright (`repository.py:123-124`)."""
    if isinstance(raw_hook_stages, list) and raw_hook_stages:
        return tuple(transform_stage(str(s)) for s in raw_hook_stages)
    return default_stages


def load_config(config_path: Path) -> ParsedConfig:
    """Parse the live `.pre-commit-config.yaml`. The ONE parser — load_hooks()
    is a view onto this, never a second read of the same file.

    Raises ConfigError on anything unreadable. That matters more than it
    looks: a config that parsed as "no hooks" would make every managed file
    unowned (loudly red for the wrong reason) while simultaneously making
    every gap declaration look stale. Unusable input must never render a
    verdict.
    """
    try:
        import yaml
    except ImportError as exc:  # pragma: no cover - PyYAML ships with pre-commit
        raise ConfigError(
            "PyYAML is not importable, so the real hook regexes cannot be "
            "evaluated. This guard refuses to guess; install PyYAML "
            f"(pre-commit itself depends on it). {exc}"
        ) from exc

    try:
        raw = config_path.read_text(encoding="utf-8")
    except OSError as exc:
        raise ConfigError(f"cannot read {config_path}: {exc}") from exc

    try:
        doc = yaml.safe_load(raw)
    except yaml.YAMLError as exc:
        raise ConfigError(f"{config_path} is not valid YAML: {exc}") from exc

    if not isinstance(doc, dict) or "repos" not in doc:
        raise ConfigError(
            f"{config_path} has no top-level 'repos' key; this is not a "
            f"pre-commit config and no ownership conclusion can be drawn."
        )

    # Schema defaults; applied to every hook in addition to its own regexes.
    global_exclude = doc.get("exclude", "^$")
    global_files = doc.get("files", "")

    # Absent default_stages must mean "runs everywhere", not "runs nowhere".
    raw_default_stages = doc.get("default_stages")
    if isinstance(raw_default_stages, list) and raw_default_stages:
        default_stages = tuple(transform_stage(str(s)) for s in raw_default_stages)
    else:
        default_stages = ALL_STAGES

    for label, pattern in (("files", global_files), ("exclude", global_exclude)):
        try:
            re.compile(pattern)
        except re.error as exc:
            raise ConfigError(
                f"{config_path}: top-level '{label}' is an invalid regex "
                f"{pattern!r}: {exc}"
            ) from exc

    hooks: list[Hook] = []
    unmodelled_repo_keys: list[tuple[str, str]] = []
    repos = doc.get("repos") or []
    if not isinstance(repos, list):
        raise ConfigError(f"{config_path}: 'repos' is not a list")
    for repo in repos:
        if not isinstance(repo, dict):
            continue
        repo_name = str(repo.get("repo", "<unnamed repo>"))
        for key in sorted(repo):
            if key not in MODELLED_REPO_KEYS:
                unmodelled_repo_keys.append((repo_name, str(key)))
        for hook in repo.get("hooks") or []:
            if not isinstance(hook, dict) or "id" not in hook:
                continue
            files = hook.get("files", "")
            exclude = hook.get("exclude", "^$")
            for pattern in (files, exclude):
                try:
                    re.compile(pattern)
                except re.error as exc:
                    raise ConfigError(
                        f"{config_path}: hook '{hook['id']}' has an invalid "
                        f"regex {pattern!r}: {exc}"
                    ) from exc
            hooks.append(
                Hook(
                    hook_id=str(hook["id"]),
                    name=str(hook.get("name", hook["id"])),
                    files=files,
                    exclude=exclude,
                    passes_filenames=hook.get("pass_filenames", True) is not False,
                    unmodelled_keys=tuple(
                        sorted(k for k in hook if k not in MODELLED_HOOK_KEYS)
                    ),
                    global_files=global_files,
                    global_exclude=global_exclude,
                    stages=_effective_stages(hook.get("stages"), default_stages),
                )
            )

    top_level = TopLevel(
        files=global_files,
        exclude=global_exclude,
        default_stages=default_stages,
        unmodelled_keys=tuple(
            sorted(k for k in doc if k not in MODELLED_TOP_LEVEL_KEYS)
        ),
        unmodelled_repo_keys=tuple(unmodelled_repo_keys),
    )
    return ParsedConfig(hooks=tuple(hooks), top_level=top_level)


def load_hooks(config_path: Path) -> list[Hook]:
    """The hook list alone — a view onto load_config(), never a second parse."""
    return list(load_config(config_path).hooks)


def rejecting_layer(path: str, hook: Hook) -> str | None:
    """Which layer drops `path` before this hook runs, or None if none does.

    Evaluated in pre-commit's own order, so the answer names the FIRST thing
    that removes the file. Computed on every call, never stored.
    """
    if not hook.passes_filenames:
        return (
            f"hook '{hook.hook_id}' sets pass_filenames: false, so it is never "
            f"handed any file (its files: regex is a TRIGGER, not a scope)"
        )
    if EVALUATED_STAGE not in hook.stages:
        return (
            f"hook '{hook.hook_id}' does not run at the '{EVALUATED_STAGE}' "
            f"stage (effective stages: {', '.join(hook.stages) or 'none'}), so "
            f"it is filtered out of the run before selection"
        )
    if not re.search(hook.global_files, path):
        return (
            f"the TOP-LEVEL files: regex {hook.global_files!r} — the file is "
            f"removed before ANY hook is consulted"
        )
    if re.search(hook.global_exclude, path):
        return (
            f"the TOP-LEVEL exclude: regex {hook.global_exclude!r} — the file "
            f"is removed before ANY hook is consulted"
        )
    if not re.search(hook.files, path):
        return f"hook '{hook.hook_id}' files: regex {hook.files!r}"
    if re.search(hook.exclude, path):
        return f"hook '{hook.hook_id}' exclude: regex {hook.exclude!r}"
    return None


def hooks_owning(path: str, hooks: list[Hook]) -> list[str]:
    """Hook ids that would be HANDED `path`, using pre-commit's own semantics.

    Selection is an INTERSECTION of layers (module docstring), not a fallback
    chain. `re.search`, not `re.match`. A hook that never receives filenames,
    or does not run at EVALUATED_STAGE, owns nothing.
    """
    return [h.hook_id for h in hooks if rejecting_layer(path, h) is None]


def tracked_files(root: Path, patterns: tuple[str, ...]) -> list[str]:
    """Repo-relative tracked paths matching any glob, from the git INDEX."""
    try:
        proc = subprocess.run(
            ["git", "ls-files", "-z"],
            cwd=root,
            capture_output=True,
            text=True,
            timeout=60,
            check=False,
        )
    except (OSError, subprocess.SubprocessError) as exc:
        raise ConfigError(f"cannot run `git ls-files` in {root}: {exc}") from exc
    if proc.returncode != 0:
        raise ConfigError(
            f"`git ls-files` failed in {root} (rc={proc.returncode}): "
            f"{proc.stderr.strip()}. Without the index there is no file "
            f"inventory and no ownership conclusion can be drawn."
        )
    everything = [p for p in proc.stdout.split("\0") if p]
    return sorted(p for p in everything if any(fnmatch.fnmatch(p, g) for g in patterns))


# --------------------------------------------------------------------------
# LINT-OWNERSHIP
# --------------------------------------------------------------------------
@dataclass
class RootReport:
    root: ManagedRoot
    all_files: list[str] = field(default_factory=list)
    covered: list[str] = field(default_factory=list)
    gapped: list[str] = field(default_factory=list)
    undeclared: list[str] = field(default_factory=list)

    @property
    def is_empty(self) -> bool:
        return not self.all_files


def why_unowned(path: str, hooks: list[Hook]) -> str:
    """Prose naming the layer that dropped `path`, for a LINT-OWNERSHIP finding.

    A bare "no hook matched" points a reader at the per-hook regexes, which is
    the wrong place when a top-level filter or default_stages: removed the
    file. Those cases are named separately.
    """
    if not hooks:
        return "the config declares no hooks at all"
    reasons = {h.hook_id: rejecting_layer(path, h) for h in hooks}
    top_level = sorted(
        {r for r in reasons.values() if r is not None and "TOP-LEVEL" in r}
    )
    staged_out = sorted(
        {
            r
            for r in reasons.values()
            if r is not None and f"'{EVALUATED_STAGE}' stage" in r
        }
    )
    if len(top_level) == 1 and len(reasons) == sum(
        1 for r in reasons.values() if r is not None and "TOP-LEVEL" in r
    ):
        return (
            f"EVERY hook is bypassed by {top_level[0]}. Widening a per-hook "
            f"files: regex will NOT help — fix the top-level filter"
        )
    if staged_out and len(staged_out) == len({r for r in reasons.values()}):
        return (
            f"EVERY hook is out of scope because {staged_out[0]}. Check the "
            f"top-level default_stages:"
        )
    per_hook = "; ".join(
        f"{hid} — {why}" for hid, why in sorted(reasons.items()) if why
    )
    return f"rejected by: {per_hook}"


def _gap_for(root: ManagedRoot, path: str) -> Gap | None:
    for gap in root.gaps:
        if gap.kind == "path" and gap.subject == path:
            return gap
        if gap.kind == "suffix" and gap.subject == file_kind(path):
            return gap
    return None


def check_ownership(
    source_root: Path,
    roots: tuple[ManagedRoot, ...] | list[ManagedRoot],
    hooks: list[Hook],
) -> tuple[list[str], list[RootReport]]:
    """Every OWNED file kind under every managed root must have a hook."""
    findings: list[str] = []
    reports: list[RootReport] = []
    for root in roots:
        report = RootReport(root=root)
        report.all_files = tracked_files(source_root, root.patterns)
        for path in report.all_files:
            gap = _gap_for(root, path)
            owners = hooks_owning(path, hooks)
            if owners:
                report.covered.append(path)
            elif gap is not None:
                report.gapped.append(path)
            elif file_kind(path) in root.owned:
                findings.append(
                    f"LINT-OWNERSHIP {path}: declared OWNED ('{file_kind(path)}' "
                    f"under managed root '{root.name}') but handed to NO hook "
                    f"by {CONFIG_FILENAME} — {why_unowned(path, hooks)}. A file "
                    f"no hook receives is linted by nothing, and a skipped hook "
                    f"looks exactly like a passing one (DEC-054). Either widen "
                    f"the filter named above, or declare a reasoned Gap for it."
                )
            else:
                report.undeclared.append(path)
        reports.append(report)
    return findings, reports


# --------------------------------------------------------------------------
# LINT-GAP
# --------------------------------------------------------------------------
def check_declared_gaps(
    roots: tuple[ManagedRoot, ...] | list[ManagedRoot],
    files: list[str],
    owners_by_file: dict[str, list[str]],
) -> list[str]:
    """Gaps must be reasoned, of a known kind, and STILL TRUE.

    The staleness half is the one that stops this declaration rotting the way
    `.pre-commit-config.yaml`'s enumeration did. Widen mypy to
    `unittests/buildguard/` and the "no type coverage is claimed" declaration
    becomes false — so it fails here until someone deletes it. A declaration
    may not outlive its own truth.

    WHEN AN EMPTY SUBJECT SET IS STALE, decided deliberately rather than left
    to fall out of the code (round 1 review, R1-F3):

      path    STALE when the subject is not in the root's tracked inventory. A
              path gap names ONE artifact and has no meaning without it; if
              the file is deleted or renamed the declaration excuses nothing
              and is pure residue.
      suffix  NOT stale when no file of that kind currently exists. A suffix
              gap is a STANDING POLICY about a CLASS of file (".sh here is
              unowned because no shell linter exists"), and the class
              legitimately empties and refills. Deleting the last .gitignore
              must not force a policy edit, and re-adding one must not need
              the policy re-derived.
      tool    NOT stale when the root is empty, for the same reason: "mypy
              deliberately does not cover this root" is a claim about the
              root, not about any particular file in it.

    The distinction is artifact-scoped versus class-scoped, and it is the
    reason only the `path` branch checks inventory membership.
    """
    findings: list[str] = []
    for root in roots:
        for gap in root.gaps:
            if gap.kind not in ("suffix", "path", "tool"):
                findings.append(
                    f"LINT-GAP {root.name}/{gap.subject}: unknown gap kind "
                    f"'{gap.kind}'. Known kinds are suffix, path, tool."
                )
                continue
            if len(gap.reason.strip()) < MIN_REASON_CHARS:
                findings.append(
                    f"LINT-GAP {root.name}/{gap.subject}: the reason is "
                    f"{len(gap.reason.strip())} chars, below the "
                    f"{MIN_REASON_CHARS}-char floor. Per DEC-054 an opt-out "
                    f"without a recorded reason is itself a defect; say what "
                    f"is not covered and why."
                )
                continue

            scoped = [
                f for f in files if any(fnmatch.fnmatch(f, g) for g in root.patterns)
            ]
            if gap.kind == "tool":
                still_true = not any(
                    gap.subject in owners_by_file.get(f, []) for f in scoped
                )
                detail = f"hook '{gap.subject}' now matches files under this root"
            elif gap.kind == "suffix":
                subjects = [f for f in scoped if file_kind(f) == gap.subject]
                still_true = not any(owners_by_file.get(f) for f in subjects)
                detail = f"'{gap.subject}' files under this root are now covered"
            elif gap.subject not in scoped:
                # A `path` gap names ONE specific artifact, so it dies with it.
                # Left standing, a declaration for a deleted or renamed file
                # prints in the PASS output as a live, re-checked exception
                # while excusing nothing that exists — a stale claim about the
                # tree's coverage, which is the same disease as a stale regex.
                still_true = False
                detail = (
                    f"'{gap.subject}' is no longer a tracked file under this "
                    f"root (deleted, renamed, or moved out of scope), so the "
                    f"declaration has outlived the file it excused"
                )
            else:
                still_true = not owners_by_file.get(gap.subject)
                detail = f"'{gap.subject}' is now covered"

            if not still_true:
                findings.append(
                    f"LINT-GAP {root.name}/{gap.subject}: this declared gap is "
                    f"STALE — {detail}. The declaration asserts that coverage "
                    f"is deliberately absent here, and that is no longer the "
                    f"case, so the tree carries a claim about its own coverage "
                    f"that is not true. Delete the Gap, or correct it to match "
                    f"what the tree now looks like."
                )
    return findings


# --------------------------------------------------------------------------
# LINT-SELF
# --------------------------------------------------------------------------
def check_self_is_linted(hooks: list[Hook]) -> list[str]:
    findings = []
    for rel in SELF_SOURCE_FILES:
        if not hooks_owning(rel, hooks):
            findings.append(
                f"LINT-SELF {rel}: the lint-ownership guard itself is matched "
                f"by no hook. A coverage guard outside the coverage it polices "
                f"is the original bug wearing a new hat (DEC-054)."
            )
    return findings


def check_self_is_gated(build_dir: Path) -> list[str]:
    """LINT-SELF's registry half: this guard is in the routine ctest gate.

    Delegated to Part A's guard rather than reimplemented — same registry,
    same semantics, and one copy of "what does the gate select" cannot drift
    from another.
    """
    try:
        from garmin_gate_coverage_guard import (
            GATE_COMMAND,
            GATE_EXCLUDE_LABEL,
            RegistryError,
            labels_of,
            load_registry,
            selected_by_gate,
        )
    except ImportError as exc:
        raise ConfigError(f"cannot import the gate-coverage guard: {exc}") from exc

    try:
        tests = load_registry(build_dir)
        gated = selected_by_gate(build_dir)
    except RegistryError as exc:
        raise ConfigError(str(exc)) from exc

    by_name = {t["name"]: t for t in tests}
    findings = []
    for name in SELF_TEST_NAMES:
        if name not in by_name:
            findings.append(
                f"LINT-SELF {name}: not registered as a ctest test. An unrun "
                f"coverage guard protects nothing."
            )
            continue
        if GATE_EXCLUDE_LABEL in labels_of(by_name[name]):
            findings.append(
                f"LINT-SELF {name}: carries '{GATE_EXCLUDE_LABEL}'. A coverage "
                f"guard outside the gate it polices is the original bug "
                f"wearing a new hat (DEC-054). No reason is good enough; "
                f"remove the label."
            )
            continue
        if name not in gated:
            findings.append(
                f"LINT-SELF {name}: registered, but `{GATE_COMMAND}` does not "
                f"select it."
            )
    return findings


# --------------------------------------------------------------------------
# LINT-MODEL
# --------------------------------------------------------------------------
def check_model_is_faithful(hooks: list[Hook]) -> list[str]:
    """LINT-MODEL: fail closed on any per-hook key outside the allow-list.

    The point is the UNKNOWN key, not the known-bad one. A key this guard has
    never heard of may narrow selection, in which case treating the hook as an
    owner OVER-CERTIFIES coverage — the precise thing LINT-MODEL claims to
    forbid. So anything not in MODELLED_HOOK_KEYS fails, whether or not this
    guard recognises it as narrowing.
    """
    findings = []
    for hook in hooks:
        if not hook.unmodelled_keys:
            continue
        narrowing = [k for k in hook.unmodelled_keys if k in KNOWN_NARROWING_KEYS]
        unknown = [k for k in hook.unmodelled_keys if k not in KNOWN_NARROWING_KEYS]
        what = []
        if narrowing:
            what.append(
                f"{', '.join(narrowing)} (known to NARROW selection beyond "
                f"files:/exclude:)"
            )
        if unknown:
            what.append(
                f"{', '.join(unknown)} (UNKNOWN to this guard — it may narrow "
                f"selection, and assuming otherwise is precisely the "
                f"over-certification this rule exists to prevent)"
            )
        findings.append(
            f"LINT-MODEL {hook.hook_id}: declares {'; '.join(what)}. This "
            f"guard evaluates only files:/exclude:/pass_filenames, so it would "
            f"CERTIFY AS OWNED a file the hook actually drops. Failing closed "
            f"is deliberate. Either teach this guard the key by adding it to "
            f"MODELLED_HOOK_KEYS once you have reasoned about its effect on "
            f"FILE SELECTION, or remove it from the config."
        )
    return findings


def check_top_level_is_modelled(top_level: TopLevel) -> list[str]:
    """LINT-MODEL, one level out: fail closed on any unknown top-level or
    repo-level key. A top-level key is applied to every hook at once, so
    assuming an unknown one is harmless over-certifies the whole config."""
    findings = []
    for key in top_level.unmodelled_keys:
        findings.append(
            f"LINT-MODEL top-level '{key}': this guard has never reasoned "
            f"about this key, so it cannot know whether it removes files from "
            f"the set handed to the hooks. A top-level key applies to EVERY "
            f"hook at once — pre-commit filters the whole candidate list "
            f"through the top level before any hook is consulted — so assuming "
            f"it is harmless would over-certify coverage across the entire "
            f"config. Failing closed is deliberate. Either add it to "
            f"MODELLED_TOP_LEVEL_KEYS with a written reason once you have "
            f"reasoned about its effect on FILE SELECTION at the "
            f"'{EVALUATED_STAGE}' stage, or remove it from the config."
        )
    for repo_name, key in top_level.unmodelled_repo_keys:
        findings.append(
            f"LINT-MODEL repo '{repo_name}' key '{key}': unknown at the repo "
            f"level, where pre-commit 4.2.0 allows only "
            f"{', '.join(sorted(MODELLED_REPO_KEYS))}. Same reasoning as the "
            f"top-level and per-hook allow-lists: an unrecognised key may "
            f"affect which files reach these hooks, and silently ignoring it "
            f"is how this guard's own model goes stale."
        )
    return findings


# --------------------------------------------------------------------------
# LINT-VACUOUS
# --------------------------------------------------------------------------
def check_not_vacuous(reports: list[RootReport]) -> list[str]:
    """The guard must have EXAMINED something before it may say PASS.

    Round 1 review found this missing and it was the sharpest finding of the
    round: with a real parseable config and a real git repo whose index simply
    contains no managed paths — a bad scope edit, a sparse or bootstrap
    checkout — every root returned an empty inventory, nothing produced a
    finding, and the registered command printed `PASS — 0/0 tracked managed
    files` and exited 0. Reproduced before fixing. It is worse than it sounds:
    the PASS path also prints the full DECLARED GAPS block, so a run that
    inspected NOTHING produced the most thorough-looking output the guard
    emits.

    A guard that can go quiet is the original bug wearing a new hat, which is
    the same argument Part A's GATE-SELF makes. This rule is where that stops.

    NOTE WHAT IS *NOT* CHANGED HERE. A single empty root is still fine and
    still costs nothing — `unittests/Core/stderrbuf/` is pre-armed on purpose
    and reports EMPTY. The rule is one level up, on the AGGREGATE: across all
    roots combined, at least one managed file must have been inventoried, and
    at least one must have been proven owned. Both prongs are needed. Inventory
    alone would be satisfied by a tree where every managed file is gap-declared,
    which proves nothing about ownership either.

    The equivalent assertion already existed in the unit tests, but only around
    a direct helper call. That did not protect the command CTest runs, and the
    command is the thing that matters.
    """
    inventoried = sum(len(r.all_files) for r in reports)
    covered = sum(len(r.covered) for r in reports)
    if inventoried == 0:
        return [
            "LINT-VACUOUS: every declared managed root is empty, so this run "
            "examined ZERO files and its PASS would mean nothing. A coverage "
            "guard that inspects no inventory is a green no-op. Likely causes: "
            "MANAGED_ROOTS patterns no longer match the tree, or this is not "
            "the repository they describe."
        ]
    if covered == 0:
        return [
            f"LINT-VACUOUS: {inventoried} managed file(s) were inventoried but "
            f"NOT ONE is matched by any hook, so no ownership was demonstrated "
            f"anywhere. Either every managed file is gap-declared — in which "
            f"case this config lints nothing and the declarations are the only "
            f"thing left — or the hook regexes are broken."
        ]
    return []


# --------------------------------------------------------------------------
# reporting
# --------------------------------------------------------------------------
def format_root_report(report: RootReport) -> str:
    if report.is_empty:
        return (
            f"  {report.root.name}: EMPTY — 0 tracked files. Regexes may be "
            f"armed for it, but nothing is proven here."
        )
    bits = [f"{len(report.covered)}/{len(report.all_files)} covered"]
    if report.gapped:
        bits.append(f"{len(report.gapped)} declared-gap")
    if report.undeclared:
        bits.append(f"{len(report.undeclared)} not-a-source-kind")
    return f"  {report.root.name}: {', '.join(bits)}"


def main(argv: list[str]) -> int:
    if len(argv) not in (2, 4) or (len(argv) == 4 and argv[2] != "--build-dir"):
        print(
            f"usage: {Path(argv[0]).name} <source-root> [--build-dir DIR]",
            file=sys.stderr,
        )
        return 2
    source_root = Path(argv[1]).resolve()
    build_dir = Path(argv[3]).resolve() if len(argv) == 4 else None

    try:
        parsed = load_config(source_root / CONFIG_FILENAME)
        hooks = list(parsed.hooks)
        findings, reports = check_ownership(source_root, MANAGED_ROOTS, hooks)
        all_files = [f for r in reports for f in r.all_files]
        owners_by_file = {f: hooks_owning(f, hooks) for f in all_files}
        findings += check_declared_gaps(MANAGED_ROOTS, all_files, owners_by_file)
        findings += check_self_is_linted(hooks)
        findings += check_model_is_faithful(hooks)
        findings += check_top_level_is_modelled(parsed.top_level)
        if build_dir is not None:
            findings += check_self_is_gated(build_dir)

        # Vacuity first: it makes every other finding a downstream symptom.
        vacuity = check_not_vacuous(reports)
        findings = vacuity + findings
    except ConfigError as exc:
        print(f"garmin-lint-ownership-guard: UNUSABLE — {exc}", file=sys.stderr)
        return 2

    if findings:
        print(f"garmin-lint-ownership-guard: FAIL — {len(findings)} finding(s)")
        if vacuity:
            print(
                "  *** READ THIS FIRST: THIS RUN EXAMINED NOTHING. The "
                "LINT-VACUOUS finding below is the diagnosis; any other "
                "finding here is a SYMPTOM of it (an empty inventory also "
                "makes every path-scoped declaration look stale). Fix the "
                "inventory before reading further. ***"
            )
        for finding in findings:
            print(f"  {finding}")
        return 1

    # Counts and gaps are printed on the PASS path DELIBERATELY. "0 findings"
    # from a guard that examined nothing looks identical to a real pass, and
    # this project has shipped that mistake before. More importantly, a guard
    # that reported full coverage while mypy silently skipped a directory would
    # be lying in exactly the register this finding exists to abolish — so the
    # gaps are part of the PASS output, not an omission from it.
    covered = sum(len(r.covered) for r in reports)
    total = sum(len(r.all_files) for r in reports)
    print(
        f"garmin-lint-ownership-guard: PASS — {covered}/{total} tracked managed "
        f"files matched by at least one hook in {CONFIG_FILENAME}, 0 findings"
    )
    for report in reports:
        print(format_root_report(report))
    print("  DECLARED GAPS (deliberate, reasoned, and re-checked every run):")
    for root in MANAGED_ROOTS:
        for gap in root.gaps:
            summary = " ".join(gap.reason.split())
            print(f"    - {root.name}: {gap.kind} '{gap.subject}' — {summary}")
    if build_dir is None:
        print(
            "  NOTE: --build-dir was not given, so LINT-SELF's ctest-registry "
            "half DID NOT RUN. This PASS covers lint ownership only."
        )
    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv))
