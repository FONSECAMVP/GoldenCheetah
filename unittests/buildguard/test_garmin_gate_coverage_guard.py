#!/usr/bin/env python3
"""B-STAGE9-16 / DEC-054 — unit tests for the gate-coverage guard.

Why this file exists, stated bluntly. The guard it tests is a COVERAGE check,
and a coverage check is the easiest kind of test to make vacuous: one that
iterates an empty registry, or that builds its expectation from the same
source as the thing it checks, passes green forever while protecting nothing.
That is the exact failure DEC-054 exists to stop, so it would be absurd to
protect against it with a test that has the same disease.

Two consequences for how this file is written:

  * The end-to-end cases configure a REAL, tiny CMake project in a tmp dir and
    run REAL ctest against it. They are not mocks of ctest's output. Custom
    properties surviving into `--show-only=json-v1` is load-bearing for the
    whole design, so it is asserted against ctest itself rather than assumed.
  * Every positive assertion ("the guard passes here") is paired with a
    negative one ("...and here is the mutation of that same fixture that makes
    it fail"). A guard that cannot go red is not a guard.
"""

from __future__ import annotations

import shutil
import subprocess
import sys
from pathlib import Path

import pytest

sys.path.insert(0, str(Path(__file__).resolve().parent))

import garmin_gate_coverage_guard as guard  # noqa: E402
from garmin_gate_coverage_guard import (  # noqa: E402
    GATE_EXCLUDE_LABEL,
    GATE_MASKING_PROPERTIES,
    MIN_REASON_CHARS,
    QUERY_TIMEOUT_SECONDS,
    REASON_PROPERTY,
    SELF_PROHIBITED_PROPERTIES,
    SELF_TEST_NAMES,
    TIER_COMMAND,
    TIER_PROPERTY,
    TIER_SELECTION_FLAGS,
    RegistryError,
    check_excuse_metadata,
    check_gate_coverage,
    check_gate_excuses,
    check_self_is_gated,
    labels_of,
    load_registry,
    main,
    masking_properties_of,
    parse_selection_tier,
    properties_of,
    selected_by_gate,
    tier_selects,
)

GOOD_REASON = (
    "Full from-scratch configure + app build per test; measured 247s/256s, "
    "far too slow for a per-unit gate."
)
GOOD_TIER = "ctest -L garmin-build-guard"


# --------------------------------------------------------------------------
# Fixtures: real CMake projects, not mocked ctest output.
# --------------------------------------------------------------------------
def _entry(
    name: str,
    labels: str | None = None,
    reason: str | None = None,
    tier: str | None = None,
    disabled: bool = False,
    skip_return_code: int | None = None,
    skip_regex: str | None = None,
    will_fail: bool = False,
    command: str = "${CMAKE_COMMAND} -E true",
) -> str:
    """One add_test() plus its properties, as CMake source."""
    lines = [f"add_test(NAME {name} COMMAND {command})"]
    props = []
    if labels is not None:
        props.append(f'    LABELS "{labels}"')
    if reason is not None:
        props.append(f'    {REASON_PROPERTY} "{reason}"')
    if tier is not None:
        props.append(f'    {TIER_PROPERTY} "{tier}"')
    if disabled:
        props.append("    DISABLED TRUE")
    if skip_return_code is not None:
        props.append(f"    SKIP_RETURN_CODE {skip_return_code}")
    if skip_regex is not None:
        props.append(f'    SKIP_REGULAR_EXPRESSION "{skip_regex}"')
    if will_fail:
        props.append("    WILL_FAIL TRUE")
    if props:
        lines.append(f"set_tests_properties({name} PROPERTIES")
        lines.extend(props)
        lines.append(")")
    return "\n".join(lines)


def configure(tmp_path: Path, *entries: str) -> Path:
    """Configure a throwaway CMake project and return its build dir."""
    source = tmp_path / "s"
    source.mkdir(exist_ok=True)
    body = "\n".join(
        [
            "cmake_minimum_required(VERSION 3.16)",
            "project(gatecov NONE)",
            "enable_testing()",
            *entries,
        ]
    )
    (source / "CMakeLists.txt").write_text(body, encoding="utf-8")
    build = tmp_path / "b"
    build.mkdir(exist_ok=True)
    done = subprocess.run(
        ["cmake", str(source)], cwd=str(build), capture_output=True, text=True
    )
    assert done.returncode == 0, f"fixture cmake failed: {done.stderr}"
    return build


# Both of the guard's own registrations, which GATE-SELF requires to be
# present and gated. Every fixture below includes them, because a tree missing
# them is a DIFFERENT finding and would mask the one under test.
SELF_ENTRIES = [_entry(name) for name in SELF_TEST_NAMES]


@pytest.fixture
def healthy_build(tmp_path: Path) -> Path:
    """The shape the real tree is supposed to have after DEC-054."""
    return configure(
        tmp_path,
        *SELF_ENTRIES,
        _entry("testUnlabelled"),
        _entry("testOrdinary", labels="garmin-fast"),
        _entry(
            "testSlowBuildGuard",
            labels=f"garmin-build-guard;{GATE_EXCLUDE_LABEL}",
            reason=GOOD_REASON,
            tier=GOOD_TIER,
        ),
    )


# --------------------------------------------------------------------------
# The registry really is the registry — the premise the design rests on.
# --------------------------------------------------------------------------
def test_custom_properties_survive_into_the_ctest_registry(
    healthy_build: Path,
) -> None:
    """Load-bearing premise, asserted against ctest rather than assumed.

    The reason and tier are checkable only because ctest reports arbitrary
    test properties in `--show-only=json-v1`. If a future CMake/CTest dropped
    unknown properties from that output, this test is where that shows up.

    Correcting what this docstring used to claim, because a wrong docstring is
    a defect here in either direction: it said losing the properties would let
    the guard "pass while checking nothing". It would not. A `gate-exclude`
    test whose REASON and TIER vanished is reported by check_gate_excuses() as
    missing both, and the guard exits nonzero — it fails LOUDLY, which is the
    safe direction. What would genuinely be lost is the ability to record a
    valid excuse at all, so every excused test would become a false finding.
    That is still worth catching here, and is what this test actually pins.
    """
    tests = {t["name"]: t for t in load_registry(healthy_build)}
    props = properties_of(tests["testSlowBuildGuard"])
    assert props[REASON_PROPERTY] == GOOD_REASON
    assert props[TIER_PROPERTY] == GOOD_TIER
    assert GATE_EXCLUDE_LABEL in labels_of(tests["testSlowBuildGuard"])
    assert labels_of(tests["testUnlabelled"]) == []


def test_gate_selection_is_asked_of_ctest_not_modelled(
    healthy_build: Path,
) -> None:
    """`ctest -LE gate-exclude` really does default-INCLUDE the unlabelled.

    This is the whole point of DEC-054 and the one behaviour the old opt-in
    gate did not have. A test carrying no labels at all must be selected.
    """
    gated = selected_by_gate(healthy_build)
    assert "testUnlabelled" in gated, "default-include is broken: " + str(gated)
    assert "testOrdinary" in gated
    assert set(SELF_TEST_NAMES) <= gated
    assert "testSlowBuildGuard" not in gated, "the opt-out did not take effect"


# --------------------------------------------------------------------------
# Anti-vacuity: the guard must refuse to pass on an empty or unreadable
# registry rather than iterate nothing and report success.
# --------------------------------------------------------------------------
def test_empty_registry_is_an_error_not_a_pass(tmp_path: Path) -> None:
    """A coverage guard over zero tests is green and worthless.

    This is the single most likely way this guard would rot: point it at the
    wrong directory, or at a tree configured before any test existed, and a
    naive implementation reports PASS. It must raise instead.
    """
    build = configure(tmp_path)  # a valid project with no tests at all
    with pytest.raises(RegistryError, match="ZERO registered tests"):
        load_registry(build)


def test_unconfigured_directory_is_an_error_not_a_pass(tmp_path: Path) -> None:
    """Pointing the guard at a non-build-dir must fail loudly."""
    with pytest.raises(RegistryError, match="not a configured CMake build tree"):
        load_registry(tmp_path)


def test_main_returns_2_and_prints_nothing_green_on_bad_build_dir(
    tmp_path: Path, capsys: pytest.CaptureFixture[str]
) -> None:
    """The exit code a CI/ctest run would see must not be 0."""
    assert main(["garmin_gate_coverage_guard.py", str(tmp_path)]) == 2
    captured = capsys.readouterr()
    assert "UNUSABLE" in captured.err
    assert "PASS" not in captured.out


# --------------------------------------------------------------------------
# GATE-COVERAGE — the central assertion, checked in BOTH directions.
# --------------------------------------------------------------------------
def test_healthy_tree_passes_end_to_end(
    healthy_build: Path, capsys: pytest.CaptureFixture[str]
) -> None:
    """The correct shape passes, and says what it examined while doing so."""
    assert main(["garmin_gate_coverage_guard.py", str(healthy_build)]) == 0
    out = capsys.readouterr().out
    assert "PASS" in out
    # The counts are part of the contract: "0 findings" from a guard that
    # examined nothing reads identically to a real pass.
    assert "5 registered tests" in out
    assert "4 selected" in out
    assert "1 excused" in out


def test_unexcused_test_outside_the_gate_is_a_finding() -> None:
    """The defect DEC-054 is about: registered, ungated, unexcused, silent.

    Synthesised rather than configured, because the gate command currently
    cannot produce this state by itself — that is the point. It models a
    DISABLED property, a CONFIGURATIONS restriction, or a future edit to the
    gate command dropping a test from selection while it carries no opt-out.
    """
    tests = [
        {"name": "testGated", "properties": []},
        {"name": "testInvisible", "properties": []},
    ]
    findings = check_gate_coverage(tests, gated={"testGated"})
    assert len(findings) == 1
    assert findings[0].startswith("GATE-COVERAGE testInvisible")
    assert "carries no 'gate-exclude' label" in findings[0]


def test_excused_test_outside_the_gate_is_not_a_finding() -> None:
    """The opt-out is legitimate — otherwise the label would mean nothing."""
    tests = [
        {
            "name": "testSlow",
            "properties": [{"name": "LABELS", "value": [GATE_EXCLUDE_LABEL]}],
        }
    ]
    assert check_gate_coverage(tests, gated=set()) == []


def test_every_unexcused_test_is_checked_not_just_the_first() -> None:
    """Guards against the classic `return` inside the loop.

    An implementation that stops at the first offender passes a one-offender
    fixture identically to a correct one, so the corpus here has three.
    """
    tests = [{"name": f"testMissing{i}", "properties": []} for i in range(3)]
    findings = check_gate_coverage(tests, gated=set())
    assert len(findings) == 3, findings
    assert {f.split()[1].rstrip(":") for f in findings} == {
        "testMissing0",
        "testMissing1",
        "testMissing2",
    }


# --------------------------------------------------------------------------
# GATE-EXCUSE — the exclusion list is the one place the new scheme can rot,
# so it is the place that is policed.
#
# The cases below call check_excuse_metadata(), the pure syntax half, because
# they are about what is RECORDED. check_gate_excuses() — the complete rule,
# and the only production entry point — takes a MANDATORY build dir and is
# exercised against real configured trees further down. That split replaced a
# `build_dir=None` default that silently skipped the tier-reachability
# invariant; the test asserting that weaker behaviour was deleted with it,
# rather than being kept as a pin on a state that can no longer occur.
# --------------------------------------------------------------------------
def _excused(name: str, reason: str | None, tier: str | None) -> dict:
    props: list[dict] = [{"name": "LABELS", "value": [GATE_EXCLUDE_LABEL]}]
    if reason is not None:
        props.append({"name": REASON_PROPERTY, "value": reason})
    if tier is not None:
        props.append({"name": TIER_PROPERTY, "value": tier})
    return {"name": name, "properties": props}


def test_excuse_with_reason_and_tier_is_accepted() -> None:
    assert check_excuse_metadata([_excused("testSlow", GOOD_REASON, GOOD_TIER)]) == []


def test_excuse_without_a_reason_is_a_finding() -> None:
    """DEC-054: an opt-out without a recorded reason is itself a defect."""
    findings = check_excuse_metadata([_excused("testSlow", None, GOOD_TIER)])
    assert len(findings) == 1
    assert f"no {REASON_PROPERTY}" in findings[0]


def test_excuse_without_a_tier_is_a_finding() -> None:
    """Excused from the gate must not silently mean never run at all."""
    findings = check_excuse_metadata([_excused("testSlow", GOOD_REASON, None)])
    assert len(findings) == 1
    assert f"no {TIER_PROPERTY}" in findings[0]
    assert "it is not excused from the gate, it is unrun" in findings[0]


def test_excuse_with_neither_reports_both() -> None:
    """Two independent defects must not collapse into one message."""
    findings = check_excuse_metadata([_excused("testSlow", None, None)])
    assert len(findings) == 2


@pytest.mark.parametrize("reason", ["", "   ", "slow", "TODO", "see above"])
def test_token_reasons_are_rejected(reason: str) -> None:
    """A reason has to say something.

    No static check can enforce that a reason is TRUE — review does that, and
    this guard is explicitly the complement, not the main protection. What it
    can enforce is a floor against the one-word placeholder that makes an
    opt-out look reasoned when it is not.
    """
    assert len(reason.strip()) < MIN_REASON_CHARS  # the fixture is honest
    findings = check_excuse_metadata([_excused("testSlow", reason, GOOD_TIER)])
    assert len(findings) == 1, findings
    assert "GATE-EXCUSE" in findings[0]


def test_whitespace_only_tier_is_not_a_tier() -> None:
    """`TIER "   "` must not satisfy the check by being non-empty."""
    findings = check_excuse_metadata([_excused("testSlow", GOOD_REASON, "   ")])
    assert len(findings) == 1
    assert f"no {TIER_PROPERTY}" in findings[0]


def test_a_real_excuse_just_over_the_floor_is_accepted() -> None:
    """The floor must not be so high that honest short reasons fail.

    Pins the boundary from the other side, so moving MIN_REASON_CHARS shows up
    in a diff rather than silently tightening or loosening the rule.
    """
    just_enough = "x" * MIN_REASON_CHARS
    assert check_excuse_metadata([_excused("t", just_enough, GOOD_TIER)]) == []
    one_short = "x" * (MIN_REASON_CHARS - 1)
    assert len(check_excuse_metadata([_excused("t", one_short, GOOD_TIER)])) == 1


# --------------------------------------------------------------------------
# GATE-SELF — a coverage guard outside the gate is the original bug in a hat.
# --------------------------------------------------------------------------
def test_self_must_be_registered() -> None:
    other = [{"name": "testOther", "properties": []}]
    findings = check_self_is_gated(other, gated=set())
    # One finding per missing registration — BOTH halves are required, so a
    # tree containing neither reports both rather than stopping at the first.
    assert len(findings) == len(SELF_TEST_NAMES), findings
    assert all("not registered as a ctest test" in f for f in findings)
    assert {f.split()[1].rstrip(":") for f in findings} == set(SELF_TEST_NAMES)


def test_self_must_not_be_excluded_from_its_own_gate() -> None:
    """The precise bug this rule exists to prevent."""
    tests = [
        {
            "name": name,
            "properties": [{"name": "LABELS", "value": [GATE_EXCLUDE_LABEL]}],
        }
        for name in SELF_TEST_NAMES
    ]
    findings = check_self_is_gated(tests, gated=set())
    assert len(findings) == len(SELF_TEST_NAMES)
    assert all("original bug wearing a new hat" in f for f in findings)


def test_self_inside_the_gate_is_accepted() -> None:
    tests = [{"name": n, "properties": []} for n in SELF_TEST_NAMES]
    assert check_self_is_gated(tests, gated=set(SELF_TEST_NAMES)) == []


# --------------------------------------------------------------------------
# DISABLED — selection is not execution. The bypass that made every rule in
# this file pass on a tree with a test silently removed from the gate.
# --------------------------------------------------------------------------
def test_disabled_test_is_still_in_the_gate_selection(tmp_path: Path) -> None:
    """The premise of the bug, asserted against real ctest, not assumed.

    If this ever stops being true — if a future ctest drops DISABLED tests
    from `-LE` selection — then the DISABLED rule below becomes dead code and
    should be revisited rather than left as decoration.
    """
    build = configure(
        tmp_path,
        *SELF_ENTRIES,
        _entry("testDisabled", disabled=True),
    )
    assert "testDisabled" in selected_by_gate(build), (
        "ctest no longer selects DISABLED tests; the DISABLED rule in "
        "masking_properties_of() may now be unreachable"
    )


def test_disabled_unexcused_test_is_a_finding() -> None:
    """A test removed from the gate without using the opt-out protocol.

    This is the whole of F2: `DISABLED TRUE` takes a test out of the gate
    while leaving it in the selection set, so the pre-fix guard — which
    treated selection as coverage — reported PASS on a tree where a
    registered, unexcused test never ran and ctest printed '100% tests
    passed'. No label, no reason, no tier, no red.
    """
    tests = [
        {
            "name": "testSneakilyDisabled",
            "properties": [{"name": "DISABLED", "value": True}],
        }
    ]
    findings = check_gate_coverage(tests, gated={"testSneakilyDisabled"})
    assert len(findings) == 1, findings
    assert "carries DISABLED" in findings[0]
    assert "never RUNS it" in findings[0]


def test_disabled_but_properly_excused_test_is_not_a_finding() -> None:
    """The opt-out protocol still works; DISABLED is not banned outright.

    Without this, the rule above could be passing because the guard rejects
    every DISABLED test unconditionally, which would be a different rule than
    the one intended.
    """
    tests = [
        {
            "name": "testSlow",
            "properties": [
                {"name": "LABELS", "value": [GATE_EXCLUDE_LABEL]},
                {"name": "DISABLED", "value": True},
            ],
        }
    ]
    assert check_gate_coverage(tests, gated=set()) == []


@pytest.mark.parametrize("raw", [True, "True", "ON", "1"])
def test_disabled_is_recognised_in_every_spelling_ctest_emits(raw: object) -> None:
    """CMake booleans are not one spelling, and the JSON is not typed prose.

    Only `True` actually occurs: real ctest normalises every CMake-true
    spelling to a JSON boolean (probed across 12 of them) and omits the
    property when false. The string entries are dead defensive code, kept
    because removing them is a change with no upside; this test documents that
    they are dead rather than implying ctest emits them.
    """
    tests = [{"name": "t", "properties": [{"name": "DISABLED", "value": raw}]}]
    found = masking_properties_of(tests[0])
    assert found == ["DISABLED"], f"DISABLED={raw!r} not recognised (got {found})"


def test_absent_or_false_disabled_carries_no_masking_property() -> None:
    """The other side of the boundary, so the rule cannot drift into 'always'."""
    assert masking_properties_of({"name": "t", "properties": []}) == []
    assert (
        masking_properties_of(
            {"name": "t", "properties": [{"name": "DISABLED", "value": False}]}
        )
        == []
    )


def test_guard_goes_red_end_to_end_on_a_disabled_unexcused_test(
    tmp_path: Path,
) -> None:
    """Through real cmake and real ctest, not synthesised dicts."""
    build = configure(
        tmp_path,
        *SELF_ENTRIES,
        _entry("testQuietlyDisabled", disabled=True),
    )
    assert main(["garmin_gate_coverage_guard.py", str(build)]) == 1


# --------------------------------------------------------------------------
# GATE_EXCLUDE_TIER must be reachable — F3(b). It does NOT prove anything
# runs the tier; see tier_selects()'s docstring.
# --------------------------------------------------------------------------
def test_tier_that_really_selects_the_test_is_accepted(tmp_path: Path) -> None:
    build = configure(
        tmp_path,
        *SELF_ENTRIES,
        _entry(
            "testSlowBuildGuard",
            labels=f"garmin-build-guard;{GATE_EXCLUDE_LABEL}",
            reason=GOOD_REASON,
            tier="ctest -L garmin-build-guard",
        ),
    )
    tests = load_registry(build)
    assert check_gate_excuses(tests, build) == []


def test_tier_that_does_not_reach_the_test_is_a_finding(tmp_path: Path) -> None:
    """Excused to nowhere — the rot F3 predicted, now detectable."""
    build = configure(
        tmp_path,
        *SELF_ENTRIES,
        _entry(
            "testSlowBuildGuard",
            labels=f"garmin-build-guard;{GATE_EXCLUDE_LABEL}",
            reason=GOOD_REASON,
            tier="ctest -L a-label-nothing-carries",
        ),
    )
    findings = check_gate_excuses(load_registry(build), build)
    assert len(findings) == 1, findings
    assert "NOT this one" in findings[0]


def test_tier_that_is_not_a_ctest_command_is_a_finding(tmp_path: Path) -> None:
    """An unverifiable tier is not a tier; and nothing gets shell-executed."""
    build = configure(
        tmp_path,
        *SELF_ENTRIES,
        _entry(
            "testSlowBuildGuard",
            labels=GATE_EXCLUDE_LABEL,
            reason=GOOD_REASON,
            tier="ask Andy to run it by hand",
        ),
    )
    findings = check_gate_excuses(load_registry(build), build)
    assert len(findings) == 1, findings
    # Rejected on the first token, naming it. This assertion used to pin the
    # phrase "is not a ctest invocation"; the R3-F1 fix replaced that message
    # with one that names the offending executable, because the rule is now
    # about the LITERAL spelling rather than about looking ctest-ish.
    assert "names the executable 'ask'" in findings[0], findings[0]
    assert "bare `ctest ...` command" in findings[0], findings[0]


def test_check_gate_excuses_requires_a_build_dir() -> None:
    """There is no weaker mode of the rule to reach by omitting an argument.

    This replaces a test that asserted the OPPOSITE — that reachability is
    skipped when `build_dir` is omitted. That seam existed for the pure-logic
    tests, but it was also a way for a future caller to get a check that
    returns success without performing the F3(b) invariant. The parameter is
    now mandatory, so the weaker mode cannot occur and is not pinned; the
    syntax-only half is check_excuse_metadata(), which is named for what it is.
    """
    excused = _excused("testSlow", GOOD_REASON, "ctest -L whatever")
    with pytest.raises(TypeError):
        check_gate_excuses([excused])  # type: ignore[call-arg]


# --------------------------------------------------------------------------
# GATE-SELF must cover BOTH of the guard's own tests — F4.
# --------------------------------------------------------------------------
@pytest.mark.parametrize("victim", SELF_TEST_NAMES)
def test_neither_half_of_the_guard_may_excuse_itself(victim: str) -> None:
    """The reviewer's exact bypass: excuse the Units half and walk through.

    Parametrised over SELF_TEST_NAMES so that EVERY name currently in the
    tuple is covered, rather than just the first — which is the bug F4 was.

    Stated narrowly, because the previous wording here claimed this made it
    impossible to "silently leave a third registration unprotected", and that
    was false. Pytest parametrises the PYTHON TUPLE; nothing in this file
    reads the CMake registrations. A third guard test could be added to
    unittests/buildguard/CMakeLists.txt with SELF_TEST_NAMES untouched, and
    this test would keep passing while the new registration went unprotected.
    Closing that would need a real source/registry correspondence check,
    which is a design addition rather than a repair and is not built here.
    """
    tests = [
        {
            "name": name,
            "properties": (
                [{"name": "LABELS", "value": [GATE_EXCLUDE_LABEL]}]
                if name == victim
                else []
            ),
        }
        for name in SELF_TEST_NAMES
    ]
    gated = {n for n in SELF_TEST_NAMES if n != victim}
    findings = check_self_is_gated(tests, gated)
    assert len(findings) == 1, findings
    assert victim in findings[0]
    assert "original bug wearing a new hat" in findings[0]


def test_a_well_formed_excuse_cannot_launder_the_guard_itself(
    tmp_path: Path,
) -> None:
    """End-to-end: reason + tier + label on the Units half must still fail.

    This is the precise thing the reviewer demonstrated passing before F4.
    """
    build = configure(
        tmp_path,
        _entry("testGarminGateCoverageGuard"),
        _entry(
            "testGarminGateCoverageGuardUnits",
            labels=f"garmin-gate-guard;{GATE_EXCLUDE_LABEL}",
            reason=GOOD_REASON,
            tier="ctest -L garmin-gate-guard",
        ),
    )
    assert main(["garmin_gate_coverage_guard.py", str(build)]) == 1


def test_both_self_tests_present_and_gated_is_accepted() -> None:
    tests = [{"name": n, "properties": []} for n in SELF_TEST_NAMES]
    assert check_self_is_gated(tests, gated=set(SELF_TEST_NAMES)) == []


def test_guard_goes_red_end_to_end_on_an_unreasoned_opt_out(
    tmp_path: Path,
) -> None:
    """The headline failure path, through real cmake and real ctest.

    Everything above tests a function; this tests the program. A test opts out
    with no reason and no tier — exactly the rot DEC-054 predicts for the
    exclusion list — and the guard must exit 1.
    """
    build = configure(
        tmp_path,
        *SELF_ENTRIES,
        _entry("testSneakyOptOut", labels=GATE_EXCLUDE_LABEL),
    )
    assert main(["garmin_gate_coverage_guard.py", str(build)]) == 1


# --------------------------------------------------------------------------
# R2-F1 — the tier string must be INCAPABLE of executing a ctest mode.
#
# GATE_EXCLUDE_TIER is repo-controlled text that this guard feeds to a real
# ctest binary. The previous revision validated only argv[0]'s basename and
# passed the rest through, with a docstring claiming that made it safe. It was
# measured false: `ctest -S script.cmake -N --show-only=json-v1` EXECUTES the
# script. So the assertion that matters below is not "the function returned
# False" — a rejection after the side effect is worthless — it is that the
# SIDE EFFECT NEVER HAPPENED.
# --------------------------------------------------------------------------
def _sentinel_script(tmp_path: Path) -> tuple[Path, Path]:
    """A CMake script whose only job is to prove it ran, and its sentinel."""
    sentinel = tmp_path / "SENTINEL_EXECUTED"
    script = tmp_path / "evil.cmake"
    script.write_text(
        f'file(WRITE "{sentinel.as_posix()}" "the dashboard script ran")\n',
        encoding="utf-8",
    )
    return script, sentinel


def test_dash_s_tier_is_rejected_and_the_script_never_runs(tmp_path: Path) -> None:
    """The whole of R2-F1, asserted on the side effect and not the verdict.

    `-N --show-only=json-v1` suppresses TEST execution; it is NOT a capability
    boundary around `-S`. Before the allow-list, this exact argv shape ran the
    script and wrote the sentinel, and only THEN did ctest exit 255 so that
    tier_selects() reported a finding. A guard that reports the arson after
    lighting the fire is not a static check, so this test would fail against
    that implementation even though it returned False.
    """
    script, sentinel = _sentinel_script(tmp_path)
    build = configure(tmp_path, *SELF_ENTRIES)

    reachable, why = tier_selects(build, f"ctest -S {script}", "testAnything")

    # This assertion is deliberately FIRST. The pre-fix implementation also
    # returned False here (ctest exited 255 after running the script), so a
    # test that checked the verdict before the side effect would have reported
    # the wrong reason for failing — or passed outright.
    assert not sentinel.exists(), (
        "REGRESSION: the dashboard script EXECUTED. The tier string reached a "
        "ctest mode flag and wrote to the filesystem before being rejected."
    )
    assert not reachable
    assert "-S" in why, why


def test_dash_s_is_rejected_by_the_parser_without_any_subprocess(
    monkeypatch: pytest.MonkeyPatch, tmp_path: Path
) -> None:
    """Rejection happens in the PARSE, before anything could be executed.

    The test above proves the sentinel is unwritten; this one proves why —
    no subprocess is spawned at all. Pinning the mechanism as well as the
    outcome, because "no side effect observed" can also be true of an
    implementation that merely got lucky with a flag ctest ignored.
    """
    called = []
    monkeypatch.setattr(
        guard.subprocess, "run", lambda *a, **k: called.append(a) or None
    )
    args, why = parse_selection_tier("ctest -S /tmp/whatever.cmake")
    assert args is None
    assert "-S" in why
    assert called == [], "a subprocess was spawned while validating the tier"


@pytest.mark.parametrize(
    "tier",
    [
        "ctest -S /tmp/script.cmake",
        "ctest -SP /tmp/script.cmake",
        "ctest -D Experimental",
        "ctest -M Nightly -T Test",
        "ctest -T Test",
        "ctest --build-and-test /src /build",
        "ctest --overwrite BuildDirectory=/tmp/x",
        "ctest -L garmin-fast --output-on-failure",
        "ctest -L=garmin-fast",
        "ctest --label-regex garmin-fast",
        "ctest garmin-fast",
        "ctest -L garmin-fast extra-positional",
        "not-ctest -L garmin-fast",
    ],
)
def test_only_the_allow_listed_selection_grammar_is_accepted(tier: str) -> None:
    """Everything outside the tiny grammar is rejected, by name.

    Includes the forms R2-F1 named (`-S`, `-SP`, `-D`, `-M`, `-T`,
    `--build-and-test`), the `--flag=value` form, a long-option spelling of an
    allowed flag, and bare positionals.

    Note what is NOT claimed: this is not an enumeration of every dangerous
    ctest flag. It cannot be, and that is the point of an allow-list — the
    grammar is closed, so a flag nobody here has heard of is rejected by
    default rather than needing to appear in a list like this one.
    """
    args, why = parse_selection_tier(tier)
    assert args is None, f"{tier!r} was accepted as {args!r}"
    assert why, "a rejection must explain itself"


@pytest.mark.parametrize("flag", TIER_SELECTION_FLAGS)
def test_each_allow_listed_flag_is_accepted_with_one_argument(flag: str) -> None:
    """The other side of the boundary: the grammar must not reject everything.

    Without this, every test above could be passing because the parser
    rejects all input, which would make the tier rule dead code wearing a
    check's clothes.
    """
    args, why = parse_selection_tier(f"ctest {flag} some-label")
    assert args == [flag, "some-label"], why


def test_several_allow_listed_flags_may_be_combined() -> None:
    args, why = parse_selection_tier("ctest -L garmin-build-guard -E slow")
    assert args == ["-L", "garmin-build-guard", "-E", "slow"], why


@pytest.mark.parametrize(
    "executable",
    [
        "/does/not/exist/ctest",
        "/usr/bin/ctest",
        "some/ctest",
        "./ctest",
        "../ctest",
        "/usr/bin/../bin/ctest",
    ],
)
def test_a_path_bearing_ctest_spelling_is_rejected(executable: str) -> None:
    """R3-F1: the first token is matched LITERALLY, not by basename.

    `/usr/bin/ctest` is in this corpus deliberately, and it is the case that
    makes the point: a test covering only the NONEXISTENT path would pass
    against an implementation that merely checked the file exists, which is
    not the defect. The defect is that the recorded token is DISCARDED and
    PATH's ctest is queried instead, so the guard certifies an excuse against
    an executable it never ran. That is wrong even when the recorded path is
    real and even when it happens to be the same binary — the guard has not
    established that, and "excused to a tier that would never reach it" is the
    exact false pass the reachability rule exists to catch.

    Measured before the fix, against the real build tree, for the two real
    excused tests: BOTH `/does/not/exist/ctest -L garmin-build-guard` and
    `/usr/bin/ctest -L garmin-build-guard` parsed cleanly and were certified
    `REACHABLE`.
    """
    args, why = parse_selection_tier(f"{executable} -L garmin-build-guard")
    assert args is None, f"{executable!r} was accepted as {args!r}"
    assert executable in why, why
    assert "bare `ctest ...` command" in why, why


def test_an_empty_tier_is_rejected_without_an_index_error() -> None:
    """The `not argv` branch, split out from the executable-name branch so it
    can report something more useful than naming a token that does not exist.
    """
    args, why = parse_selection_tier("   ")
    assert args is None
    assert "is empty" in why, why


def test_the_parsed_executable_and_the_queried_executable_are_one_constant(
    monkeypatch: pytest.MonkeyPatch, tmp_path: Path
) -> None:
    """Command identity, pinned rather than left to inspection.

    R3-F1 was possible because the token the parser VALIDATED and the binary
    tier_selects() RESOLVED were two independent decisions. They are now the
    same constant, so this asserts the argv[0] actually executed resolves to
    TIER_COMMAND — an implementation that validated `ctest` but ran something
    else would fail here even though every rejection test above still passed.
    """
    build = configure(tmp_path, *SELF_ENTRIES)
    seen: list[list[str]] = []
    real_run = guard.subprocess.run

    def spy(argv, *args, **kwargs):  # type: ignore[no-untyped-def]
        seen.append(list(argv))
        return real_run(argv, *args, **kwargs)

    monkeypatch.setattr(guard.subprocess, "run", spy)
    tier_selects(build, f"{TIER_COMMAND} -L garmin-build-guard", "testAnything")

    assert len(seen) == 1, seen
    assert Path(seen[0][0]).name == TIER_COMMAND
    assert seen[0][0] == shutil.which(TIER_COMMAND)


def test_bare_ctest_is_an_accepted_tier_and_this_is_deliberate() -> None:
    """Zero selection flags is inside the grammar. Recording why, because it
    is the one place the allow-list is weaker than it first looks.

    Bare `ctest` selects EVERY registered test, so tier_selects() is trivially
    satisfied by it for any excused test — the reachability invariant tells a
    reader nothing in that case. It is nevertheless accepted, for two reasons:
    DEC-054 names bare `ctest` as this project's exhaustive audit tier, so it
    is a TRUE statement about where an excused test runs; and rejecting it
    would mean this guard refusing the one tier that is guaranteed to reach
    everything. The reachability rule is a floor against "excused to a tier
    that cannot reach it", not a judgement of whether the tier is well chosen
    — that is review's job, as it is for the reason text.
    """
    args, why = parse_selection_tier("ctest")
    assert args == [], why


@pytest.mark.parametrize("tier", ["ctest -L", "ctest -L garmin-fast -R"])
def test_a_flag_without_its_argument_is_rejected(tier: str) -> None:
    """A missing argument must not let the next token be consumed as a flag."""
    args, why = parse_selection_tier(tier)
    assert args is None
    assert "needs exactly one argument" in why, why


def test_a_flag_argument_may_not_itself_be_a_flag() -> None:
    """`ctest -L -S` must not smuggle a mode flag through as a value.

    Rejected rather than reasoned about: whether ctest's own parser would read
    `-S` as -L's value or as a mode flag is not a question this guard should
    be betting a capability boundary on.
    """
    args, why = parse_selection_tier("ctest -L -S")
    assert args is None
    assert "-S" in why, why


def test_the_executed_argv_is_rebuilt_from_the_parse_not_the_input(
    monkeypatch: pytest.MonkeyPatch, tmp_path: Path
) -> None:
    """The reconstruction requirement, pinned rather than assumed.

    An implementation that validates the tokens and then passes the ORIGINAL
    string through would satisfy every rejection test above while leaving the
    accepted path built from unvalidated text. So capture the real argv and
    assert it is exactly `[ctest, *parsed, -N, --show-only=json-v1]`.
    """
    build = configure(tmp_path, *SELF_ENTRIES)
    seen: list[list[str]] = []
    real_run = guard.subprocess.run

    def spy(argv, *args, **kwargs):  # type: ignore[no-untyped-def]
        seen.append(list(argv))
        return real_run(argv, *args, **kwargs)

    monkeypatch.setattr(guard.subprocess, "run", spy)
    tier_selects(build, "ctest  -L   garmin-build-guard", "testAnything")

    assert len(seen) == 1, seen
    assert seen[0][1:] == [
        "-L",
        "garmin-build-guard",
        "-N",
        "--show-only=json-v1",
    ], seen[0]
    assert Path(seen[0][0]).name == "ctest"


# --------------------------------------------------------------------------
# R2-F1, second half — no query may hang the routine gate.
# --------------------------------------------------------------------------
def test_every_ctest_query_carries_a_timeout(tmp_path: Path) -> None:
    """All THREE subprocess call sites, not just the one R2-F1 named.

    A hung ctest in the registry read or the gate-selection read hangs the
    gate just as effectively as one in the tier query. Asserted by observing
    the real calls, so a future fourth call site without a timeout is caught
    by the count as well as by the keyword.
    """
    build = configure(
        tmp_path,
        *SELF_ENTRIES,
        _entry(
            "testSlowBuildGuard",
            labels=f"garmin-build-guard;{GATE_EXCLUDE_LABEL}",
            reason=GOOD_REASON,
            tier=GOOD_TIER,
        ),
    )
    timeouts: list[object] = []
    real_run = guard.subprocess.run

    def spy(argv, *args, **kwargs):  # type: ignore[no-untyped-def]
        timeouts.append(kwargs.get("timeout"))
        return real_run(argv, *args, **kwargs)

    monkeypatch = pytest.MonkeyPatch()
    monkeypatch.setattr(guard.subprocess, "run", spy)
    try:
        assert main(["garmin_gate_coverage_guard.py", str(build)]) == 0
    finally:
        monkeypatch.undo()

    assert len(timeouts) == 3, f"expected registry + selection + tier: {timeouts}"
    assert all(t == QUERY_TIMEOUT_SECONDS for t in timeouts), timeouts


@pytest.mark.parametrize(
    "target,expect",
    [("load_registry", RegistryError), ("selected_by_gate", RegistryError)],
)
def test_a_timed_out_registry_query_is_an_error_not_a_pass(
    monkeypatch: pytest.MonkeyPatch, tmp_path: Path, target: str, expect: type
) -> None:
    """TimeoutExpired must surface as a finding, never as a crash or a pass."""
    build = configure(tmp_path, *SELF_ENTRIES)

    def boom(*args, **kwargs):  # type: ignore[no-untyped-def]
        raise subprocess.TimeoutExpired(cmd="ctest", timeout=QUERY_TIMEOUT_SECONDS)

    monkeypatch.setattr(guard.subprocess, "run", boom)
    with pytest.raises(expect, match="did not return within"):
        getattr(guard, target)(build)


def test_a_timed_out_tier_query_is_a_finding_not_a_crash(
    monkeypatch: pytest.MonkeyPatch, tmp_path: Path
) -> None:
    build = configure(tmp_path, *SELF_ENTRIES)

    def boom(*args, **kwargs):  # type: ignore[no-untyped-def]
        raise subprocess.TimeoutExpired(cmd="ctest", timeout=QUERY_TIMEOUT_SECONDS)

    monkeypatch.setattr(guard.subprocess, "run", boom)
    reachable, why = tier_selects(build, GOOD_TIER, "testAnything")
    assert not reachable
    assert f"within {QUERY_TIMEOUT_SECONDS}s" in why, why


def test_the_guard_reports_unusable_rather_than_passing_when_a_query_times_out(
    monkeypatch: pytest.MonkeyPatch, tmp_path: Path, capsys: pytest.CaptureFixture[str]
) -> None:
    """The exit code a ctest run would see must not be 0."""
    build = configure(tmp_path, *SELF_ENTRIES)

    def boom(*args, **kwargs):  # type: ignore[no-untyped-def]
        raise subprocess.TimeoutExpired(cmd="ctest", timeout=QUERY_TIMEOUT_SECONDS)

    monkeypatch.setattr(guard.subprocess, "run", boom)
    assert main(["garmin_gate_coverage_guard.py", str(build)]) == 2
    assert "PASS" not in capsys.readouterr().out


# --------------------------------------------------------------------------
# R2-F2 — result-MASKING properties, not just DISABLED.
#
# The measurements these tests encode (ctest 3.31.6, 2026-09-15):
#   SKIP_RETURN_CODE 1      -> a real `exit 1` is reported (Skipped)
#   SKIP_REGULAR_EXPRESSION -> matched output is (Skipped) whatever the exit
#   and ctest COUNTS SKIPPED AS PASSED, keeping them in the denominator:
#       `75% tests passed, 1 tests failed out of 4` with TWO masked failures
# DISABLED at least shrinks the denominator. These two inflate the pass rate,
# which makes them strictly worse, which is why documentation alone would have
# under-rated them.
# --------------------------------------------------------------------------
@pytest.mark.parametrize(
    "props,expected",
    [
        ([{"name": "SKIP_RETURN_CODE", "value": 1}], ["SKIP_RETURN_CODE"]),
        ([{"name": "SKIP_RETURN_CODE", "value": 0}], ["SKIP_RETURN_CODE"]),
        (
            [{"name": "SKIP_REGULAR_EXPRESSION", "value": ["BOOM"]}],
            ["SKIP_REGULAR_EXPRESSION"],
        ),
        (
            [{"name": "SKIP_REGULAR_EXPRESSION", "value": ["BOOM", "BANG"]}],
            ["SKIP_REGULAR_EXPRESSION"],
        ),
        (
            [
                {"name": "DISABLED", "value": True},
                {"name": "SKIP_RETURN_CODE", "value": 1},
            ],
            ["DISABLED", "SKIP_RETURN_CODE"],
        ),
    ],
)
def test_masking_properties_are_detected_in_the_shapes_ctest_really_emits(
    props: list[dict], expected: list[str]
) -> None:
    """The VALUE SHAPES are the trap, so they are pinned from measurement.

    ctest emits SKIP_RETURN_CODE as an int and SKIP_REGULAR_EXPRESSION as a
    list — not strings. And `SKIP_RETURN_CODE 0` IS emitted, as the falsy int
    `0`, while masking a passing test into a skip: an implementation using
    `if props.get(name)` waves it through, which is why presence is the
    trigger. That case is in this corpus for exactly that reason.
    """
    assert masking_properties_of({"name": "t", "properties": props}) == expected


def test_will_fail_is_not_a_general_masking_property() -> None:
    """The self-only boundary of the WILL_FAIL rule.

    On an ordinary test WILL_FAIL inverts the result and still executes the
    process, so it cannot be introduced into a green gate quietly — it turns
    that test red at once. Making it a general rule would flag a legitimate
    expected-failure semantic, so it is prohibited for the self-tests only.
    """
    test = {"name": "t", "properties": [{"name": "WILL_FAIL", "value": True}]}
    assert masking_properties_of(test) == []
    assert masking_properties_of(test, SELF_PROHIBITED_PROPERTIES) == ["WILL_FAIL"]


@pytest.mark.parametrize("prop", GATE_MASKING_PROPERTIES)
def test_an_unexcused_masked_test_is_a_gate_coverage_finding(prop: str) -> None:
    value: object = True if prop == "DISABLED" else 1
    if prop == "SKIP_REGULAR_EXPRESSION":
        value = ["."]
    tests = [{"name": "testMasked", "properties": [{"name": prop, "value": value}]}]
    findings = check_gate_coverage(tests, gated={"testMasked"})
    assert len(findings) == 1, findings
    assert f"carries {prop}" in findings[0]


@pytest.mark.parametrize("prop", GATE_MASKING_PROPERTIES)
def test_a_properly_excused_masked_test_is_not_a_finding(prop: str) -> None:
    """The rule is about the OPT-OUT PROTOCOL, not about banning properties.

    Without this, the tests above could be passing because the guard rejects
    every masked test unconditionally — a different and much more annoying
    rule than the one DEC-054 asks for.
    """
    tests = [
        {
            "name": "testSlow",
            "properties": [
                {"name": "LABELS", "value": [GATE_EXCLUDE_LABEL]},
                {"name": prop, "value": 1},
            ],
        }
    ]
    assert check_gate_coverage(tests, gated=set()) == []


def test_every_masking_property_on_one_test_is_reported_separately() -> None:
    """Three mechanisms, three findings — not one collapsed message.

    An implementation that stops at the first masking property looks identical
    to a correct one on every single-property fixture above.
    """
    tests = [
        {
            "name": "testVeryMasked",
            "properties": [
                {"name": "DISABLED", "value": True},
                {"name": "SKIP_RETURN_CODE", "value": 1},
                {"name": "SKIP_REGULAR_EXPRESSION", "value": ["."]},
            ],
        }
    ]
    findings = check_gate_coverage(tests, gated={"testVeryMasked"})
    assert len(findings) == len(GATE_MASKING_PROPERTIES), findings


@pytest.mark.parametrize(
    "kwargs,prop",
    [
        ({"skip_return_code": 1}, "SKIP_RETURN_CODE"),
        ({"skip_regex": "."}, "SKIP_REGULAR_EXPRESSION"),
    ],
)
def test_a_skip_masked_test_is_still_in_the_gate_selection(
    tmp_path: Path, kwargs: dict, prop: str
) -> None:
    """The premise of the bypass, asserted against real ctest.

    If a future ctest ever drops skip-masked tests from `-LE` selection, the
    corresponding rule becomes dead code and should be revisited rather than
    left as decoration — this is where that shows up.
    """
    build = configure(tmp_path, *SELF_ENTRIES, _entry("testMasked", **kwargs))
    assert "testMasked" in selected_by_gate(build)
    registry = {t["name"]: t for t in load_registry(build)}
    props = properties_of(registry["testMasked"])
    assert (
        prop in props
    ), f"{prop} is not visible in --show-only=json-v1; this guard cannot see it"


@pytest.mark.parametrize("kwargs", [{"skip_return_code": 1}, {"skip_regex": "BOOM"}])
def test_guard_goes_red_end_to_end_on_a_skip_masked_unexcused_test(
    tmp_path: Path, kwargs: dict
) -> None:
    """Through real cmake and real ctest, both skip mechanisms."""
    build = configure(
        tmp_path,
        *SELF_ENTRIES,
        _entry("testQuietlySkipped", **kwargs),
    )
    assert main(["garmin_gate_coverage_guard.py", str(build)]) == 1


@pytest.mark.parametrize("prop", SELF_PROHIBITED_PROPERTIES)
def test_the_self_tests_may_not_carry_a_result_masking_property(prop: str) -> None:
    """GATE-SELF: the instrument may not be neutralised or inverted.

    `SKIP_RETURN_CODE 1` on the guard itself is the sharpest case in the whole
    unit: the moment it DETECTS an exclusion defect it exits 1, ctest launders
    that into Skipped, counts it as passed, and the routine gate stays green
    while the instrument screams into a void. `WILL_FAIL` is the inverse — a
    clean run FAILS and a detected defect PASSES.
    """
    victim = SELF_TEST_NAMES[0]
    value: object = True if prop in ("DISABLED", "WILL_FAIL") else 1
    tests = [
        {
            "name": name,
            "properties": ([{"name": prop, "value": value}] if name == victim else []),
        }
        for name in SELF_TEST_NAMES
    ]
    findings = check_self_is_gated(tests, gated=set(SELF_TEST_NAMES))
    assert len(findings) == 1, findings
    assert findings[0].startswith(f"GATE-SELF {victim}")
    assert prop in findings[0]


@pytest.mark.parametrize("kwargs", [{"skip_return_code": 1}, {"will_fail": True}])
def test_guard_goes_red_end_to_end_when_a_self_test_is_masked(
    tmp_path: Path, kwargs: dict
) -> None:
    """The self-neutering case, through real cmake and real ctest."""
    entries = [_entry(SELF_TEST_NAMES[0], **kwargs)] + [
        _entry(name) for name in SELF_TEST_NAMES[1:]
    ]
    build = configure(tmp_path, *entries)
    assert main(["garmin_gate_coverage_guard.py", str(build)]) == 1
