# Garmin Connect — Known Limitations

GoldenCheetah's Garmin Connect integration (Phase 1) has four known
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

## Tokens are stored as files, not in the OS keychain

Garmin Connect sign-in tokens are stored as files inside the GoldenCheetah
athlete configuration directory — `tokens.json` for the session itself, plus
per-account sidecar files beside it (see `src/Cloud/GarminTokenStore.h` and
`src/Cloud/GarminSidecarStore.h`). The files are written atomically and
locked to owner-only (0600) permissions, which is the strongest file-level
protection available. This is still a deliberate trade-off with a residual
risk: any malware running as the SAME OS user can read those files and replay
the tokens, and GoldenCheetah cannot defend against that, because by then the
attacker already holds every right the user has. Storing the tokens in the
operating system keychain instead is the improvement path that removes it,
and a future phase may adopt that if real-world use surfaces concrete
concerns — it is named as a direction, not committed to or scheduled.
