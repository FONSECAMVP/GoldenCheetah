#!/usr/bin/env python3
"""B-STAGE9-16 / DEC-054 static guard: the routine gate covers every test.

DEC-054 inverted this project's verification default. The routine gate used to
be `ctest -L garmin-fast` — OPT-IN, so a test was gated only if someone
remembered to attach that one label. `testGarminI18nSourceGuard` carried
`garmin-i18n-guard` instead, so the habitual gate could not see it and it sat
RED in HEAD for two days across two Inspectors' own independent re-runs. Every
"garmin-fast 40/40" line in that period is 40 of 53 registered tests.

The gate is now `ctest -LE gate-exclude` — DEFAULT-INCLUDE. A newly registered
test is gated without anyone enrolling it, and the failure mode inverts from
silent omission to loud inclusion. This guard is the DETECTION half, a
complement and never the main protection (the protection is the default
itself), asserting the two things that can still go wrong:

  GATE-COVERAGE   every registered test is SELECTED by
                  `ctest -LE gate-exclude` and carries none of the
                  result-masking properties ENUMERATED in
                  GATE_MASKING_PROPERTIES, unless it carries the
                  `gate-exclude` label.
                  Read that promise narrowly, because a previous revision of
                  this docstring said "actually RUNNABLE" and that was an
                  overclaim it had not earned — it had proved only "not
                  DISABLED". What is proved is the absence of the THREE
                  specific masking mechanisms that have been measured against
                  real ctest (see masking_properties_of() for the measurements
                  and the exact JSON shapes). It is NOT a proof that a
                  selected test will run and be counted; ctest has other
                  properties, and future versions may add more. This rule
                  shrinks a known class of bypass; it does not close the
                  category.
                  Selection alone is nowhere near enough. All three masking
                  properties leave a test in the `-LE gate-exclude` selection
                  set while removing its result from the gate, touching no
                  label, recording no reason and naming no tier — a complete
                  bypass of DEC-054's opt-out protocol.
  GATE-EXCUSE     every `gate-exclude` test carries BOTH a non-trivial
                  GATE_EXCLUDE_REASON and a GATE_EXCLUDE_TIER, and that tier
                  is a ctest command which really SELECTS the excused test.
                  Per DEC-054 an opt-out without a recorded reason is itself a
                  defect: the exclusion list is the one place the new scheme
                  can still rot, so it is the place that is policed.
                  Stated plainly because the alternative is the overclaiming
                  this guard was itself caught doing: verifying the tier
                  proves the excused test is REACHABLE by the recorded
                  command. It does NOT prove anything ever runs that command,
                  and nothing in this repository currently does.
                  The tier must also match a deliberately tiny ALLOW-LIST
                  grammar (see parse_selection_tier()): the literal token
                  `ctest` followed only by selection-restricting flags. A
                  repo-controlled property string that this guard hands to a
                  real ctest binary is an execution surface, and `ctest -S`
                  was measured executing a CMake script straight through
                  `-N --show-only=json-v1`.
                  The executable spelling is matched LITERALLY, which is what
                  entitles the word "really" above: the recorded token and the
                  binary this guard resolves are the same string (TIER_COMMAND),
                  so the command certified is the command queried. A basename
                  match was the previous rule and broke exactly that identity.

WHICH ARTIFACT THIS READS, and why it is not the CMakeLists text. The registry
is obtained from `ctest --show-only=json-v1`, which lists every test with its
resolved properties without running anything. That is the artifact that
actually describes what ctest will do. Parsing `unittests/buildguard/
CMakeLists.txt` would be checking the thing that LOOKS like the registry: it
would miss tests registered anywhere else in the tree, miss generator-expression
and foreach() expansion, and happily pass while the configured build says
something different. (This project has been bitten six times by verifying
against a look-alike artifact; that is a ledgered lesson.)

WHY THE REASON AND TIER ARE PROPERTIES, NOT COMMENTS. An adjacent comment is
required too and is part of DEC-054's deliverable — humans read the CMakeLists.
But a comment cannot be checked from the registry, so the same facts are
carried as custom test properties, which `--show-only=json-v1` reports
verbatim. That keeps the machine-checkable copy and the human-readable copy in
the same edit.

THIS GUARD MUST NOT ITSELF CARRY `gate-exclude`. A coverage guard outside the
gate is the original bug wearing a new hat. GATE-SELF below asserts that about
this guard's own registration, from the same registry.

Usage: garmin_gate_coverage_guard.py <build-dir> [--gate-label LABEL]
Exit 0 = PASS, 1 = findings (each printed as RULE test: detail), 2 = usage /
unusable registry.
"""

from __future__ import annotations

import json
import shlex
import shutil
import subprocess
import sys
from pathlib import Path

# The label that excuses a test from the routine gate, and the gate command it
# is excused from. One string, used both to query and to report, so the guard
# and the documented ritual cannot drift apart silently.
GATE_EXCLUDE_LABEL = "gate-exclude"
GATE_COMMAND = f"ctest -LE {GATE_EXCLUDE_LABEL}"

# The properties an excused test must carry. Names are deliberately verbose:
# they appear in `ctest --show-only=json-v1` output and in CMake source, and a
# reader hitting either should not have to guess what they govern.
REASON_PROPERTY = "GATE_EXCLUDE_REASON"
TIER_PROPERTY = "GATE_EXCLUDE_TIER"

# A reason has to say something. This is a floor against `REASON "slow"` or
# `REASON "TODO"`, not a quality bar — no static check can enforce that a
# reason is TRUE, which is why review, not this guard, is the main protection.
MIN_REASON_CHARS = 40

# Every subprocess this guard runs is a ctest SELECTION QUERY: `-N
# --show-only=json-v1`, no test execution, measured at well under a second on
# this repository's 55-test registry. 60s is therefore ~2 orders of magnitude
# of headroom for a loaded machine while still FAILING rather than hanging.
# The number matters because the guard's own ctest registration carries
# TIMEOUT 300: without an inner timeout a wedged query burns the full 300s and
# dies with ctest's generic timeout message instead of a finding that names
# which query hung. Three call sites, one constant, no bare subprocess.run.
QUERY_TIMEOUT_SECONDS = 60

# Properties that leave a test in the gate's SELECTION while removing its
# result from the gate's verdict. Measured against real ctest 3.31.6 on
# 2026-09-15, not read off the documentation — the documentation says a
# skipped test "does not run", and would not have told us the far worse part:
#
#     $ ctest -LE gate-exclude          # 4 tests, two of them genuinely
#                                       # failing with `echo BOOM; exit 1`
#     100% tests passed, 0 tests failed out of 3
#     The following tests did not run:
#             1 - t_skip_retcode (Skipped)
#             2 - t_skip_regex (Skipped)
#
# ctest COUNTS SKIPPED AS PASSED. With a real failure also present it printed
# `75% tests passed, 1 tests failed out of 4` — the skipped tests stayed in the
# denominator and were counted among the passes. DISABLED at least shrinks the
# denominator; these two INFLATE the pass rate. Both are visible in
# `--show-only=json-v1`, so this guard can see them.
GATE_MASKING_PROPERTIES = (
    "DISABLED",
    "SKIP_RETURN_CODE",
    "SKIP_REGULAR_EXPRESSION",
)

# WILL_FAIL is NOT a silent-green bypass on an ordinary test: it inverts the
# result, so introducing it into a healthy gate turns that test red at once and
# cannot be done quietly. On THIS guard it is the instrument inverted — a clean
# exit 0 becomes a FAIL and a detected-defect exit 1 becomes a PASS. So it is
# prohibited for the self-tests ONLY, deliberately not as a general rule.
SELF_PROHIBITED_PROPERTIES = GATE_MASKING_PROPERTIES + ("WILL_FAIL",)

# Two different value shapes, and the difference is load-bearing. Measured:
#
#   DISABLED YES / TRUE / on / 1  -> JSON true        (bool)
#   DISABLED FALSE / unrecognised -> property OMITTED entirely
#   WILL_FAIL YES                 -> JSON true        (bool)
#   SKIP_RETURN_CODE 1            -> 1                (int)
#   SKIP_RETURN_CODE 0            -> 0                (int)  <-- FALSY, EMITTED
#   SKIP_REGULAR_EXPRESSION "B"   -> ['B']             (list)
#   SKIP_REGULAR_EXPRESSION ""    -> property OMITTED entirely
#
# So for the CMake-boolean pair, ctest normalises every true spelling and omits
# a false one (probed across 12 spellings — there is no spelling hole, and the
# string entries below are harmless dead defensive code). For the skip pair,
# PRESENCE alone must be the trigger: `SKIP_RETURN_CODE 0` is emitted as a
# falsy int and masks a PASSING test into a skip, so `if props.get(name)` would
# wave it through. An empty regex is never emitted, so presence costs nothing.
_BOOLEAN_SHAPED_PROPERTIES = ("DISABLED", "WILL_FAIL")
_CMAKE_TRUE_VALUES = (True, "True", "ON", "1")

# What each masked property does, for the finding text. A guard whose message
# does not say what the mechanism IS gets argued with instead of fixed.
_MASKING_EFFECT = {
    "DISABLED": (
        "the gate SELECTS it but never RUNS it, and ctest reports '100% tests "
        "passed' with the test dropped from the denominator"
    ),
    "SKIP_RETURN_CODE": (
        "that exit status is laundered into 'Skipped', which ctest COUNTS AS "
        "PASSED while leaving the test in the denominator — so a genuine "
        "failure inflates the pass rate instead of failing the gate"
    ),
    "SKIP_REGULAR_EXPRESSION": (
        "a matching line in the test's own output marks it 'Skipped' whatever "
        "its exit status, and ctest COUNTS SKIPPED AS PASSED — so the test's "
        "own output decides whether the gate can see it fail"
    ),
    "WILL_FAIL": (
        "the result is INVERTED: this guard's clean exit 0 becomes a FAIL and "
        "its exit 1 on a detected defect becomes a PASS"
    ),
}

# The ONLY tier grammar this guard will execute. See parse_selection_tier():
# `-L` is the form used today, `-R` is a plausible future tier, and `-LE`/`-E`
# are their negations. Each takes exactly one argument. Kept this small on
# purpose: an allow-list fails CLOSED when ctest grows a new mode flag, and a
# deny-list of known-dangerous flags would fail OPEN on the next one.
TIER_SELECTION_FLAGS = ("-L", "-LE", "-R", "-E")

# The one accepted spelling of the tier executable, matched LITERALLY — and
# the same string this guard resolves on PATH to run the query. One constant
# for both, so the recorded command and the invoked command cannot diverge.
#
# A basename match (`Path(argv[0]).name == "ctest"`) was the previous rule and
# was a false-pass generator, measured on the real tree:
#
#     GATE_EXCLUDE_TIER "/does/not/exist/ctest -L garmin-build-guard"
#       -> parse ACCEPTED, tier_selects() -> REACHABLE (certified)
#     GATE_EXCLUDE_TIER "/usr/bin/ctest -L garmin-build-guard"
#       -> parse ACCEPTED, tier_selects() -> REACHABLE (certified)
#
# In both cases the recorded token was discarded and PATH's ctest was queried
# instead, so the guard certified a command it had never run — the first one
# being a command that cannot be run at all. That is the same "excused to a
# tier that would never reach it" false pass the reachability rule exists to
# catch, so the rule was catching everyone except itself.
TIER_COMMAND = "ctest"

# This guard's own registered test names — see GATE-SELF above. BOTH entries
# matter: an earlier revision named only the first, so the pytest half could be
# given `gate-exclude` plus a well-formed reason and slip past all three rules
# while the adjacent CMake comment promised "NEITHER of these may ever carry
# gate-exclude". A guard that exempts half of itself is the bug it polices.
SELF_TEST_NAMES = (
    "testGarminGateCoverageGuard",
    "testGarminGateCoverageGuardUnits",
)


class RegistryError(RuntimeError):
    """The registry could not be read — never silently treated as 'no tests'."""


def load_registry(build_dir: Path) -> list[dict]:
    """Every registered test with its resolved properties, from ctest itself.

    Raises rather than returning [] when ctest cannot be run or returns
    nothing: a coverage guard that iterates an empty set is perfectly green,
    which is precisely the failure this whole unit exists to prevent.
    """
    ctest = shutil.which("ctest")
    if ctest is None:
        raise RegistryError("ctest not found on PATH")
    if not (build_dir / "CTestTestfile.cmake").is_file():
        raise RegistryError(
            f"{build_dir} is not a configured CMake build tree "
            "(no CTestTestfile.cmake) — run cmake first"
        )
    try:
        proc = subprocess.run(
            [ctest, "--show-only=json-v1"],
            cwd=str(build_dir),
            capture_output=True,
            text=True,
            timeout=QUERY_TIMEOUT_SECONDS,
        )
    except subprocess.TimeoutExpired as exc:
        raise RegistryError(
            f"ctest --show-only=json-v1 did not return within "
            f"{QUERY_TIMEOUT_SECONDS}s — a hung registry read would otherwise "
            f"hang the routine gate"
        ) from exc
    if proc.returncode != 0:
        raise RegistryError(
            f"ctest --show-only=json-v1 exited {proc.returncode}: "
            f"{proc.stderr.strip()[:200]}"
        )
    try:
        tests = json.loads(proc.stdout)["tests"]
    except (json.JSONDecodeError, KeyError) as exc:
        raise RegistryError(f"unparseable ctest registry: {exc}") from exc
    if not tests:
        raise RegistryError(
            "ctest reports ZERO registered tests — refusing to pass vacuously"
        )
    return tests


def properties_of(test: dict) -> dict[str, object]:
    """Flatten ctest's [{name, value}, ...] property list into a dict."""
    return {p["name"]: p["value"] for p in test.get("properties", [])}


def labels_of(test: dict) -> list[str]:
    """A test's LABELS, or [] when it carries none (the default-gated case)."""
    labels = properties_of(test).get("LABELS", [])
    return list(labels) if isinstance(labels, list) else [labels]


def selected_by_gate(build_dir: Path) -> set[str]:
    """The names `ctest -LE gate-exclude` SELECTS, asked of ctest itself.

    Asking ctest to do the selection is the point: it is the real behaviour of
    the real gate command, not this guard's model of it, so a future change to
    the gate command shows up as a disagreement rather than as silence.

    What this function does NOT tell you, stated here because an earlier
    revision of this docstring claimed the opposite and was wrong: SELECTION
    IS NOT EXECUTION. A `DISABLED` test appears in this set and never runs.
    Do not treat membership here as proof a test is covered — pair it with
    masking_properties_of(), which is where that gap is handled and evidenced
    — and note there are THREE such mechanisms, not just DISABLED.
    """
    ctest = shutil.which("ctest")
    if ctest is None:  # pragma: no cover - load_registry already raised
        raise RegistryError("ctest not found on PATH")
    try:
        proc = subprocess.run(
            [ctest, "-N", "-LE", GATE_EXCLUDE_LABEL, "--show-only=json-v1"],
            cwd=str(build_dir),
            capture_output=True,
            text=True,
            timeout=QUERY_TIMEOUT_SECONDS,
        )
    except subprocess.TimeoutExpired as exc:
        raise RegistryError(
            f"`ctest -N -LE {GATE_EXCLUDE_LABEL} --show-only=json-v1` did not "
            f"return within {QUERY_TIMEOUT_SECONDS}s"
        ) from exc
    if proc.returncode != 0:
        raise RegistryError(
            f"ctest -N -LE {GATE_EXCLUDE_LABEL} exited {proc.returncode}: "
            f"{proc.stderr.strip()[:200]}"
        )
    try:
        return {t["name"] for t in json.loads(proc.stdout)["tests"]}
    except (json.JSONDecodeError, KeyError) as exc:
        raise RegistryError(f"unparseable gate selection: {exc}") from exc


def masking_properties_of(
    test: dict, candidates: tuple[str, ...] = GATE_MASKING_PROPERTIES
) -> list[str]:
    """Which of the ENUMERATED result-masking properties this test carries.

    The name says what it does and nothing more, on purpose. Two previous
    revisions of this rule were findings for promising more than they checked:
    one called selection "coverage", the next called "not DISABLED"
    "RUNNABLE". This returns the intersection of `candidates` with the
    properties ctest reports for `test`. An empty list means "none of the
    THREE known masking mechanisms is present", which is strictly weaker than
    "this test will run and be counted", and the caller must not upgrade it.

    Each mechanism was measured, not read off the documentation. All three
    stay in the `ctest -N -LE gate-exclude` SELECTION, and all three remove
    the test's result from the gate's verdict:

        DISABLED                -> ***Not Run (Disabled); dropped from the
                                   denominator; `100% tests passed`
        SKIP_RETURN_CODE 1      -> a genuine `exit 1` becomes (Skipped), and
                                   ctest COUNTS SKIPPED AS PASSED while
                                   KEEPING it in the denominator
        SKIP_REGULAR_EXPRESSION -> same laundering, triggered by the test's
                                   own output rather than its exit status

    The second and third are strictly worse than the first in the arithmetic:
    a run of four tests, two of them skip-masked and one genuinely failing,
    printed `75% tests passed, 1 tests failed out of 4` — the masked failures
    were counted among the passes.

    PRESENCE versus TRUTH, because the value shapes differ and one of them is
    a trap. DISABLED and WILL_FAIL are CMake booleans: ctest normalises every
    true spelling to JSON `true` and OMITS the property when false, so the
    value is checked. SKIP_RETURN_CODE and SKIP_REGULAR_EXPRESSION are
    triggered by PRESENCE, because `SKIP_RETURN_CODE 0` is emitted as the
    falsy int `0` and masks a passing test into a skip — a truthiness test
    would wave it through — while an empty regex is never emitted at all.
    Whether a present property would actually FIRE at runtime is deliberately
    not modelled: that is an execution question, and modelling it is how this
    rule would drift back into overclaiming.
    """
    props = properties_of(test)
    found = []
    for name in candidates:
        if name not in props:
            continue
        if name in _BOOLEAN_SHAPED_PROPERTIES and props[name] not in _CMAKE_TRUE_VALUES:
            continue
        found.append(name)
    return found


def check_gate_coverage(tests: list[dict], gated: set[str]) -> list[str]:
    """GATE-COVERAGE: an unexcused test the gate does not select, or whose
    result one of the ENUMERATED masking properties would hide.

    Not "would not run" — that was the previous summary line here and it
    promised an execution guarantee this function does not have. See
    masking_properties_of() for what is and is not established.
    """
    findings = []
    for test in tests:
        name = test["name"]
        if GATE_EXCLUDE_LABEL in labels_of(test):
            continue
        if name not in gated:
            findings.append(
                f"GATE-COVERAGE {name}: registered but NOT selected by "
                f"`{GATE_COMMAND}`, and it carries no '{GATE_EXCLUDE_LABEL}' "
                f"label. Labels: {labels_of(test) or '(none)'}. Either it "
                f"belongs in the routine gate, or excuse it deliberately with "
                f"{REASON_PROPERTY} and {TIER_PROPERTY} (DEC-054)."
            )
        else:
            for prop in masking_properties_of(test):
                findings.append(
                    f"GATE-COVERAGE {name}: carries {prop}, so "
                    f"`{GATE_COMMAND}` SELECTS it but {_MASKING_EFFECT[prop]}. "
                    f"That removes a test from the gate without the "
                    f"'{GATE_EXCLUDE_LABEL}' label, without a reason and "
                    f"without a tier, which is the whole protocol DEC-054 "
                    f"exists to enforce. Remove {prop}, or excuse the test "
                    f"properly."
                )
    return findings


# CONFIGURATIONS: INVESTIGATED, NOT A RULE, AND DELIBERATELY SO.
#
# An earlier revision of check_gate_coverage() failed an unexcused test that
# carried a CONFIGURATIONS property, labelling the rule "precautionary". That
# branch has been DELETED. The knowledge is kept here so nobody re-litigates
# it from scratch, and so the deletion does not read as an oversight.
#
# What was probed (ctest 3.31.6, single-config Unix Makefiles and Ninja): a
# test carrying CONFIGURATIONS "Release" was SELECTED **and RUN** under
# `-C Debug` and even under `-C NotAConfig`. No bypass could be reproduced on
# any generator available on this machine. No test in this repository carries
# the property at all. A genuine multi-config generator (Visual Studio, Xcode,
# Ninja Multi-Config) was not available, so "CONFIGURATIONS never filters" is
# NOT asserted here — only that it could not be made to filter.
#
# Why that means no rule rather than a cheap precaution: this guard's whole
# authority rests on its rules being DEMONSTRATED. A hard red for an
# undemonstrated harm, sitting next to three rules that ARE demonstrated
# (GATE_MASKING_PROPERTIES), dilutes the instrument and trains readers to
# argue with its output instead of fixing what it names. If multi-config
# support ever becomes in-scope, probe it properly and build a rule from
# observed behaviour — not from this comment.


def parse_selection_tier(tier: str) -> tuple[list[str] | None, str]:
    """Parse a tier string against the allow-list, or explain the rejection.

    Returns `(args, "")` on success, where `args` are the VALIDATED flag/value
    pairs to run after the ctest binary — never the caller's tokens. On
    failure returns `(None, reason)` and nothing is executed.

    WHY AN ALLOW-LIST, AND WHY IT IS THIS SMALL. `GATE_EXCLUDE_TIER` is a
    string in CMake source that this guard hands to a real ctest binary, so
    its grammar is a capability decision. The previous revision validated only
    `Path(argv[0]).name == "ctest"` and passed every later token through
    unchecked, with a docstring claiming the basename made that safe. It does
    not, and the claim was measured false rather than argued against:

        tier = "ctest -S /tmp/insp124_dash_probe/evil.cmake"
        argv = [ctest, "-S", evil.cmake, "-N", "--show-only=json-v1"]
        -> SENTINEL EXISTS: the dashboard script EXECUTED and wrote to disk

    `-N --show-only=json-v1` suppresses TEST execution; it is not a capability
    boundary around `-S`. ctest then exited 255, so the old code would have
    reported a finding — AFTER the side effect. A check that reports the arson
    once the fire is lit is not a static check.

    A deny-list of the mode flags known today (`-S`, `-SP`, `-D`, `-M`, `-T`,
    `--build-and-test`) would fail OPEN on the next one ctest adds. This
    whole DEC is about failing SAFE, so the grammar is an allow-list and it is
    kept tiny: the LITERAL token TIER_COMMAND plus zero or more of
    TIER_SELECTION_FLAGS, each with exactly one argument. Anything else is
    rejected by name.

    THE FIRST TOKEN IS MATCHED LITERALLY, NOT BY BASENAME, and that is a
    separate guarantee from the mode confinement above. A basename rule
    accepted `/does/not/exist/ctest -L garmin-build-guard`, then tier_selects()
    discarded the recorded path and queried PATH's ctest, certifying an excuse
    against an executable it had never run — the same "excused to a tier that
    would never reach it" false pass the reachability rule exists to catch.
    The spelling is REJECTED rather than chased: executing the recorded path
    would hand mode-execution back to a repo-controlled string, which is
    precisely what this allow-list exists to prevent.

    WHAT THE ALLOW-LIST DOES AND DOES NOT GUARANTEE. It guarantees this
    function executes no ctest MODE: no dashboard script, no configure, no
    build, no test run — only a selection query, reconstructed from the parsed
    flags. It guarantees COMMAND IDENTITY: the executable named in the recorded
    tier is the executable queried, because both are TIER_COMMAND. It does NOT
    guarantee the query is cheap or terminates (hence
    QUERY_TIMEOUT_SECONDS at the call site), and it is NOT a sandbox: the
    flag ARGUMENTS still reach ctest, as regexes for `-R`/`-E` and label
    patterns for `-L`/`-LE`. It also does not judge whether the tier is a
    SENSIBLE tier — only that it is a selection query this guard may run.
    """
    try:
        argv = shlex.split(tier)
    except ValueError as exc:
        return None, f"is not parseable as a command ({exc})"
    if not argv:
        return None, (
            "is empty, so there is no command to verify; write the tier as a "
            "bare `ctest ...` command"
        )
    if argv[0] != TIER_COMMAND:
        return None, (
            f"names the executable {argv[0]!r}, but the tier must be written "
            f"as a bare `{TIER_COMMAND} ...` command. The match is LITERAL, "
            f"not a basename match: this guard runs the `{TIER_COMMAND}` found "
            f"on PATH, so accepting a path-bearing spelling would mean "
            f"certifying the recorded tier as reachable after querying a "
            f"DIFFERENT executable — including one that does not exist. "
            f"Rejecting the spelling is deliberate; the alternative is "
            f"executing the recorded path, which would reopen the mode-"
            f"execution surface this allow-list exists to close."
        )
    args: list[str] = []
    rest = argv[1:]
    index = 0
    while index < len(rest):
        token = rest[index]
        if token not in TIER_SELECTION_FLAGS:
            return None, (
                f"contains the token {token!r}, which this guard will not "
                f"execute. A tier may use only "
                f"{' '.join(TIER_SELECTION_FLAGS)}, each with exactly one "
                f"argument — no other flag, no `--flag=value` form and no "
                f"bare argument. The allow-list is deliberately tiny: ctest "
                f"mode flags such as -S run scripts and builds, and a "
                f"deny-list would fail open on the next one"
            )
        if index + 1 >= len(rest):
            return None, f"ends with {token!r}, which needs exactly one argument"
        value = rest[index + 1]
        if value.startswith("-"):
            # Otherwise `ctest -L -S` smuggles a mode flag through as a value:
            # this guard would have "validated" -S as -L's argument while
            # ctest's own parser may well read it as a flag. Rejected here
            # rather than reasoned about.
            return None, (
                f"gives {token!r} the argument {value!r}, which looks like a "
                f"flag; a selection argument may not begin with '-'"
            )
        args += [token, value]
        index += 2
    return args, ""


def tier_selects(build_dir: Path, tier: str, test_name: str) -> tuple[bool, str]:
    """Does the command in GATE_EXCLUDE_TIER actually select `test_name`?

    WHAT THIS PROVES, precisely, because overclaiming here is the mistake this
    whole round is about: it proves the recorded tier is a ctest selection
    query, within parse_selection_tier()'s allow-list, that really selects the
    excused test — i.e. that the test is reachable by the command someone
    wrote down. It does NOT prove that anything ever RUNS that command.
    Nothing in this repository does: there is no CI workflow and no scheduler,
    so an excused test is run when a human types the tier and not otherwise.
    That gap is real, is tracked as its own ledger row, and is NOT something a
    static check can close.

    The tier is parsed against the allow-list and the query is rebuilt FROM
    THE PARSE, not from the caller's tokens; nothing is passed to a shell.

    WHY "the recorded tier" IS AN HONEST DESCRIPTION OF WHAT WAS QUERIED, and
    it was not one revision ago. The parser requires argv[0] to be the literal
    string TIER_COMMAND, and this function resolves that SAME constant on PATH.
    Under the previous basename rule, a recorded tier of
    `/does/not/exist/ctest -L garmin-build-guard` parsed cleanly, its first
    token was discarded, PATH's ctest was queried with the recorded FILTERS,
    and the excuse was certified reachable — naming a command that could not
    be invoked at all. The claim is now about the command actually run.
    """
    args, why = parse_selection_tier(tier)
    if args is None:
        return False, why
    # Resolved from the SAME constant the parser matched argv[0] against, so
    # the executable named in the recorded tier and the executable queried
    # here are the same thing by construction rather than by coincidence.
    ctest = shutil.which(TIER_COMMAND)
    if ctest is None:  # pragma: no cover - load_registry already raised
        return False, f"{TIER_COMMAND} not found on PATH"
    try:
        proc = subprocess.run(
            [ctest, *args, "-N", "--show-only=json-v1"],
            cwd=str(build_dir),
            capture_output=True,
            text=True,
            timeout=QUERY_TIMEOUT_SECONDS,
        )
    except subprocess.TimeoutExpired:
        return False, (
            f"did not return within {QUERY_TIMEOUT_SECONDS}s when run as a "
            f"selection query — an unbounded tier query would hang the "
            f"routine gate rather than fail it"
        )
    if proc.returncode != 0:
        return False, f"exits {proc.returncode} when run as a selection query"
    try:
        selected = {t["name"] for t in json.loads(proc.stdout)["tests"]}
    except (json.JSONDecodeError, KeyError) as exc:
        return False, f"produced an unparseable selection ({exc})"
    if test_name not in selected:
        return False, (
            f"runs {len(selected)} test(s) but NOT this one — the test is "
            f"excused to a tier that would never reach it"
        )
    return True, ""


def check_excuse_metadata(tests: list[dict]) -> list[str]:
    """GATE-EXCUSE, the part that needs no build tree: is the excuse RECORDED?

    Reason present, reason above the floor, tier present. Nothing more.

    This exists as its own function so that check_gate_excuses() can take a
    MANDATORY build_dir. It used to default to None and silently skip the tier
    reachability invariant, which meant a future caller could get a weaker
    check by omitting an argument — the same shape of defect as the rest of
    this unit, one level down. The seam existed only so pure-logic tests could
    run without configuring a tree; that is what this function is for now.

    SYNTAX ONLY, and callers must not treat it as the GATE-EXCUSE rule: it
    does not verify the recorded tier selects anything. check_gate_excuses()
    is the complete rule and the only production entry point.
    """
    findings = []
    for test in tests:
        name = test["name"]
        if GATE_EXCLUDE_LABEL not in labels_of(test):
            continue
        props = properties_of(test)
        reason = str(props.get(REASON_PROPERTY, "") or "").strip()
        tier = str(props.get(TIER_PROPERTY, "") or "").strip()
        if not reason:
            findings.append(
                f"GATE-EXCUSE {name}: carries '{GATE_EXCLUDE_LABEL}' but no "
                f"{REASON_PROPERTY}. DEC-054: an opt-out without a recorded "
                f"reason is itself a defect."
            )
        elif len(reason) < MIN_REASON_CHARS:
            findings.append(
                f"GATE-EXCUSE {name}: {REASON_PROPERTY} is {len(reason)} "
                f"chars ({reason!r}), under the {MIN_REASON_CHARS}-char floor. "
                f"Say what makes this test unfit for the routine gate, with "
                f"the measurement that supports it."
            )
        if not tier:
            findings.append(
                f"GATE-EXCUSE {name}: carries '{GATE_EXCLUDE_LABEL}' but no "
                f"{TIER_PROPERTY}. An excused test must name the tier under "
                f"which it is run (e.g. 'ctest -L garmin-build-guard'), "
                f"otherwise it is not excused from the gate, it is unrun."
            )
    return findings


def check_gate_excuses(tests: list[dict], build_dir: Path) -> list[str]:
    """GATE-EXCUSE: an opt-out without a recorded, REACHABLE alternate tier.

    `build_dir` is mandatory. There is no weaker mode of this check to fall
    into by omitting an argument; the pure syntax half is
    check_excuse_metadata(), which says so in its own name.
    """
    findings = check_excuse_metadata(tests)
    for test in tests:
        name = test["name"]
        if GATE_EXCLUDE_LABEL not in labels_of(test):
            continue
        tier = str(properties_of(test).get(TIER_PROPERTY, "") or "").strip()
        if not tier:
            continue  # already reported by check_excuse_metadata()
        reachable, why = tier_selects(build_dir, tier, name)
        if not reachable:
            findings.append(
                f"GATE-EXCUSE {name}: {TIER_PROPERTY} {tier!r} {why}. An "
                f"excused test must at minimum be REACHABLE by the tier "
                f"recorded for it, or the excuse is decoration and the "
                f"test is simply gone."
            )
    return findings


def check_self_is_gated(tests: list[dict], gated: set[str]) -> list[str]:
    """GATE-SELF: BOTH of this guard's own tests must be inside the gate.

    Checked for every name in SELF_TEST_NAMES, not just the first: the guard
    and its unit tests are one instrument, and excusing either half disarms it
    just as effectively.

    The masking properties are prohibited here too, plus WILL_FAIL, which is
    prohibited for the self-tests ONLY (see SELF_PROHIBITED_PROPERTIES). The
    self-neutering case is the sharp one: `SKIP_RETURN_CODE 1` on this guard
    means that the moment it DETECTS an exclusion defect and exits 1, ctest
    launders that into Skipped, counts it as passed, and the gate stays green
    while the instrument screams into a void.
    """
    findings = []
    registered = {t["name"] for t in tests}
    by_name = {t["name"]: t for t in tests}
    for name in SELF_TEST_NAMES:
        if name not in registered:
            findings.append(
                f"GATE-SELF {name}: this guard is not registered as a ctest "
                f"test. An unrun coverage guard protects nothing."
            )
            continue
        if GATE_EXCLUDE_LABEL in labels_of(by_name[name]):
            findings.append(
                f"GATE-SELF {name}: the gate-coverage guard carries "
                f"'{GATE_EXCLUDE_LABEL}' — a coverage guard outside the gate "
                f"is the original bug wearing a new hat (DEC-054). No reason "
                f"is good enough here; remove the label."
            )
            continue
        if name not in gated:
            findings.append(
                f"GATE-SELF {name}: the gate-coverage guard is registered but "
                f"`{GATE_COMMAND}` does not select it — a coverage guard "
                f"outside the gate is the original bug wearing a new hat "
                f"(DEC-054)."
            )
            continue
        for prop in masking_properties_of(by_name[name], SELF_PROHIBITED_PROPERTIES):
            findings.append(
                f"GATE-SELF {name}: the gate-coverage guard carries {prop}, "
                f"so {_MASKING_EFFECT[prop]}. The gate would stay green while "
                f"its own self-policing instrument is neutralised, which is "
                f"the original bug wearing a new hat (DEC-054). No reason is "
                f"good enough here; remove {prop}."
            )
    return findings


def main(argv: list[str]) -> int:
    if len(argv) != 2:
        print(f"usage: {Path(argv[0]).name} <build-dir>", file=sys.stderr)
        return 2
    build_dir = Path(argv[1]).resolve()

    try:
        tests = load_registry(build_dir)
        gated = selected_by_gate(build_dir)
    except RegistryError as exc:
        print(f"garmin-gate-coverage-guard: UNUSABLE — {exc}", file=sys.stderr)
        return 2

    findings = (
        check_gate_coverage(tests, gated)
        + check_gate_excuses(tests, build_dir)
        + check_self_is_gated(tests, gated)
    )

    excused = [t["name"] for t in tests if GATE_EXCLUDE_LABEL in labels_of(t)]
    if findings:
        print(f"garmin-gate-coverage-guard: FAIL — {len(findings)} finding(s)")
        for finding in findings:
            print(f"  {finding}")
        return 1

    # The counts are printed on the PASS path on purpose: "0 findings" from a
    # guard that examined nothing looks identical to a real pass, and this
    # project has shipped that mistake before.
    print(
        f"garmin-gate-coverage-guard: PASS — {len(tests)} registered tests, "
        f"{len(gated)} selected by `{GATE_COMMAND}`, {len(excused)} excused "
        f"with a recorded reason and tier ({', '.join(excused) or 'none'}), "
        f"0 findings"
    )
    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv))
