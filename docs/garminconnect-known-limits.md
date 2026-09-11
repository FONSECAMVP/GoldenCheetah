# Garmin Connect — Known Limitations

GoldenCheetah's Garmin Connect integration (Phase 1) has three known
limitations users and contributors should be aware of.

## One account at a time

Only one Garmin account can be connected at a time per GoldenCheetah athlete.
Switching accounts is supported — Disconnect, then reconnect with different
credentials — and each account's sidecar dedup/state files are preserved on
disk rather than deleted, so switching back to a previously-connected account
does not lose its download history.

## Session expiry forces a full re-login

If Garmin invalidates GoldenCheetah's stored session on their side (for
example, after a password change on the Garmin account), GoldenCheetah will
not attempt a silent reauthentication. Instead it prompts a full re-login.
This is deliberate: silently retrying with a stale session is how a
transient error gets mistaken for a persistent one, and forcing a fresh
sign-in is the more honest failure mode. Internally this is surfaced as a
distinct error kind (`session_expired`) from the Python adapter, kept
separate from an outright credential rejection so the two cases can
eventually be messaged differently.

## Third-party library tracks Garmin's SSO flow

Phase 1 depends on `python-garminconnect`, an unofficial third-party library,
to interoperate with Garmin's single sign-on flow. GoldenCheetah does not
implement that flow itself. When Garmin changes their SSO flow, Garmin
Connect sign-in in GoldenCheetah can break until the library is updated —
recovery time is the sum of how long the library takes to catch up upstream
plus GoldenCheetah's own release cadence, so an outage of hours to weeks is
possible during a transition. The adapter that wraps this library
(`src/Python/garminconnect/garmin_client.py`) is written as a narrow, single
seam specifically so that swapping the underlying library — for a fork, or a
different implementation entirely — is a one-file change rather than a
rewrite of the feature.
