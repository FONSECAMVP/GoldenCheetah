"""DES-012 — stable adapter over python-garminconnect.

The *only* module in the Garmin Connect feature that imports `garminconnect`
directly. The C++ worker (DES-001) calls this adapter; nothing else in the
codebase imports the underlying library. Swapping `python-garminconnect` for
a community fork is a one-file change here.

Phase 2.2 status (REQ-by-REQ):
  - REQ-002 login / GarminError(kind='auth') translation: GREEN.
  - REQ-007 download_activity (fmt map + connection/rate_limit translation): GREEN.
  - REQ-014 login connection/rate_limit translation: GREEN.
  - REQ-010 (DES-005): login/submit_mfa/list_activities_since/download_activity
    are all paced + retried via gc_rate.py.
  - REQ-013 (DEC-050 first slice — dob/weight_kg/height_cm only; hr_max/ftp_w
    deferred) get_profile: GREEN.
  - REQ-003 / REQ-008 / REQ-012: still raise NotImplementedError until their
    owning slice reaches GREEN.

The wider `_EXCEPTION_MAP` (captcha, mfa_required, token_permissions) lands
with later slices (A2-005, REQ-003, REQ-015) — not this module's scope yet.
"""

from __future__ import annotations

from collections.abc import Iterator
from datetime import date
from typing import Any

from gc_rate import rate_limited, with_retry

try:
    import os
    import sys

    if hasattr(os, "RTLD_DEEPBIND"):
        # B-STAGE9-06: GoldenCheetah's own linked libcurl-gnutls.so.4 (loaded
        # globally in-process for its own, unrelated networking) interposes
        # curl_cffi's statically-bundled, patched libcurl-impersonate symbols
        # via normal ELF global symbol resolution -- curl_cffi's wrapper has
        # no -Bsymbolic protection. This forces curl_cffi's own extension to
        # bind to itself instead of the host process's already-loaded system
        # libcurl. RTLD_DEEPBIND is a glibc/Linux-only extension (not on
        # macOS/Windows) -- the interposition this guards against is itself
        # an ELF/glibc-specific loader behavior, so skipping it elsewhere is
        # not a coverage gap on those platforms.
        _old_dlopen_flags = sys.getdlopenflags()
        try:
            sys.setdlopenflags(_old_dlopen_flags | os.RTLD_DEEPBIND)
            import curl_cffi._wrapper  # noqa: F401 — import-for-side-effect
        finally:
            sys.setdlopenflags(_old_dlopen_flags)

    import garminconnect as _gc
except ImportError:
    # Library not present in dev/test environments without the runtime dep
    # (python-garminconnect + curl_cffi). Tests monkeypatch `_gc` to a fake;
    # production builds bundle the wheel under DEC-011 / REQ-NF-Pkg-001.
    _gc = None


class GarminError(Exception):
    """GC-stable error type emitted by the adapter.

    Callers (worker → wizard) switch on .kind, never on the underlying
    library's exception classes. See DES-008 error-translation table.

    kind ∈ {'auth', 'rate_limit', 'connection', 'captcha',
            'mfa_required', 'token_permissions', 'unknown'}
    """

    def __init__(self, kind: str, message: str, original: Exception | None = None) -> None:
        self.kind = kind
        self.message = message
        self.original = original
        super().__init__(message)


class GarminClient:
    """Adapter — see DES-012 for surface and translation responsibilities."""

    def __init__(self, email: str, password: str) -> None:
        if _gc is None:
            raise GarminError(
                "unknown",
                "python-garminconnect is not installed; cannot construct GarminClient",
            )
        # Forward `password` straight into the library constructor and drop
        # the local reference; REQ-005 forbids retaining it on this adapter.
        # The library's own retention is its contract — DES-012 isolates it.
        #
        # DEC-014 Option B (A3-R004-M3 security-close): construct the library
        # AUTH-ONLY — exactly (email, password), NO tokenstore path. Passing a
        # path here would let the library self-write a SECOND, unaudited token
        # file whose permissions are never enforced/checked. Under Option B the
        # session lives in memory; C++ (GarminTokenStore) owns the single atomic
        # 0600 write of the blob exported by dump_tokens(). Nothing below this
        # adapter writes a token file.
        #
        # NOTE(DEC-014 OQ1): the AUTH-ONLY 2-arg construction is a real-lib
        # signature item, unconfirmed against the not-yet-bundled
        # python-garminconnect wheel (like dumps()/loads() below). It is pinned
        # by the fakes/pystub until the wheel is bundled (DES-007/Pkg).
        self._garmin = _gc.Garmin(email, password)
        # REQ-003 — retained pending-MFA state (the library's client_state) set
        # by login() when Garmin requires an OTP; consumed by submit_mfa(). None
        # until/unless the MFA path is entered. Holds no password (REQ-005).
        self._pending_mfa: Any = None

    def _login_impl(self) -> dict[str, Any]:
        # REQ-014 — connection/rate_limit classified by exception TYPE (LSN-006),
        # mirroring download_activity()/list_activities_since()'s existing pattern.
        try:
            result = self._garmin.login()
        except _gc.exceptions.GarminConnectAuthenticationError as e:
            raise GarminError("auth", str(e) or "Authentication failed", e) from e
        except _gc.exceptions.GarminConnectConnectionError as e:
            raise GarminError("connection", str(e) or "Could not reach Garmin Connect", e) from e
        except _gc.exceptions.GarminConnectTooManyRequestsError as e:
            raise GarminError("rate_limit", str(e) or "Garmin Connect is rate-limiting sign-in", e) from e
        # REQ-003 — MFA-required signal. python-garminconnect/garth's login()
        # returns a ("needs_mfa", client_state) sentinel (instead of raising)
        # when the account needs a 6-digit OTP. Detect it by SHAPE, never by
        # message content (LSN-006); retain the client_state so submit_mfa() can
        # resume the SAME session, and surface a stable {"mfa_required": True}
        # sentinel to the worker adapter instead of a success identity. The
        # no-MFA return below is unchanged byte-for-byte (library returns None
        # / a non-sentinel on plain success -> falls through).
        #
        # NOTE(DEC-014 OQ1): the exact needs-MFA sentinel and resume_login()
        # signature are unconfirmed against the not-yet-bundled
        # python-garminconnect wheel (like dumps()/loads() above); pinned here by
        # the fake/pystub until the wheel is bundled (DES-007/Pkg).
        if isinstance(result, tuple) and len(result) == 2 and result[0] == "needs_mfa":
            self._pending_mfa = result[1]
            return {"mfa_required": True}
        return {
            "garmin_user_id": str(self._garmin.full_name_id),
            "display_name": self._garmin.display_name,
        }

    # DES-005/DEC-007 — paced (REQ-NF-Perf-002) and retried (REQ-NF-Reliab-001)
    # through gc_rate.py, same as list_activities_since/download_activity below.
    # is_transient excludes kind='auth' (and the MFA-only kind='unknown') so a
    # bad password is never auto-retried — REQ-002/REQ-003's locked
    # immediate-failure behaviour is unchanged.
    login = with_retry(
        rate_limited(_login_impl),
        is_transient=lambda e: isinstance(e, GarminError) and e.kind in ("connection", "rate_limit"),
    )

    def dump_tokens(self) -> str:
        # REQ-004 / DEC-014 Option B: export the authenticated in-memory OAuth
        # session as an opaque, serializable blob. The adapter does NOT write it
        # to disk — C++ owns the single atomic 0600 write (DES-002/DES-006).
        # REQ-005: only OAuth bearer + refresh tokens live in this blob; the
        # password was handed to the library and never retained here.
        #
        # NOTE(DEC-014 OQ1): the exact library export method is unconfirmed
        # against the not-yet-bundled python-garminconnect wheel (no version pin
        # in repo; lib absent from .venv). On the current native engine this is
        # `dumps()`; confirm when the wheel is bundled (DES-007/Pkg). The adapter
        # contract (return a non-empty str) is pinned by pytest against a fake.
        return str(self._garmin.dumps())

    @classmethod
    def from_tokens(cls, token_str: str) -> GarminClient:
        # REQ-007 closure (Slice 1) / REQ-005 / REQ-NF-Compat-001(b): restore an
        # authenticated session from a stored OAuth blob WITHOUT a password. A
        # fresh CloudService open() has no password (REQ-005 forbids persisting
        # it), so silent reauth MUST come from the stored TOKENS only. The library
        # is constructed password-free and the blob loaded; a tampered/expired
        # blob surfaces from load_tokens() as GarminError(kind='session_expired').
        #
        # NOTE(DEC-014 OQ1): the password-free `_gc.Garmin()` construction is a
        # real-lib signature item, unconfirmed against the not-yet-bundled
        # python-garminconnect wheel (like dumps()/loads()). Pinned by the
        # pystub/fake until the wheel is bundled (DES-007/Pkg).
        if _gc is None:
            raise GarminError(
                "unknown",
                "python-garminconnect is not installed; cannot restore GarminClient",
            )
        self = cls.__new__(cls)
        self._garmin = _gc.Garmin()
        self.load_tokens(token_str)
        return self

    def load_tokens(self, token_str: str) -> None:
        # REQ-004 counterpart of dump_tokens(): restore an authenticated session
        # from a previously-exported blob (the REQ-006 resume path feeds this).
        #
        # NOTE(DEC-014 OQ1): real library import method unconfirmed (see above);
        # `loads()` on the native engine.
        #
        # DEC-014 OQ2 → REQ-NF-Compat-001(b): a tampered or server-side-
        # invalidated session surfaces from the library as an authentication
        # error; translate it to GC-stable kind='session_expired' — DISTINCT
        # from login's 'auth' and from 'token_permissions' — so the resume path
        # can route it to a re-login prompt. Classify by exception TYPE, never
        # by message content (LSN-006).
        try:
            self._garmin.loads(token_str)
        except _gc.exceptions.GarminConnectAuthenticationError as e:
            raise GarminError(
                "session_expired",
                str(e) or "Stored Garmin session is expired; please sign in again",
                e,
            ) from e

    def _submit_mfa_impl(self, code: str) -> dict[str, Any]:
        # REQ-003 — resume the pending-MFA session established by a prior login()
        # that returned {"mfa_required": True}. Two-step flow: login() retained a
        # client_state; resume_login(code, client_state) completes auth on the
        # SAME session and populates the identity the same way login() does.
        #
        # NOTE(DEC-014 OQ1): the resume_login() call signature is unconfirmed
        # against the not-yet-bundled python-garminconnect wheel (like
        # dumps()/loads()); pinned here by the fake/pystub until the wheel is
        # bundled (DES-007/Pkg).
        pending = getattr(self, "_pending_mfa", None)
        if pending is None:
            # Called out of order (no prior login() MFA outcome). This is a
            # programming/contract error, NOT an authentication failure —
            # kind='unknown' so DES-008 never routes "wrong code" copy for it.
            raise GarminError(
                "unknown",
                "submit_mfa() called with no pending MFA session; call login() first",
            )
        try:
            self._garmin.resume_login(code, pending)
        except _gc.exceptions.GarminConnectAuthenticationError as e:
            # Bad/expired OTP. Classify by exception TYPE (LSN-006) as
            # kind='auth' so the Slice-B page can re-prompt (up to 3 attempts —
            # REQ-003). The pending state is deliberately RETAINED so a retry
            # resumes the SAME session.
            raise GarminError("auth", str(e) or "Invalid MFA code", e) from e
        # Success — clear the pending state and return the SAME identity dict
        # shape login() returns on a no-MFA success (byte-for-byte parity so the
        # worker's Success mapping is identical for both paths).
        self._pending_mfa = None
        return {
            "garmin_user_id": str(self._garmin.full_name_id),
            "display_name": self._garmin.display_name,
        }

    # DES-005/DEC-007 — same pacing/retry wrapping as login. is_transient
    # excludes kind='auth' (bad/expired OTP) and kind='unknown' (no pending
    # session) so neither is auto-retried — REQ-003's re-prompt-on-bad-code
    # flow (not a silent retry loop) is unchanged.
    submit_mfa = with_retry(
        rate_limited(_submit_mfa_impl),
        is_transient=lambda e: isinstance(e, GarminError) and e.kind in ("connection", "rate_limit"),
    )

    def _list_activities_since_impl(self, ts_gmt: str) -> Iterator[dict[str, Any]]:
        # REQ-008 Slice A (DES-010 step 4 / DES-012). List the activities whose
        # Garmin server-side startTimeGMT is newer than ts_gmt. `ts_gmt` is
        # Garmin's SERVER-SIDE timestamp, NOT the local clock (DES-010 — protects
        # against clock-skew duplicates), and it is forwarded to the library
        # VERBATIM (no reformatting, no local-clock substitution). Dedup and
        # download are later slices (B/C) — this is a thin list+translate.
        #
        # Error translation mirrors download_activity(): classify by exception
        # TYPE, then re-raise as a GC-stable GarminError kind (LSN-006). Only the
        # known-transient listing errors become kinds; anything else (a foreign
        # exception, or a malformed activity record → KeyError) propagates
        # unchanged rather than inheriting a Garmin kind. Translation is EAGER
        # (the summaries are built here, not lazily inside a generator) so a
        # caller sees connection/rate_limit at call time exactly like the sibling
        # methods; the return is still a true Iterator (DES-012 signature).
        #
        # NOTE(DEC-014 OQ1): the exact python-garminconnect listing signature
        # (get_activities_by_date vs get_activities, and its date/paging bounds)
        # is unconfirmed against the not-yet-bundled wheel (like dumps()/loads()
        # elsewhere in this module); the since-timestamp is forwarded verbatim and
        # the surface is pinned by the fakes/pystub until the wheel is bundled
        # (DES-007/Pkg). Swapping it is a one-line change here (DES-012 is the
        # single point of underlying-library knowledge).
        try:
            raw = self._garmin.get_activities_by_date(ts_gmt)
        except _gc.exceptions.GarminConnectConnectionError as e:
            raise GarminError("connection", str(e) or "Could not reach Garmin Connect", e) from e
        except _gc.exceptions.GarminConnectTooManyRequestsError as e:
            raise GarminError("rate_limit", str(e) or "Garmin Connect is rate-limiting listing", e) from e
        # Normalize each library record to the GC-stable summary shape carrying at
        # least activityId + startTimeGMT (as strings, matching what the C++ seam
        # marshals). Extra library keys are dropped; a record missing either key
        # is a library contract breach whose KeyError propagates unclassified.
        summaries = [{"activityId": str(a["activityId"]), "startTimeGMT": str(a["startTimeGMT"])} for a in raw]
        return iter(summaries)

    # DES-005/DEC-007 — paced (REQ-NF-Perf-002) and retried (REQ-NF-Reliab-001)
    # through gc_rate.py. Retry keys on the GC-stable kind (translation already
    # happened above) rather than the raw library exception type design.md's
    # `_TRANSIENT` sketch names — see gc_rate.py's module docstring for why.
    list_activities_since = with_retry(
        rate_limited(_list_activities_since_impl),
        is_transient=lambda e: isinstance(e, GarminError) and e.kind in ("connection", "rate_limit"),
    )

    def _download_activity_impl(self, activity_id: str, fmt: str = "ORIGINAL") -> bytes:
        # REQ-007 / DEC-006: FIT is the default (dl_fmt=ORIGINAL); TCX is the
        # fallback the C++ readFile requests when Garmin has no FIT original
        # (DES-004 owns the fallback orchestration — this adapter is a thin,
        # single-format fetch). Map the GC-stable `fmt` to the library's enum
        # here so nothing above this seam knows the library's format type.
        fmt_map = {
            "ORIGINAL": self._garmin.ActivityDownloadFormat.ORIGINAL,
            "TCX": self._garmin.ActivityDownloadFormat.TCX,
        }
        if fmt not in fmt_map:
            # Reject before any network call — never forward a garbage dl_fmt.
            raise GarminError("unknown", f"unsupported download format {fmt!r}")
        try:
            # Classify by exception TYPE, then translate (LSN-006): only the
            # known-transient download errors become GC-stable kinds; anything
            # else propagates unchanged rather than inheriting a Garmin kind.
            data: bytes = self._garmin.download_activity(activity_id, dl_fmt=fmt_map[fmt])
        except _gc.exceptions.GarminConnectConnectionError as e:
            raise GarminError("connection", str(e) or "Could not reach Garmin Connect", e) from e
        except _gc.exceptions.GarminConnectTooManyRequestsError as e:
            raise GarminError("rate_limit", str(e) or "Garmin Connect is rate-limiting downloads", e) from e
        return data

    # DES-005/DEC-007 — same pacing/retry wrapping as list_activities_since.
    download_activity = with_retry(
        rate_limited(_download_activity_impl),
        is_transient=lambda e: isinstance(e, GarminError) and e.kind in ("connection", "rate_limit"),
    )

    def _get_profile_impl(self) -> dict[str, Any]:
        # REQ-013 (DEC-050 first slice) — dob/weight_kg/height_cm only; hr_max
        # and ftp_w are DEFERRED (they live in GC's date-ranged Zones/CP system,
        # not a simple scalar — see DES-011's "Scope" paragraph). The real
        # library's profile/settings response is UNTYPED (no typed.py model)
        # and UNVERIFIED against a live account (this project has never tested
        # against one — traceability.md's "LIVE-SERVICE TESTED: NO" row). Every
        # field below is therefore extracted defensively: a short list of
        # plausible key-name candidates is tried per field, and a field is
        # silently OMITTED (never raises) on a missing key, wrong type, or an
        # implausible value. REQ-013's own "only fill missing fields" contract
        # already treats "Garmin didn't have this" as a normal outcome, not an
        # error, so this degrades safely either way.
        try:
            raw = self._garmin.get_userprofile_settings()
        except _gc.exceptions.GarminConnectConnectionError as e:
            raise GarminError("connection", str(e) or "Could not reach Garmin Connect", e) from e
        except _gc.exceptions.GarminConnectTooManyRequestsError as e:
            raise GarminError("rate_limit", str(e) or "Garmin Connect is rate-limiting profile fetch", e) from e

        if not isinstance(raw, dict):
            return {}

        profile: dict[str, Any] = {}

        # DOB — candidate keys unconfirmed (DEC-050); accept only a value whose
        # first 10 chars parse as an ISO YYYY-MM-DD date.
        for key in ("birthDate", "dateOfBirth"):
            value = raw.get(key)
            if not isinstance(value, str):
                continue
            try:
                date.fromisoformat(value[:10])
            except ValueError:
                continue
            profile["dob"] = value[:10]
            break

        # Weight — Garmin has been observed to report weight in grams rather
        # than kilograms on some endpoints; anything implausibly large for a
        # kilogram value is treated as grams and converted. A result outside a
        # sane human range is dropped rather than filled with nonsense.
        for key in ("weight", "weightInKilograms", "userWeight"):
            value = raw.get(key)
            if not isinstance(value, (int, float)) or isinstance(value, bool):
                continue
            weight_kg = float(value)
            if weight_kg > 300:
                weight_kg /= 1000.0
            if 20.0 <= weight_kg <= 300.0:
                profile["weight_kg"] = weight_kg
                break

        # Height — candidate keys unconfirmed (DEC-050); sanity-bounded to a
        # plausible human range in centimeters.
        for key in ("height", "heightInCentimeters", "userHeight"):
            value = raw.get(key)
            if not isinstance(value, (int, float)) or isinstance(value, bool):
                continue
            height_cm = float(value)
            if 50.0 <= height_cm <= 250.0:
                profile["height_cm"] = height_cm
                break

        return profile

    # DES-005/DEC-007 — same pacing/retry wrapping as the other read calls.
    get_profile = with_retry(
        rate_limited(_get_profile_impl),
        is_transient=lambda e: isinstance(e, GarminError) and e.kind in ("connection", "rate_limit"),
    )

    def disconnect(self) -> None:
        raise NotImplementedError("REQ-012 GREEN step not yet implemented")
