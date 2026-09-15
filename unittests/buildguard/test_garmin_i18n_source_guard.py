#!/usr/bin/env python3
"""B-STAGE9-15 / DEC-053 — unit tests for the i18n guard's technical-literal
exemption predicate.

Why this file exists. `testGarminI18nSourceGuard` only runs the guard over the
real source tree: it proves "0 findings today", never "the predicate still
rejects things". That is the difference between a guard and a guard that has
gone vacuous, and DEC-053 widens exactly the predicate where a vacuous outcome
would be invisible — `is_technical()` returning True for everything makes the
tree-level test pass louder, not fail.

So the REJECTION cases below are the load-bearing half of this file. They are
written to fail if the developer-trace exemption is ever widened far enough to
swallow prose, which is this project's signature failure mode (B-STAGE9-13,
B-STAGE9-14, and the misfiring heuristic that produced B-STAGE9-15 itself).

Three kinds of case live here, and they are not interchangeable:
  ACCEPTANCE       — traces and date patterns that must stay exempt.
  REJECTION        — prose that must still be caught. The load-bearing half.
  PINNED LIMITATION — an accepted false negative, asserted so that moving the
                     boundary shows up in a diff. Read
                     test_all_key_value_prose_is_a_known_limitation before
                     treating it as a statement about what SHOULD happen.
"""

from __future__ import annotations

import sys
import unicodedata
from pathlib import Path

import pytest

sys.path.insert(0, str(Path(__file__).resolve().parent))

from garmin_i18n_source_guard import (  # noqa: E402
    is_developer_trace,
    is_technical,
    is_user_facing,
    main,
)

REPO_ROOT = Path(__file__).resolve().parents[2]


# --------------------------------------------------------------------------
# ACCEPTANCE — developer-trace literals are exempt as a CATEGORY (DEC-053).
# --------------------------------------------------------------------------
# Every entry is a real or realistic qDebug() trace line: key=value tokens,
# optionally preceded by ONE bare snake_case event-name token.
TRACE_LITERALS = [
    # The three flagged call sites (GarminCredentialsPage.cpp:145,
    # GarminMfaPage.cpp:147,161) — the literal this unit exists to exempt.
    "garmin_auth_unknown exception_type=%1",
    # REQ-NF-Obs-001's gcObsTrace() mirror — already exempt before DEC-053,
    # but via placeholder arithmetic rather than by category.
    "gc_obs op=%1 outcome=%2 error_code=%3 duration_ms=%4",
    "gc_obs op=%1 outcome=%2 error_code=%3 duration_ms=%4 activity_count=%5",
    # Bare key=value runs with no leading event name.
    "op=auth outcome=fail",
    "error_code=empty duration_ms=0",
    # A single key=value token carrying an empty value (the success case
    # emits error_code= with nothing after it).
    "outcome=ok error_code= duration_ms=%1",
]

# Date/time format patterns — the guard's other pre-existing exemption, kept.
DATE_LITERALS = [
    "yyyy-MM-dd HH:mm:ss",
    "yyyy-MM-dd",
]


# --------------------------------------------------------------------------
# REJECTION — prose that must STILL be caught after the widening.
# --------------------------------------------------------------------------
# These are the cases DEC-053 names explicitly, plus the near-miss shapes a
# too-greedy predicate would swallow. If the exemption is ever widened until
# one of these is exempt, the guard has gone vacuous and these tests fail.
PROSE_LITERALS = [
    # (1) genuine prose that merely CONTAINS an '=' sign
    "Sorry, the timeout = 30 seconds was exceeded",
    "Set the value = 5 before continuing",
    # (2) genuine prose with a leading snake_case-looking word
    "garmin_auth_unknown please sign in again",
    "gc_obs could not be written to disk",
    # (3) ordinary multi-word prose, no '=' at all
    "Could not restore the stored session",
    "Garmin Connect: the stored session file is empty; please sign in again.",
    # (4) near-miss: starts like a trace line, then drifts into prose. A
    # predicate that only checks the FIRST token, or that accepts a literal
    # merely because SOME token is key=value, lets this through.
    "timeout=30 seconds exceeded",
    "op=auth failed, please try again",
    # (5) near-miss: two bare words before the key=value run. Only ONE
    # leading event-name token is allowed.
    "garmin auth unknown exception_type=%1",
    # (5b) CLASS A — ONE ordinary word followed by a clean key=value run.
    # Capitalisation is NOT the discriminator (an earlier revision of this
    # file claimed it was, and the lower-case half of the class walked
    # straight through). The discriminator is that an event name is true
    # snake_case: it carries an underscore. Ordinary English words do not.
    # Both cases of every pair below must be rejected.
    "Error code=5 retry=now",
    "error code=5 retry=now",
    "Warning disk=full",
    "warning disk=full",
    "status code=404",
    "retry attempts=3",
    "sync progress=50",
    "note value=1 other=2",
    # (5c) CLASS B — a pure key=value run, no leading word at all, whose
    # VALUES are capitalised words. Nothing about the token shape betrays
    # this as prose, so the value side has to be constrained: every value a
    # real trace emits is a %N placeholder, empty, or lower-case.
    "status=Connected action=Retry",
    "state=Disconnected reason=Timeout",
    # (5d) CLASS B under a legitimate event name — the event name is real,
    # but the values are capitalised prose. The leading-token rule cannot
    # catch this one; only the value rule can.
    "gc_obs status=Connected action=Retry",
    # (6) near-miss: prose with TWO '%' and TWO '=' — the exact shape the OLD
    # `count('%') >= 2 and count('=') >= 2` clause exempted by accident. This
    # one is prose and was ALREADY wrongly exempt before DEC-053, so it is
    # red both before and after: the widening must also be a tightening here.
    # ("retry now" is the adjacent letter-token pair that makes the PROSE
    # heuristic fire; without one, a literal is not classified as prose at
    # all and never reaches the technical exemption.)
    "%1 of %2 files = incomplete sync = retry now",
]


# --------------------------------------------------------------------------
# PINNED LIMITATION — an ACCEPTED false negative, asserted so it cannot move
# in silence. Read the test's docstring before adding to this list.
# --------------------------------------------------------------------------
# These are NOT trace lines, and they are NOT strings anyone should leave
# untranslated. They are hypothetical user-facing strings that happen to be
# written entirely as key=value tokens, which the shape-based exemption
# therefore lets through.
KNOWN_LIMITATION_LITERALS = [
    "status=offline action=retry",
    "error=connection-lost action=retry",
]


# --------------------------------------------------------------------------
# UNICODE — the value rule is a case comparison, not an ASCII character class.
# --------------------------------------------------------------------------
# The rule it replaced, `[^A-Z\s]*`, was ASCII-only, so the SAME VISIBLE
# STRING classified differently depending on the Unicode normalisation form
# the source file happened to be saved in: a precomposed 'É' (U+00C9) is not
# in [A-Z] and was exempted, while its decomposed form ('E' + U+0301) was
# caught by the ASCII 'E' it starts with. Both forms must be flagged now, and
# lower-case non-ASCII values must stay exempt. NFC/NFD pairs are spelled with
# explicit escapes so the distinction survives an editor's re-normalisation.
UNICODE_CAPITAL_VALUES = [
    "status=\u00c9teint action=arr\u00eat",  # precomposed capital E-acute U+00C9
    "status=E\u0301teint action=arret",  # decomposed: ASCII 'E' + U+0301
    "status=\u00dcbertragung code=%1",  # precomposed capital U-umlaut U+00DC
    "status=\u1e9e action=retry",  # capital sharp s U+1E9E
    "op=\u0394elta outcome=fail",  # Greek capital delta U+0394
    "op=\u041doscow outcome=fail",  # Cyrillic capital En U+041D
]

UNICODE_LOWER_VALUES = [
    "status=\u00e9teint action=arr\u00eat",  # precomposed small e-acute U+00E9
    "status=e\u0301teint action=arret",  # decomposed: ASCII 'e' + U+0301
    "status=\u00dfeta action=retry",  # small sharp s U+00DF, see the casefold note
    "op=\u30c6\u30b9\u30c8 outcome=fail",  # caseless script (katakana)
]


@pytest.mark.parametrize("literal", TRACE_LITERALS)
def test_developer_trace_literals_are_technical(literal: str) -> None:
    """DEC-053: key=value trace lines are exempt as a category."""
    assert (
        is_technical(literal) is True
    ), f"developer-trace literal must be exempt: {literal!r}"
    assert (
        is_user_facing(literal) is False
    ), f"developer-trace literal must not be flagged as user-facing: {literal!r}"


@pytest.mark.parametrize("literal", DATE_LITERALS)
def test_date_format_literals_stay_technical(literal: str) -> None:
    """The pre-existing date/time exemption must survive the widening."""
    assert is_technical(literal) is True, f"date pattern must stay exempt: {literal!r}"


@pytest.mark.parametrize("literal", PROSE_LITERALS)
def test_prose_is_still_rejected(literal: str) -> None:
    """The load-bearing half: widening must not swallow real prose."""
    assert (
        is_technical(literal) is False
    ), f"prose must NOT be exempted as technical: {literal!r}"
    assert (
        is_user_facing(literal) is True
    ), f"prose must still be flagged as user-facing: {literal!r}"


@pytest.mark.parametrize("literal", KNOWN_LIMITATION_LITERALS)
def test_all_key_value_prose_is_a_known_limitation(literal: str) -> None:
    """Pins an ACCEPTED FALSE NEGATIVE. This is NOT an endorsement.

    What it asserts: a literal written entirely as key=value tokens is exempt
    from the i18n guard even when it reads as something a user could be shown.
    If you are writing a string like this FOR A USER, wrap it in tr() — the
    guard will not remind you, and that is the whole point of this test.

    Why it is accepted rather than fixed. DEC-053 exempts developer traces as
    a CATEGORY recognised by SHAPE. No shape predicate can separate
    "status=offline action=retry" from the genuine trace "op=auth
    outcome=fail": they are the same shape, token for token. Any rule that
    flags the first flags the second, and the second is exactly what DEC-053
    exists to exempt. So this false negative is not a gap in the
    implementation, it is the irreducible cost of the decision — raised as
    BLOCKING in review, adjudicated as accepted-by-design (B-STAGE9-15,
    round 2). The residual risk is bounded by convention, not by the rule:
    every genuine user-facing literal in the scanned scope is ordinary prose
    ("Garmin Connect: …", "Too many incorrect codes…", "Paused: …"), none is
    remotely all-key=value.

    Why it is pinned. An accepted limitation that is only written down in a
    comment moves without anyone noticing. Asserted here, the boundary cannot
    shift in EITHER direction in silence:

      * narrow the exemption so these literals are flagged, and this test goes
        RED — that is the pin working, not a bug. Re-read DEC-053, confirm the
        real traces are still exempt, then update this list with the decision
        recorded alongside it.
      * widen it, and test_predicate_is_not_vacuous plus the PROSE_LITERALS
        corpus go red instead.
    """
    assert is_technical(literal) is True, (
        f"KNOWN LIMITATION MOVED: {literal!r} is no longer exempt. If that was "
        f"deliberate, check the real traces in TRACE_LITERALS are still exempt "
        f"and update KNOWN_LIMITATION_LITERALS with the reason."
    )
    assert is_user_facing(literal) is False, (
        f"KNOWN LIMITATION MOVED: {literal!r} is now flagged as user-facing. "
        f"See this test's docstring before changing it back."
    )


@pytest.mark.parametrize("literal", UNICODE_CAPITAL_VALUES)
def test_capitalised_values_are_rejected_in_every_script(literal: str) -> None:
    """The value rule must not be ASCII-only.

    `[^A-Z\\s]*` exempted a precomposed 'É' (U+00C9) while flagging the same
    character decomposed ('E' + U+0301), so a literal's classification
    depended on the normalisation form of the file it was saved in. A capital
    in ANY script now disqualifies a value.

    Only `is_technical` is asserted: whether such a literal becomes a FINDING
    also depends on the ASCII-only PROSE heuristic firing, which for
    "status=\\u1e9e action=retry" it does not. That is a separate,
    pre-existing limitation of PROSE, not of this rule.
    """
    assert (
        is_technical(literal) is False
    ), f"a capitalised value must disqualify the trace exemption: {literal!r}"


@pytest.mark.parametrize("literal", UNICODE_LOWER_VALUES)
def test_lower_case_values_stay_exempt_in_every_script(literal: str) -> None:
    """The regression direction that matters as much as the fix.

    Tightening the value rule must not start flagging real traces. Note
    "status=\\u00dfeta action=retry": the rule is `value == value.lower()` and
    deliberately NOT `value == value.casefold()`, because casefold maps 'ß' to
    'ss', which would make a legitimately lower-case German value compare
    unequal and be flagged.
    """
    assert (
        is_technical(literal) is True
    ), f"a value with no capital must stay exempt: {literal!r}"


def test_classification_is_normalisation_independent() -> None:
    """The property the ASCII character class violated.

    Stated as a property over every corpus in this file rather than as another
    hand-picked pair: NFC and NFD of the same literal must classify the same,
    whatever that classification is. This is what actually failed before —
    not "É was exempt", but "É was exempt and É was not".
    """
    corpus = (
        TRACE_LITERALS
        + DATE_LITERALS
        + PROSE_LITERALS
        + KNOWN_LIMITATION_LITERALS
        + UNICODE_CAPITAL_VALUES
        + UNICODE_LOWER_VALUES
    )
    divergent = [
        literal
        for literal in corpus
        if is_technical(unicodedata.normalize("NFC", literal))
        is not is_technical(unicodedata.normalize("NFD", literal))
    ]
    assert divergent == [], (
        f"{len(divergent)}/{len(corpus)} literals classify differently under "
        f"NFC vs NFD — the value rule has gone ASCII-only again: {divergent}"
    )


def test_at_least_one_key_value_token_is_required() -> None:
    """DEC-053's third condition, which no prose case above exercises.

    A bare event name with nothing after it is not a trace literal. These do
    not reach the guard in practice (a single-token literal never matches the
    PROSE heuristic), so the predicate is asserted directly.
    """
    assert is_developer_trace("gc_obs") is False
    assert is_developer_trace("garmin_auth_unknown") is False
    assert is_developer_trace("") is False
    assert is_developer_trace("   ") is False


def test_key_must_be_identifier_shaped() -> None:
    """A token that merely CONTAINS '=' is not a key=value token.

    The pre-DEC-053 clause was `all('=' in token for token in tokens)`, which
    accepts '=value', '%1=%2' and '3x=y'. Reverting to that looseness is the
    most likely way this predicate would be "simplified" back into vagueness,
    and no natural prose case covers it, so it is pinned structurally.
    """
    assert is_developer_trace("=value =other") is False
    assert is_developer_trace("%1=%2 %3=%4") is False
    assert is_developer_trace("3x=y 4z=w") is False
    # ...while the legitimate identifier-keyed forms still pass.
    assert is_developer_trace("op=auth outcome=fail") is True


def test_event_name_convention_is_lower_case_snake_case() -> None:
    """Pins a CONVENTION, and says so rather than overclaiming.

    Unlike the cases above, this one is not defending against prose: no
    natural user-facing string starts with a capitalised underscored token
    ("Error_code value=5" is not English). Relaxing the event name to allow
    capitals therefore admits no prose that the author could construct, and
    is close to an equivalent mutation.

    It is pinned anyway because event names in this codebase are lower-case
    snake_case by convention (`gc_obs`, `garmin_auth_unknown`), the rule is
    free to keep, and an unpinned constraint drifts. Read this as "the
    convention is asserted", NOT as "prose safety depends on it".
    """
    assert is_developer_trace("gc_obs op=%1") is True
    assert is_developer_trace("Gc_obs op=%1") is False
    assert is_developer_trace("GC_OBS op=%1") is False


def test_predicate_is_not_vacuous() -> None:
    """A blunt anti-vacuity assertion.

    If someone widens `is_technical` to `return True`, every other test in
    this file that checks a single literal could in principle be argued away
    one case at a time. This one cannot: it asserts that EVERY literal in the
    prose corpus is rejected — not most of them — so any predicate that
    exempts even one prose case fails here with the offenders listed.
    """
    exempted = [lit for lit in PROSE_LITERALS if is_technical(lit)]
    assert exempted == [], (
        f"{len(exempted)}/{len(PROSE_LITERALS)} prose literals were exempted — "
        f"the technical predicate has gone vacuous: {exempted}"
    )


def test_guard_passes_over_the_real_tree() -> None:
    """End-to-end: the real guard over the real repo reports 0 findings.

    This is the assertion that was RED in HEAD since c1948b513. It is kept
    deliberately alongside the rejection cases above — on its own it can be
    satisfied by a vacuous predicate, which is precisely why it is not on
    its own.
    """
    assert main(["garmin_i18n_source_guard.py", str(REPO_ROOT)]) == 0
