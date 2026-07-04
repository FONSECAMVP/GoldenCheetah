# A3 — REQ-002 end-to-end slice (GarminWorker + WorkerAuthClient + IGarminPyAdapter)

**Run:** 2026-05-24
**Scope:** GREEN files of the REQ-002 end-to-end slice:
- `src/Cloud/IGarminPyAdapter.h` (DES-001a — header-only contract)
- `src/Cloud/GarminWorker.{h,cpp}` (DES-001 — Auth-only subset)
- `src/Cloud/WorkerAuthClient.{h,cpp}` (DES-003 concrete adapter)
- `src/Cloud/IGarminAuthClient.h` (Q_DECLARE_METATYPE additions for cross-thread)

**Driving tests:** `unittests/Core/garminconnect/testGarminConnectAuthClient.cpp` — 10 assertions in 8 slots (5 worker-side; 3 worker-on-real-QThread).

**Tooling:** Manual mutation enumeration. C++ mutation tooling (mull-cxx / cosmic-ray-cpp) remains deferred per A3-R001-tool — re-evaluate at Phase 3 entry.

## Mutation enumeration (proposed → verdict)

| ID | Mutation | Killing test | Verdict |
|---|---|---|---|
| E-W1 | `case Success` body → emit `failed(Auth)` instead of `finished` | successOutcomeEmitsFinishedWithSameRequestId — `finishedSpy.size()==1` and `failedSpy.size()==0` | KILLED |
| E-W2 | `case AuthFailed` → set `err.kind = Network` | authFailedOutcomeMapsToFailedWithAuthKind — `QCOMPARE(emitted.kind, GarminAuthFailure::Auth)` | KILLED |
| E-W3 | `case Network` → set `err.kind = Auth` | networkOutcomeMapsToFailedWithNetworkKind — `QCOMPARE(emitted.kind, GarminAuthFailure::Network)` | KILLED |
| E-W4 | `case Unknown/default` → set `err.kind = Auth` | unknownOutcomeMapsToFailedWithUnknownKind — `QCOMPARE(emitted.kind, GarminAuthFailure::Unknown)` | KILLED |
| E-W5 | Drop requestId on `finished` (emit with `QUuid()`) | successOutcomeEmitsFinishedWithSameRequestId — `QCOMPARE(finishedSpy.first().at(0).toUuid(), id)` | KILLED |
| E-W6 | Drop requestId on `failed` (emit with `QUuid()`) | authFailedOutcomeMapsToFailedWithAuthKind — `QCOMPARE(failedSpy.first().at(0).toUuid(), id)` | KILLED |
| E-W7 | Swap fields on Success: `garmin_user_id = outcome.display_name` | successOutcomeEmitsFinishedWithSameRequestId — `QCOMPARE(emitted.garmin_user_id, "uid-99")` | KILLED |
| E-W8 | Empty out `translatedMessage` on failure | authFailedOutcomeMapsToFailedWithAuthKind / networkOutcomeMapsToFailedWithNetworkKind — `QCOMPARE(emitted.translatedMessage, …)` | KILLED |
| E-W9 | Default switch case omits `emit failed(...)` (silent drop) | unknownOutcomeMapsToFailedWithUnknownKind — `QCOMPARE(failedSpy.size(), 1)` | KILLED |
| E-W10 | Argument swap: `m_py->authenticate(password, email)` | credentialsForwardedToAdapterVerbatim — `QCOMPARE(fake.lastEmail, …)` | KILLED |
| E-W11 | Drop the `m_py->authenticate()` call entirely | credentialsForwardedToAdapterVerbatim — `callCount==1`; successOutcome…SameRequestId — finishedSpy stays empty (outcome default = Unknown → failed) | KILLED |
| E-W12 | Emit `finished` AND `failed` on Success (double-emit) | successOutcomeEmitsFinishedWithSameRequestId — `QCOMPARE(failedSpy.size(), 0)` | KILLED |
| E-W13 | Switch fall-through (omit `return`) so Success leaks into AuthFailed | successOutcomeEmitsFinishedWithSameRequestId — `QCOMPARE(failedSpy.size(), 0)` | KILLED |
| E-WAC1 | Don't `emit dispatchAuthenticate(...)` in `WorkerAuthClient::authenticate` | workerAuthClientForwardsAndAdapterRunsOnWorkerThread — `waitFor(finishedSpy.size>=1)` times out | KILLED |
| E-WAC2 | Argument swap in `emit dispatchAuthenticate(password, email, requestId)` | workerAuthClientForwardsAndAdapterRunsOnWorkerThread — `QCOMPARE(fake.lastEmail, "rider@example.com")` | KILLED |
| E-WAC3 | Drop `connect(worker, &finished, this, &IGarminAuthClient::finished)` | workerAuthClientReemitsFinished — `waitFor(finishedSpy.size>=1)` times out | KILLED |
| E-WAC4 | Drop `connect(worker, &failed, this, &IGarminAuthClient::failed)` | workerAuthClientReemitsFailed — `waitFor(failedSpy.size>=1)` times out | KILLED |
| E-WAC5 | Force `Qt::DirectConnection` on the dispatch (skip thread-hop) | workerAuthClientForwardsAndAdapterRunsOnWorkerThread — `QVERIFY(fake.threadSeen != QThread::currentThread())` and `fake.threadSeen == &workerThread` | KILLED |
| E-WAC6 | Pass `QUuid()` instead of `requestId` in the emit | workerAuthClientReemitsFinished — `QCOMPARE(finishedSpy.first().at(0).toUuid(), id)` | KILLED |
| E-WAC7 | Omit `qRegisterMetaType<GarminAuthSuccess>()` (Qt-6 with `Q_DECLARE_METATYPE` auto-registers) | none — no observable behaviour change on supported Qt 6 | NON-MUTATION (no observable behaviour change on supported Qt; accept-with-rationale) |

## Findings

- **Open:** 0 (zero blocking survivors).
- **Accept-with-rationale:** E-WAC7 — Qt 6 auto-registers `Q_DECLARE_METATYPE`-declared types for queued connections; the explicit `qRegisterMetaType` calls are defence-in-depth for older Qt versions but produce no observable behaviour change on the supported Qt 6 toolchain. Documented in `WorkerAuthClient.cpp` comment.
- **Defer:** none (no new survivors; existing A3-R001-tool stays deferred to Phase 3 entry).

## Status

- [x] Clean pass for REQ-002 end-to-end slice — no kill tests added (the 10 existing assertions cover every plausible mutation).
- All `garmin-fast` tests green: 4/4 executables, 27 + 10 = 37 assertions.
- Next cycle: A3 / REQ-003 (MFA) when its TDD slice lands (extends `IGarminAuthClient` with `mfaRequired(QUuid)` and adds a new worker slot).
