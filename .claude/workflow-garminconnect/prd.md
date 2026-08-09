# PRD — Garmin Connect Integration (Phase 1: Activity Download)

> **Scope reminder (per DEC-001, accepted 2026-05-17):** This PRD covers **Phase 1 only** — read-only activity download from Garmin Connect via the `python-garminconnect` library, embedded via GC's `PythonEmbed` runtime. Workout upload (Phase 2) and schedule push (Phase 3) are explicit non-goals here and will have their own PRDs.

## Goals

1. Let a GoldenCheetah user link a Garmin Connect account and have new Garmin-recorded activities flow into GC's import pipeline automatically — without going through Strava or USB-tethered Garmin Express.
2. Let the user perform a one-time bulk backfill of recent activity history on first connect (default last 90 days; user-configurable).
3. Make Garmin Connect a discoverable, first-class entry in GC's existing Add-Cloud-Service wizard, alongside Strava / Dropbox / RideWithGPS.
4. Ship behind a CMake feature flag so the feature can be disabled per-build until A4 passes.

## User stories

### US-1 — First-time connect
*As an athlete with an existing Garmin Connect account,*  
*I want to* link my Garmin account to GoldenCheetah,  
*so that* my activities reach GC without manual exports.

**Acceptance:**
- A "Garmin Connect" tile is present in `AddCloudWizard`.
- Selecting it shows a credentials form (email, password).
- Submitting valid credentials completes the SSO flow in ≤30s end-to-end (including MFA round-trip if applicable).
- If Garmin requires MFA, GC presents a dialog accepting a 6-digit code; on success, the dialog closes; on failure, an error message describes the cause.
- On first successful connect, GC presents a one-time ToS-risk notice (REQ-009) which the user must acknowledge before the service is enabled.
- Tokens are persisted under the active Athlete's config directory.
- The Garmin password is never written to disk.

### US-2 — Bulk backfill on first connect
*As a new user,*  
*I want* my last 90 days of Garmin activities pulled in on first connect,  
*so that* I see history in GC immediately, not just rides going forward.

**Acceptance:**
- After first-connect success, GC offers a one-click "Import last 90 days" option (date range editable).
- Backfill streams activities through GC's existing FIT/TCX import path; each lands as a normal `RideItem`.
- Progress is visible (count of N/M, current activity name); cancel button works.
- Backfill is resumable on next launch if interrupted (last-successful timestamp persisted).

### US-3 — Incremental sync
*As a connected user,*  
*I want* GC to pull new activities since last sync when I trigger sync (or on a schedule consistent with other CloudServices),  
*so that* new rides appear without manual download.

**Acceptance:**
- Activities newer than the last-successful-sync timestamp are downloaded.
- Already-imported activities (matched by Garmin activity ID + timestamp) are not duplicated.
- Sync completes ≤5s for the common case of 0–3 new activities on typical broadband.

### US-4 — Disconnect and reconnect
*As a connected user,*  
*I want* to be able to disconnect the service,  
*so that* tokens are removed and the account is unlinked.

**Acceptance:**
- "Disconnect" action available on the GC service settings page.
- Disconnecting deletes the stored tokens immediately.
- Reconnecting (US-1) succeeds; previously-imported activities are not re-imported.

### US-5 — Optional profile auto-fill (nice-to-have)
*As a new user,*  
*I want* my Garmin profile (DOB, weight, HRMax) to optionally pre-fill GC's Athlete profile,  
*so that* I don't enter the same data twice.

**Acceptance:**
- Post-connect dialog offers an opt-in checkbox: "Use Garmin profile data to fill missing Athlete fields."
- Only *missing* GC Athlete fields are touched. No overwrite of user data.
- Skippable; defaults to **off** (privacy-leaning).

## Functional Requirements

| ID | Must/Should/Nice | Description | Acceptance criterion | Motivation |
|----|------------------|-------------|-----------------------|------------|
| REQ-001 | must | "Garmin Connect" tile present in `AddCloudWizard` UI. | Tile renders with logo and short description; clicking opens credentials dialog. | US-1 / intake |
| REQ-002 | must | SSO authentication via `python-garminconnect`. | Valid email+password produces persisted OAuth tokens; invalid credentials produce a labeled error. | US-1 |
| REQ-003 | must | MFA OTP prompt when Garmin requires it. | When the library invokes its `prompt_mfa` callback (or raises `GarminConnectAuthenticationError` with an MFA-required signal), GC opens a modal Qt dialog with 6-digit numeric input + Submit/Cancel. Valid OTP completes auth. Invalid OTP shows error and re-prompts up to 3 attempts; after 3rd fail, the connect attempt aborts with a non-retry error. If the library does *not* invoke the MFA callback, no dialog appears. **[A1-001 fix]** | US-1 |
| REQ-004 | must | Per-athlete token storage with safe directory creation. | Tokens written to `<athlete-config-dir>/garminconnect/tokens.json` (atomic tmp+rename, see REQ-NF-Reliab-002). If the parent directory does not exist, it is created with mode `0700` (POSIX) / owner-only ACL (Windows). Two athletes on one GC install have independent tokens. **[A1-016 fix]** | A-04 resolution |
| REQ-005 | must | Garmin password never persisted. | Audit: password string never reaches disk; only OAuth bearer + refresh tokens are stored. The password is held only in the credentials-dialog memory until SSO completes, then zeroed. | REQ-NF-Security |
| REQ-006 | must | Token files restricted to OS user; non-conforming files refused. | POSIX: file mode `0600`. Windows: ACL grants only the owning user. **On load, if the file mode/ACL does not match, GC refuses to read the file, emits an error, and prompts a fresh SSO.** **[F-M4 fix]** | REQ-NF-Security |
| REQ-007 | must | Activity download (FIT default) into existing import pipeline. | GC calls `download_activity(activity_id, fmt='ORIGINAL'->'FIT')` for each new activity. Bytes are written to GC's import staging directory under the activity's natural filename; the standard `FitRideFile` parser produces a `RideItem`. TCX fallback only if FIT is not available for that activity. **[A1-002 fix]** | US-2, US-3 |
| REQ-008 | must | Incremental sync via existing RideCache dedup. | The dedup key is *not* invented here: each downloaded activity is staged as a normal file and offered to GC's existing import path, which dedupes against `RideCache` using the same rules it uses for any file import. Additionally, a **per-account** sidecar map `<athlete-config-dir>/garminconnect/imported-<garmin_user_id>.json` records `garmin_activity_id → local-RideItem-filename` keyed on **Garmin's server-side `startTimeGMT`** (not local clock) to short-circuit repeat downloads without filesystem scanning. The per-account file partition prevents cross-account `garmin_activity_id` collisions if the user disconnects and reconnects with a different Garmin account. **[A1-003 + A1-015 + A2-006 fix]** | US-3 |
| REQ-009 | must | First-connect ToS-risk notice with inline text. | Modal dialog before tokens persist, displaying *exactly* this text (subject to translation): "GoldenCheetah connects to Garmin Connect using the same authentication flow as Garmin's mobile app. Garmin does not officially endorse third-party clients, and aggressive use may, in rare cases, lead to a temporary account restriction. GoldenCheetah limits its requests to a low rate to avoid this. You can disconnect at any time from the Cloud Services settings." Two buttons: "I understand — connect" / "Cancel". Wording is intentionally factual rather than alarmist. **[A1-004 + F-N3 fix]** | A-05 resolution |
| REQ-010 | must | Bulk backfill: configurable date range, default 90 days, capped, paginated, resumable, cancellable, atomic-per-page. | Default 90 days; user-editable; **hard cap 5 years** (configurable via advanced setting). Paginated at 20 activities/page with 1s inter-page delay (REQ-NF-Perf-002). Empty result (no activities in range) is a **success**, not an error. Progress and cancel both functional. **Resumability:** the last successfully-imported activity's `startTimeGMT` is persisted after each successful per-activity import (atomic write). On next launch, backfill resumes from that timestamp. Interruption types covered: (a) user-cancel — preserved partial results, ack on next launch; (b) hard crash / kill — same; (c) Garmin transient HTTP error — retries per REQ-NF-Reliab-001 then surfaces error and pauses backfill; (d) torn write of a per-activity FIT file — see REQ-NF-Reliab-002. **[A1-005 + A1-013 + A1-014 fix]** | US-2 |
| REQ-011 | must | Phase-1 is read-only (Garmin side). | `GarminConnect::capabilities()` returns `Query | Download` only. No code path writes to `connect.garmin.com` endpoints. (Writes to *GC's own* athlete profile via REQ-013 are GC-side and allowed.) **[A1 contradiction clarification]** | A0.3-001 / DEC-001 staging |
| REQ-012 | should | Disconnect deletes tokens; per-account sidecars preserved. | "Disconnect" deletes the token file (`tokens.json`) before clearing in-memory state. Reconnect must perform a full SSO. **All per-account sidecars** (`imported-<uid>.json`, `backfill-state-<uid>.json`) **are preserved** — reconnecting with the same Garmin account resumes its history; reconnecting with a different account creates/uses that account's own sidecar files. Prior-account sidecars sit on disk untouched and are not consulted by the active session. **[A2-006 clarification]** | US-4 |
| REQ-013 | nice | Optional profile auto-fill (opt-in, fills only missing Athlete fields). | Acceptance per US-5. Defaults off. | US-5 |
| REQ-014 | must | Friendly error translation for Garmin auth failures. | Library raises `GarminConnectAuthenticationError`, `GarminConnectConnectionError`, `GarminConnectTooManyRequestsError`. GC maps each to a user-friendly localized message via a fixed switch table, never showing the raw exception name. Unknown codes fall back to a generic "Connection to Garmin Connect failed" + code. **[F-N1 fix]** | US-1 / Persona |
| REQ-015 | must | CAPTCHA-detected path. | If SSO returns a CAPTCHA challenge (library raises a recognisable signal or returns specific HTTP status), GC closes the auth flow and shows a dialog: "Garmin Connect is asking for a CAPTCHA. Please log in once at https://connect.garmin.com in your browser, then try again here." Button to open the URL in default browser. **[F-N2 fix]** | US-1 / Persona |
| REQ-016 | should | Dedup record survives an import that fails downstream. | The Tier-1 sidecar (`imported-<uid>.json`) must NOT mark an activity imported until GC's import pipeline has confirmed a parsed RideItem — OR a reconcile pass must re-download activities whose recorded RideItem is absent. Currently `readFile` records at stage-time (before the async parse), so a download that passes the FIT magic-sniff but fails the full parse (or any TCX) is recorded and silently, permanently skipped on future syncs. Acceptance: an activity whose bytes stage but fail to parse is re-attempted on a subsequent sync (not permanently dropped), and this is covered by a test that feeds unparseable staged bytes. **[A3-R008-F1 + F2 fix]** | US-3 reliability / A3-R008 |
| REQ-017 | must | Session lifetime is bound to the connected account; Disconnect deterministically stops live sessions. | On Disconnect, a `GarminConnect` instance that was `open()`ed *before* the disconnect must stop being usable **deterministically, by binding — not by a per-call disk re-check**: (a) with `accountStillConnected()` neutralised (stubbed always-true, i.e. DEC-020's fail-closed guard removed), the A3-R012-F1 exploit still downloads nothing — the live instance's next `readdir`/`readFile` fails with a Garmin-labelled error and issues zero list/download calls; (b) the restored garth session is torn down (worker thread stopped, adapter session released) rather than living to process exit — read FUNCTIONALLY per DEC-021: it never outlives the owning window, which makes clause (e)'s dialog-destructor fix load-bearing rather than parallel; (c) **[narrowed at DEC-021, 2026-08-03]** a download whose result lands after the disconnect has that result **discarded** — neither staged nor recorded in any sidecar — via a post-download, pre-stage recheck, closing the TOCTOU residual DEC-020 accepted. True mid-flight *cancellation* is explicitly OUT of scope and unreachable today (no cancel primitive exists in `GarminWorker`/`PyEmbeddedAdapter`; DES-001 invariant 3 forbids `terminate()`); the in-flight HTTP call runs to completion or its 60s watchdog, and only its result is dropped. An interruptible download path needs its own REQ+DEC — do not re-read this clause as though cancellation were implemented; (d) the `garmin_user_id` is **latched at session open** and used by `readdir`/`readFile`/`recordImport` for the life of that session, so a mid-sync change to `active-account.json` can never record an import against a different-or-empty account **[A3-R012-F10 fix]**; (e) an owner that opens a `CloudService` closes and destroys it — `CloudServiceSyncDialog` (CloudService.h:336, no dtor) and `MainWindow::syncCloud` (MainWindow.cpp:2561-2566) no longer leak their store, worker thread and interpreter session past dialog close **[A3-R012-F12 fix]**. The mechanism is DEC-021's call; this criterion is outcome-shaped and must not be weakened into a restatement of the DEC-020 guard. | A3-R012-F1 residual + F10 + F12; the deferred remainder of DEC-020 Option A |
| REQ-018 | must | A downloaded Garmin activity actually imports (compression contract). | A successfully downloaded Garmin activity is accepted by `CloudService::uncompressRide` and yields a parsed `RideFile`, in BOTH consumers (`CloudServiceSyncDialog::completedRead` and `CloudServiceAutoDownload::readComplete`). Today it cannot: the base ctor defaults `downloadCompression` to `zip` (CloudService.cpp:54), `uncompressRide` rejects any name not ending `.zip` with `tr("expected compressed activity file.")` (CloudService.cpp:239-241), and `GarminConnect` never overrides it while staging/listing `garmin-<id>.fit` (GarminConnect.cpp:492, :625) and `garmin-<id>.tcx` (:522) — so every successful download is rejected at the first guard. **The acceptance test must CROSS the `uncompressRide` boundary** (bytes → completion → `uncompressRide` → non-NULL `RideFile`), not stop at `readFile`'s staging: the defect survived a 22/22-green suite precisely because no test crossed it, so a test that only asserts staging does not satisfy this REQ. Covers both the FIT and the TCX fallback names. **[B-R017-09 fix]** | US-2/US-3 — the download feature is inert in production without it; found during the DEC-022 slice |
| REQ-019 | must | **[STUB — follow-up wave; scoped 2026-08-07]** The UPLOAD dialog must not use-after-free on parent teardown (same class as REQ-017's sync fix). | `CloudServiceUploadDialog`/`CloudService::upload`/`MainWindow::uploadCloud` currently reproduce the pre-DEC-024..027 UAF verbatim: a STACK dialog parented to the WA_DeleteOnClose MainWindow whose ctor runs `store->open()` + two `QMessageBox::exec()` nested loops unguarded (CloudService.cpp:78-88, 326-412; MainWindow.cpp:2548-2565). **ACCEPTANCE (elaborated 2026-08-07 with DEC-029; this is the criterion the test must encode as written, not a weaker paraphrase):** an executed AddressSanitizer test constructs `CloudServiceUploadDialog` as a heap child of a `WA_DeleteOnClose` MainWindow stand-in and tears that parent down at EACH of the four blocking-call suspension points — `store->open()`, the unsaved-changes `QMessageBox::exec()`, `compressRide`/`writeFile`, and the `exec()` wait for `writeComplete` — and reports ZERO heap-use-after-free and ZERO bad-free on the dialog itself or on `context`/`context->mainWindow`; AND the store (`db`) is closed-and-deleted exactly once with no double-free, with no leak beyond the deliberate busy-teardown residual DEC-025 already established as tolerated. Each guard must be shown load-bearing by mutation (neuter it → that slot dies under ASan). HIGH priority: live for 11 Upload-capable services (GarminConnect is NOT one — `GarminConnect.h:76` is `Query|Download` per DEC-005). **[A3-R027-F1 fix; DEC-029 Option B]** | US-2 upload path; found by the DEC-027 A3 sibling-scan (LSN-041) |
| REQ-021 | must | **[STUB — scoped 2026-08-08 from A3-R019-F1/F2; covers BOTH dialogs in one fix]** A cloud dialog must not use-after-free on its COLLABORATORS (`context`, `item`) when the athlete tab is closed underneath it — a distinct axis from REQ-017/REQ-019, which guarded only the dialog's OWN lifetime. | The dialogs are parented to `MainWindow`, but `Context`/`RideItem` are owned by the narrower-lived `AthleteTab`. `MainWindow::removeAthleteTab` (MainWindow.cpp:2183-2185) deletes `tab`/`athlete`/`context` SYNCHRONOUSLY, while MainWindow's own `WA_DeleteOnClose` deletion is DEFERRED (`deleteLater`) — so a dialog reliably OUTLIVES its Context, every `self.isNull()` bail stays false, and `start()` resumes onto freed memory. Two routes: whole-window close (`MainWindow::closeEvent` :1103) and single-tab close (`AthleteView.cpp:213` → `closeAthleteTab`). Affected, both to be fixed together: **`CloudServiceUploadDialog::start()`** (CloudService.cpp:424, :443-445, :469, :473 — REQ-019's new code) and **`CloudServiceSyncDialog::start()`** (CloudService.cpp:1070, :1094-1099 — ALREADY SHIPPED in `ae5a7a8ab`, so this is live on master). **Acceptance (to elaborate when this REQ starts):** an executed ASan test in which a purpose-built owner destroys its `Context` SYNCHRONOUSLY while deferring its own deletion via `deleteLater()` — modelling `MainWindow::closeEvent`'s real two-phase order, which the current harness cannot express — shows zero UAF on `context`/`item` at every suspension point in BOTH dialogs, with each new guard mutation-proven load-bearing. Requires harness work first: the ASan target's `MainWindow`/`Context`/`RideItem` are inert stubs (`ImportSeamStubs.cpp`), which also caused A3-R019-F3. **[A3-R019-F1 + A3-R019-F2 fix; user scope decision 2026-08-08: commit REQ-019, fix this axis in one REQ across both dialogs]** | US-2 upload + US-3 sync paths; found by the REQ-019 A3 going one level up the call chain |
| REQ-020 | should | **[STUB — follow-up wave; scoped 2026-08-07]** The OAuth "Add Cloud Account" wizard auth must not use-after-free while the user completes browser OAuth. | `AddCloudWizard::AddAuth::doAuth()` runs `oauthDialog->exec()` (arbitrarily long) then touches `token`/`message`/`messageLabel`/`wizard` members with no self-bail, on a `QWizard(context->mainWindow)` + WA_DeleteOnClose ancestor (AddCloudWizard.cpp:104-112, 474-508). Acceptance (to elaborate): a QPointer self-bail after `oauthDialog->exec()` (or equivalent lifetime rework) + a teardown-during-OAuth test. Live for every OAuth service. Also track A3-R027-F3 (dormant folder-browse, same class, arms if any service sets `CloudService::Folder`). **[A3-R027-F2 fix; F3 tracked]** | US-1 account setup; DEC-027 A3 sibling-scan |

## Non-Functional Requirements

| ID | Category | Description | Verification |
|----|----------|-------------|--------------|
| REQ-NF-Perf-001 | Performance | First-connect SSO round-trip ≤30s on **50 Mbit down / 10 Mbit up** broadband, including MFA round-trip. **[A1-006 fix]** | Integration test or manual stopwatch on reference network. |
| REQ-NF-Perf-002 | Performance / rate-limit | No more than **1 request/sec** sustained to `connect.garmin.com`; bulk-backfill paginates at 20 activities/page with 1s inter-page delay. **Concurrent sync attempts** (user clicks Sync while one is running, or auto-sync overlaps): the second invocation is **rejected** with "sync already in progress"; the running one continues. **[A1-012 fix]** | Unit test on the rate-limiter; integration test on concurrent-invoke. |
| REQ-NF-Perf-003 | Performance | Incremental sync of 0–3 new activities completes ≤5s end-to-end on 50/10 Mbit broadband. **[A1-006 fix]** | Manual stopwatch; logged duration. |
| REQ-NF-Sec-001 | Security | Password never persisted; only tokens. | Code review + grep audit for the password handle. |
| REQ-NF-Sec-002 | Security | Token file permission `0600` POSIX / owner-only ACL Windows. | Unit test on file mode; manual Windows ACL check. |
| REQ-NF-Sec-003 | Security | All network traffic is HTTPS validated against the **OS system trust store** via `curl_cffi` default behaviour. No certificate pinning in Phase 1. `verify=False` is forbidden anywhere in the code path. **[A1-007 fix]** | Code-grep for `verify=False`; manual MITM-proxy test. |
| REQ-NF-Sec-004 | Security | Token storage is file-based in Phase 1 with documented residual risk (same-user malware can replay tokens). OS-keychain integration is a Phase-1.5 follow-up if beta surfaces concrete concerns. **[F-M1 disposition]** | Documented in README + first-connect notice. |
| REQ-NF-Reliab-001 | Reliability | Network errors retried with exponential backoff (250 ms → 2 s, max 3 attempts). Permanent failures surface to UI with the Garmin error code. | Unit test; UI inspection. |
| REQ-NF-Reliab-002 | Reliability — atomic writes | All on-disk state (tokens.json, imported.json, in-flight FIT files) is written via **tmp-and-rename**: write to `*.tmp` in the same directory, `fsync`, then atomic `rename` over the destination. Torn writes are detected on read (parse failure → fallback to last-good or trigger re-fetch). Backfill is resumable across crashes via the post-each-success atomicity. **[A1-008 + A1-017 fix]** | Unit tests on the atomic-writer; integration test: SIGKILL mid-write, restart, verify no corruption. |
| REQ-NF-Threads-001 | Concurrency / UX | All Garmin Connect calls run off the GUI thread via a request/response queue (no direct PyObject deref from the UI thread — silent dependency on DEC-002). The QApplication event loop must not stall > **100 ms** while a sync is in progress, measured by an in-process frame-time monitor enabled in dev builds. **[A1-009 fix]** | Dev-build instrumentation that logs stalls > 100 ms; manual observation. |
| REQ-NF-Cancel-001 | UX | User can cancel an in-progress sync or backfill; in-flight request is allowed to complete; subsequent ones are skipped. Partial results are preserved (no rollback). | Integration test. |
| REQ-NF-Obs-001 | Observability | All sync ops emit user-facing errors via **`ErrorBus`** (same channel as other CloudServices) and developer-trace logging via `qDebug`. Structured fields: op name, duration, activity count, Garmin error code. **[A1-010 fix]** | Log-format review; ErrorBus subscriber test. |
| REQ-NF-i18n-001 | i18n | All user-facing strings use `tr()` and appear in `.ts` translation files. | Translation-file diff. |
| REQ-NF-Build-001 | Build | Feature gated behind CMake flag `GC_WANT_GARMINCONNECT`, default **OFF** until A4 passes. Build with flag OFF must not require `garminconnect` or `curl_cffi`. | CMake configure with flag OFF on a machine without the Python deps succeeds. |
| REQ-NF-Pkg-001 | Packaging | Installer (Win/macOS/Linux AppImage) bundles `garminconnect`+`curl_cffi` when the build flag is ON. **Phase 1: manual smoke checklist** for the 3 platforms (documented in CONTRIBUTING). **Phase 2: CI smoke job per platform** asserting `import garminconnect` works inside the installed bundle. **[A1-011 disposition]** | Manual checklist Phase 1; CI Phase 2. |
| REQ-NF-Compat-001 | Known limitations | (a) **Concurrent** multi-account: only one Garmin account is connected at a time per GC Athlete. Switching accounts (Disconnect + reconnect with different credentials) is supported; per-account sidecars are preserved on disk (see REQ-012). (b) If Garmin invalidates the refresh token (e.g., password change on Garmin side), GC must prompt full re-login — no silent reauth. (c) **Library-tracked SSO risk:** Phase 1 depends on `python-garminconnect`, an unofficial third-party library, to track Garmin's SSO changes. When Garmin changes the SSO flow, the recovery path is `library-upstream-fix-time + GC-release-cadence` — outages of hours-to-weeks are possible during transitions. DES-012's adapter seam contains the swap cost if the library needs to be replaced. Documented in README + connect dialog. **[F-P3 + REQ-005↔REQ-002 contradiction disposition + A2-002 + A2-006]** | Doc review. |

## Success Metrics

| Metric | Target | Window |
|--------|--------|--------|
| Beta first-connect success rate | ≥80% of users complete connect (incl. MFA) on first attempt | First 30 days of beta |
| Steady-state sync error rate | <5% per sync session | After first 7 days post-connect |
| Support reports of Garmin account suspension attributable to this feature | 0 | 90 days post-launch |
| Activity-import latency (incremental, 0–3 activities) | p50 ≤5s, p95 ≤15s | First 30 days |
| Beta-user retention (still connected after 30 days) | ≥70% | 30 days |

## Explicit Non-Goals (deferred to later phases or out-of-scope)

| Item | Reason | Where it lives |
|------|--------|----------------|
| Workout upload to Garmin Connect | Phase 2; separate DEC family + A4 cycle | options-catalog O-04 |
| Schedule push to Garmin Calendar | Phase 3 | options-catalog O-05 |
| Activity deletion on Garmin side | Phase 3+ | options-catalog O-02 |
| Training-plan upload | Phase 3+ | options-catalog O-07 |
| HRV / training-readiness / VO2 ingestion | Phase 4 candidate; depends on AI Coach consumer signal | options-catalog O-08 |
| Body-composition / weight ingestion | Phase 4 candidate; overlap with Withings — needs merge strategy | options-catalog O-10 |
| Sleep / hydration ingestion | Phase 4+ | options-catalog O-14 |
| Gear / equipment sync | Phase 4+ | options-catalog O-13 |
| Race-prediction / PRs / badges | Phase 4+ (cosmetic) | options-catalog O-11 |
| Golf data | Permanent non-goal (off-domain) | options-catalog O-17 |

## Open assumptions (call out at A1)

- A. `python-garminconnect` and `curl_cffi` are bundleable on Windows / macOS / Linux without requiring system-Python. To be validated empirically during Phase 2 bootstrap.
- B. Garmin's SSO endpoint will accept the library's user-agent / TLS-fingerprint from a desktop-bundled CPython. The library does this from a CLI today; a desktop-embed has not been publicly validated.
- C. The existing `PythonEmbed` runtime can host network-IO modules safely (it's used for compute-only FixPy today). Threading and GIL behaviour under long-running network calls is unproven in this codebase.

These are not REQs — they are *risks*. A2 will turn each into a failure narrative and either disposition them or escalate.
