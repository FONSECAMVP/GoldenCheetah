# System Design — Garmin Connect Integration (Phase 1 — download only)

Last updated: 2026-05-17 (Phase 1.4 initial draft).

> **Scope:** This document specifies the *implementation shape* for Phase 1 (read-only activity download). It does not specify Phase 2 (workout upload) or Phase 3 (schedule push); those will have their own DES families.

## Component overview

```
                       ┌──────────────────────────┐
                       │     Qt GUI thread        │
   AddCloudWizard      │                          │
   (DES-003) ──────────►  GarminConnect           │
                       │  : CloudService (DES-004)│
                       └──────┬───────────────────┘
                              │  Qt signal: request(payload)
                              ▼
                       ┌──────────────────────────┐
                       │   GarminWorker (DES-001) │
                       │   QObject in QThread     │
                       │   - mailbox queue        │
                       │   - cancellation token   │
                       │   - sole GIL holder      │
                       └──────┬───────────────────┘
                              │  PyObject calls
                              ▼
            ┌────────────────────────────────────────────┐
            │  Embedded CPython (existing PythonEmbed)   │
            │  ┌──────────────────────────────────────┐  │
            │  │ gc_rate.py (DES-005)                 │  │
            │  │  - 1 req/s token bucket              │  │
            │  │  - exponential backoff decorator     │  │
            │  └────────────────┬─────────────────────┘  │
            │                   │ wraps every call to    │
            │                   ▼                        │
            │  garminconnect / curl_cffi (vendor libs)   │
            └────────────────────────────────────────────┘
                              │  HTTPS (OS trust store)
                              ▼
                      connect.garmin.com

   Persistence (DES-002 / DES-006 atomic-writer):
     <athlete-config-dir>/garminconnect/tokens.json
     <athlete-config-dir>/garminconnect/imported-<garmin_user_id>.json       (per-account)
     <athlete-config-dir>/garminconnect/backfill-state-<garmin_user_id>.json (per-account)

   Build gate (DES-007):
     -DGC_WANT_GARMINCONNECT=ON  → adds src/Cloud/GarminConnect.* + python deps
```

The arrow direction is **UI → Worker → Python**. Nothing in the Python layer or the worker ever calls back into the UI directly; results return via Qt `finished(payload)` signals delivered to the GUI thread's event loop.

---

## DES-001 — GarminWorker: dedicated worker thread + mailbox

**Implements:** DEC-002.
**Serves:** REQ-002, REQ-003, REQ-007, REQ-010, REQ-NF-Perf-002, REQ-NF-Perf-003, REQ-NF-Threads-001, REQ-NF-Cancel-001.

### Shape

A `GarminWorker : QObject` lives on a dedicated `QThread`. The worker owns the *sole* reference to the embedded-Python sub-interpreter used for the Garmin client. The worker calls only the **GC adapter** (DES-012) — it never imports `garminconnect` directly. The worker exposes a Qt-signal API:

```cpp
// public (called from GUI thread)
void enqueue(const GcRequest& req);     // queue a request
void cancel(QUuid requestId);           // cooperative cancel for in-flight + queued

// signals (received by GUI thread)
void finished(const GcResult& result);
void progress(QUuid requestId, int done, int total, QString label);
void error(QUuid requestId, GcError err);
void mfaRequired(QUuid requestId);      // worker pauses, waits for submitMfa
void captchaDetected(QUuid requestId);
```

`GcRequest` is a tagged union of operation kinds: `Authenticate`, `RefreshTokens`, `ListActivitiesSince(ts)`, `DownloadActivity(id, fmt)`, `FetchProfile`, `Disconnect`. Each carries a `QUuid requestId` so the GUI can correlate emitted signals.

### Mailbox semantics

- Queue is FIFO, bounded to **8** in-flight + pending; an attempt to enqueue when full returns `QueueFull` synchronously (this is consumed by REQ-NF-Perf-002's *"sync already in progress" → reject* rule).
- Cancellation is **cooperative**: the rate-limiter checkpoint (DES-005) and the per-activity loop (DES-009) check an `std::atomic<bool>` cancellation flag between requests.
- The worker holds the GIL only inside Python calls; the rate-limiter sleep (`time.sleep`) **releases** the GIL, so cancellation is responsive even mid-pause.

### Thread-safety invariants

1. The GUI thread **never** dereferences a `PyObject*`. Test-asserted: a debug-build hook (`PyGILState_Check`) at every PyObject access logs a thread id; tests assert it equals the worker's id.
2. All state crossing the thread boundary is value-copied (POD or QVariant). No shared mutable state.
3. The worker's destructor calls `quit()` + `wait()` on its `QThread` from the GUI thread; the worker thread does not own its lifetime.

### Failure modes (cross-link)

- Embedded Python raises uncaught: caught by a top-level `try/except` in the worker; converted to `GcError` and emitted via `error()`.
- Python sub-interpreter init fails: emit fatal `GcError`; `GarminConnect::capabilities()` returns 0 until reset.
- **Sub-interpreter wedges** (deadlock, native crash inside Python, GIL stuck): no automatic recovery in Phase 1 — `capabilities()` stays 0, user-facing tile becomes non-functional, full GC restart required to recover. **Phase 1.5 follow-up: thread-heartbeat with kill-and-recreate.** **[A2-001 disposition]**
- Worker thread blocked >10s: instrumented; logged. Does not block UI by construction.

---

## DES-001a — `IGarminPyAdapter` interface (worker ↔ Python seam)

**Implements:** DEC-013.
**Serves:** REQ-002 (end-to-end Authenticate testable in C++); future REQ-003/006/007/010/012/014 worker ops (extended additively).
**Composes with:** DES-001 (`GarminWorker` is constructed with an `IGarminPyAdapter*`), DES-012 (production `PyEmbeddedAdapter` calls `garmin_client.GarminClient.login()`), DES-008 (failure-kind enum maps to translated messages at the page layer; the adapter returns raw kinds, not translated strings).

### Header location

`src/Cloud/IGarminPyAdapter.h` — pure header-only contract. Must compile with `GC_WANT_GARMINCONNECT=OFF` (no `Python.h`, no embedded-interpreter symbols). No transitive Python dependencies; safe for the `garmin-fast` CTest label.

### Surface (Authenticate-only in REQ-002 slice; extends additively in later slices)

```cpp
// IGarminPyAdapter.h
#pragma once
#include <QString>

struct PyAuthOutcome {
    enum Kind { Success, AuthFailed, Network, Unknown };
    Kind kind = Unknown;
    // Populated only when kind == Success:
    QString garmin_user_id;
    QString display_name;
    // Populated for non-Success; raw library message (DES-008 translates at page layer):
    QString rawMessage;
};

class IGarminPyAdapter {
  public:
    virtual ~IGarminPyAdapter() = default;
    // Synchronous from the worker-thread caller's perspective: the worker
    // calls this from its own thread; the GUI thread never invokes this method.
    // Returning a value (vs emitting a signal) keeps the interface free of
    // QObject-inheritance — production PyEmbeddedAdapter does not need to be a
    // QObject, and FakePyAdapter is trivially constructible in unit tests.
    virtual PyAuthOutcome authenticate(const QString& email, const QString& password) = 0;

    // REQ-007 additive extension (slice 2). Fetch one activity's bytes in
    // the requested format ("ORIGINAL" for FIT, "TCX" for the DES-004 fallback);
    // reuses the session authenticate() established.
    virtual PyDownloadOutcome downloadActivity(const QString& activityId, const QString& fmt) = 0;
};

// REQ-007 value type (mirror of PyAuthOutcome one op down):
//   struct PyDownloadOutcome {
//       enum Kind { Success, Network, RateLimited, Unknown };
//       Kind kind = Unknown;
//       QByteArray data;      // populated only on Success; binary-safe (keeps NUL)
//       QString  rawMessage;  // non-Success raw library message (DES-008 translates)
//   };
// RateLimited is a distinct Kind (not folded to Unknown) so DES-008 rate-limit
// copy and DES-005/DES-010 pacing can key off it.
```

### Lifecycle and ownership

The wizard creates the production `PyEmbeddedAdapter` once (per-athlete) and passes it to `GarminWorker`'s constructor. The worker stores the raw pointer; it does not own it. Destruction order: wizard outlives worker outlives adapter. In tests, the fake's lifetime is the test slot's.

**REQ-007 session model (slice 2):** because REQ-005 forbids retaining the password, a download cannot construct a fresh authenticated client. The production `PyEmbeddedAdapter` therefore **retains the authenticated Python `GarminClient`** established by a successful `authenticate()` (member `m_client`, owned ref) and `downloadActivity()` reuses it; called before any successful auth it returns `Unknown`/"not authenticated". The adapter is worker-thread-confined (DES-001), so the retained handle has a single-threaded access model; its destructor releases the ref under the GIL only while `Py_IsInitialized()` (a finalized-interpreter DECREF would be use-after-free). This makes the per-athlete adapter the sole holder of the live library session across auth+download.

### Why an interface, not virtual on `GarminWorker`

DEC-013 weighed (A) injected interface, (B) virtual `doAuthenticate()` subclassed in tests, (C) `#ifdef TESTING` swap. (A) won on:
- compile-time guarantees (a real adapter that forgets to implement a new op is a build break, not a runtime no-op);
- preserves the `garmin-fast` CTest Python-free invariant by construction (the worker translation unit pulls only `IGarminPyAdapter.h`, never `Python.h`);
- mirrors DEC-012 so every layer of the dispatch chain (page → worker → Python) uses the same DI pattern;
- additive extension for future ops (REQ-003 SubmitMfa, REQ-006 RefreshTokens, REQ-007 DownloadActivity, REQ-010 ListActivitiesSince, REQ-012 Disconnect, REQ-014 FetchProfile) without test-shape rework.

### What is **not** specified by this interface

- Threading (DES-001 owns — the worker is what makes this call on a non-GUI thread).
- Rate-limit / retry (DES-005 — applied inside the production adapter implementation as Python-side decorators).
- Token persistence (DES-002 — the production adapter writes `tokens.json` after `Success`; failed outcomes leave disk state untouched).
- Translation of failure messages (DES-008 — the page layer maps `Kind` to a localized string; the adapter returns raw enum + raw library text).

These stay one level below the interface so the worker tests need not stand any of them up.

---

## DES-002 — Per-athlete storage layer

**Implements:** DEC-003, DEC-021 (the account epoch, below).
**Serves:** REQ-004, REQ-006, REQ-008, REQ-010, REQ-012, REQ-017, REQ-NF-Reliab-002, REQ-NF-Sec-002, REQ-NF-Sec-004.

### The account epoch — in-memory, NOT storage [added 2026-08-05, DEC-021 / REQ-017, VAL-017 check-3 cascade]

Alongside the on-disk state below there is one piece of **deliberately non-persistent** per-athlete state:
`GarminAccountEpoch` (`src/Cloud/GarminAccountEpoch.{h,cpp}`) — a process-global
`static QHash<QString /*configDir*/, quint64>` guarded by a `QMutex`, exposing `current(dir)` / `bump(dir)`.
It is pure Qt (no `Python.h`), the same seam discipline as `GarminDownloadChain.h`, so every Python-free
`garmin-fast` target can link it.

`GarminConnect::open()` latches `m_openedEpoch = current(dir)` **and** `m_openedUserId = resolveGarminUserId()`;
`disconnectService()` calls `bump(dir)` alongside `GarminTokenStore::clearAccount(dir)`; `readFile`/`readdir` refuse
when `current(dir) != m_openedEpoch` (`sessionSuperseded()`), and `downloadResultStillWanted()` re-checks after a
download completes but before staging/recording. This answers a question the on-disk credential cannot:
`GarminTokenStore::loadChecked` (DEC-020) answers *"is SOME account connected right now?"*, whereas the epoch
answers *"is the account THIS session opened against still the connected one?"* — disconnect-then-reconnect answers
the first YES while the second is NO.

**Why it is NOT persisted:** it exists only to invalidate live in-process sessions, and no live session survives a
restart. It is not a revocation record and must not be treated as one.

**Why `bump()` touches nothing but the map:** `blockingDownload()` runs a nested `QEventLoop`, so a disconnect can
land inside a live download frame; reaching into another `GarminConnect` from there would be a use-after-free (the
A3-R007-01 hazard). Invalidation is therefore LAZY — a superseded session refuses at its next call rather than being
torn down at the instant of disconnect. That is why REQ-017 clause (b) is read functionally and why the
`CloudServiceSyncDialog` ownership contract (DES-014) is load-bearing rather than cleanup.

### Paths

All paths are rooted at `<athlete-config-dir>` (the same directory used by other CloudServices for per-athlete state):

```
<athlete-config-dir>/garminconnect/
  ├── tokens.json                                # OAuth bearer + refresh tokens (singular per athlete — only one account connected at a time)
  ├── active-account.json                        # { garmin_user_id } — the ACTIVE account pointer (DEC-018 Option B). Deliberately NOT inside tokens.json, whose schema is security-locked by REQ-006/007. Tolerant read: absent/torn -> empty uid -> callers no-op. Deleted on Disconnect alongside tokens.json.
  ├── imported-<garmin_user_id>.json             # PER ACCOUNT — { garmin_activity_id : { startTimeGMT, local_filename } }
  └── backfill-state-<garmin_user_id>.json       # PER ACCOUNT — { last_success_startTimeGMT, range_start, range_end }
```

`<garmin_user_id>` is the stable Garmin account identifier obtained from the SSO response (the library exposes it as `Garmin.display_name` or `Garmin.full_name_id`; we use the numeric ID for filename safety). It is recorded once at first-connect into `active-account.json` (**DEC-018 Option B**, 2026-07-19 — *not* `tokens.json`, whose schema is frozen by the REQ-006 permission-check and the REQ-007 `from_tokens`/`loadTokens` parse) so the per-account sidecar paths can be resolved before any further library call. Write ordering is `tokens.json` first, then `active-account.json`, so a crash between the two degrades to an empty uid (safe no-op) rather than a token file bound to the wrong active account. **[Prose corrected 2026-08-02 — VAL-016 check-5 WARN; the pre-DEC-018 text said `tokens.json`.]**

### Invariants

- Directory created with mode **`0700`** (POSIX) / owner-only ACL (Windows) on first write. Existing directory permissions are *not* tightened (avoid surprising the user).
- All files are written **0600** / owner-only via DES-006 atomic-writer.
- On read: if the file's mode/ACL is wider than owner-only, GC **refuses to load it**, raises a typed `TokenPermissionsRejected` (for `tokens.json`) or `SidecarPermissionsRejected` (for sidecars), surfaces a *specific* user-facing message via DES-008 (naming the file path and expected mode), and triggers a forced re-login (REQ-006). **[A2-005 fix]**
- `tokens.json` is **singular per athlete** — only one Garmin account is connected at a time. Disconnect deletes `tokens.json` **and `active-account.json`** (DEC-018/DEC-019; `GarminTokenStore::clearAccount`).
- Per-account sidecars (`imported-<uid>.json`, `backfill-state-<uid>.json`) are **all preserved across Disconnect** (active or otherwise). Reconnecting with the same account picks up its sidecar; reconnecting with a different account uses (or creates) that account's sidecar — prior-account sidecars are left untouched and not consulted by the active session. **[A2-006 fix, Option C]**
- The library's own default token path (`~/.garminconnect/tokens.json`) is **never** used. **[Corrected 2026-08-02 — VAL-016 check-5 WARN.]** The `tokenstore=` path this invariant originally prescribed was REMOVED by REQ-006 Slice B (`3edb705cb`, A3-R004-M3 security close): the adapter now constructs the library **auth-only** (`(email, password)`, exactly 2 args — TEST-015/016 pin it) and GC owns persistence entirely, writing the blob itself via `GarminTokenStore` so the 0600/owner-only invariant is never delegated to the library.

### Migration / multi-athlete / multi-account

- Two GC athletes on one install have fully independent `garminconnect/` directories.
- For a single GC athlete switching Garmin accounts (sell-watch-buy-new-watch scenario), each account gets its own sidecar files; cross-account `garmin_activity_id` collisions are structurally impossible. Old-account sidecars remain on disk as cold storage; a "manage previous-account data" UI is a Phase 1.5 follow-up.
- Phase 1 ship requires no migration (first release). A pre-A2 draft used singular `imported.json` and `backfill-state.json` — never released, so no migration code is needed in Phase 1.

---

## DES-003 — AddCloudWizard pages (credentials, MFA, CAPTCHA, ToS, backfill)

**Implements:** DEC-004, DEC-012 (seam to SSO layer).
**Serves:** REQ-001, REQ-002, REQ-003, REQ-009, REQ-014, REQ-015.

**Seam to SSO layer (DEC-012):** Each wizard page that talks to SSO (`GarminCredentialsPage`, `GarminMfaPage` [realized REQ-003 Slice B — see the "Realized MFA seam" note under DES-003a], and the future `GarminCaptchaPage`) takes an `IGarminAuthClient*` via its constructor (see **DES-003a** below). Pages never reference `GarminWorker` directly — this is what keeps the `garmin-fast` CTest label Python-free and lets the C++ page tests run in milliseconds without the embedded interpreter. In production the worker (DES-001) ships a concrete `WorkerAuthClient : IGarminAuthClient` adapter that forwards `authenticate(...)` to `enqueue(Authenticate{...})` and re-emits the worker's `finished`/`error` signals through the interface's signals.

### Page flow

```
  AddCloudWizard
    ├── (existing) ServiceTile selection
    │     ├── Strava / Dropbox / RWGPS / ... / Garmin Connect ◄── REQ-001
    │     └── on Garmin Connect selected:
    │
    ├── GarminToSPage                ─── REQ-009 (one-time notice, REQ-NF-i18n-001)
    │     ├── displays the verbatim REQ-009 text
    │     ├── "I understand — connect" / "Cancel"
    │     └── shown only on FIRST connect for this athlete
    │
    ├── GarminCredentialsPage         ─── REQ-002, REQ-005
    │     ├── email QLineEdit
    │     ├── password QLineEdit (EchoMode = Password, autocomplete off)
    │     ├── on Next: dispatches Authenticate request to DES-001
    │     └── on response:
    │         · success → wizard advances to backfill setup
    │         · mfa_required → push GarminMfaPage
    │         · captcha_detected → push GarminCaptchaPage
    │         · error → inline label via DES-008-translated message
    │
    ├── GarminMfaPage                 ─── REQ-003 (conditional)
    │     ├── 6-digit numeric QLineEdit (InputMask "999999")
    │     ├── Submit dispatches SubmitMfa(code); attempts counter
    │     ├── 3 failed attempts → abort with non-retry error + close wizard
    │     └── library does not invoke prompt_mfa → page never shown
    │
    ├── GarminCaptchaPage             ─── REQ-015 (conditional)
    │     ├── displays REQ-015 text
    │     ├── "Open Garmin in browser" → QDesktopServices::openUrl
    │     └── "Cancel" closes the wizard
    │
    └── GarminBackfillPage            ─── REQ-010, US-2
          ├── DateRange picker (default = today - 90d → today)
          ├── hard-cap 5y enforced by validator
          ├── "Skip" / "Import" buttons
          └── on Import: enqueue ListActivitiesSince + per-activity downloads
```

### Why pages, not modal dialogs

- Reuses `AddCloudWizard`'s existing Back / Next / Cancel / Help machinery — matches Strava/Dropbox precedent.
- Cancellation at any page consistently emits a `cancel()` to DES-001's worker.
- The conditional pages (MFA, CAPTCHA) are *pushed* onto the wizard's page stack at runtime via `setPage()` / `nextId()` overrides — they are not always-present pages.

### Optional profile auto-fill

A post-wizard checkbox dialog (Yes / No, defaults No) — wired to DES-011. Not a wizard page because it depends on the successful auth result and is a clearly separable step.

### Implementation note — REQ-002 closure slice (page-id wiring, 2026-07-05)

`AddCloudWizard`'s existing page-id scheme (01→10→15→20→25→30→90) gets a new
id **21**, reached only for the Garmin Connect tile: `AddService::nextId()`
and `AddConsent::nextId()` branch on `cloudService->id() == "Garmin Connect"`
to return 21 instead of the generic 20 (every other service's routing is
byte-for-byte unchanged). Page 21 is `AddGarminAuth`, a TU-local subclass of
`GarminCredentialsPage` (no new Q_OBJECT — it only overrides `nextId()` to
mirror `AddAuth`'s `hasAthlete ? 25 : 30` for an Activities-only service).

The wizard lazily builds the production stack on first entry to the Garmin
path (`AddService::clicked` / edit-mode ctor) via `ensureGarminAuthPage()`:
constructs a `PyEmbeddedAdapter` (DES-013) and a `GarminAuthChain` (the
DES-001/DES-001a thread+worker+adapter RAII assembly — new, see below), then
registers page 21 with `chain->client()`. The wizard owns both and destroys
them in `~AddCloudWizard()` in DES-001a order (chain/worker before adapter).
All of this is fenced under `#ifdef GC_WANT_GARMINCONNECT`; no other service's
code path is touched.

**`GarminAuthChain`** (`src/Cloud/GarminAuthChain.{h,cpp}`, TEST garmin:T-006)
is the piece DES-001/DES-001a described but didn't name: a small RAII class
owning the dedicated `QThread`, the `GarminWorker` (moved onto it), and the
`WorkerAuthClient`, given a non-owned `IGarminPyAdapter*`. Its destructor
performs DES-001 invariant 3 (`quit()` + bounded `wait()`, `terminate()` only
as a last resort so shutdown never hangs even if the worker is wedged).
Python-free by construction (the adapter arrives as an interface pointer), so
its test runs under `garmin-fast` against `FakePyAdapter` — the same fixture
TEST-004 already established.

`modulePath` resolves from env `GC_GARMIN_PYPATH` else the compile-time
`GARMIN_PY_MODULE_DIR` dev default (`src/Python/garminconnect`); the installed
path is DES-007/NF-Pkg-001 territory, not yet addressed. **Updated (REQ-006 Slice B,
`3edb705cb`):** `PyEmbeddedAdapter` is now constructed with `modulePath` ONLY — no
`tokenstorePath` is forwarded to the adapter or the library (auth-only, DEC-014 Option B).
C++ (GarminTokenStore) owns the token path and the single 0600 write; DES-002 owns the
directory layout and permission invariants under it.

**Known gaps (flagged for A3):** the wizard routing itself (page 21, the
nextId branches, chain lifecycle across Back/Next) has no automated test —
only `GarminAuthChain` in isolation does. `AddGarminAuth::nextId()`'s
`hasAthlete` branch (→25) is currently dead code (`GarminConnect` sets no
`AthleteID`), live only once a Garmin athlete-select slice exists.

---

## DES-003a — `IGarminAuthClient` interface (page ↔ SSO seam)

**Implements:** DEC-012.
**Serves:** REQ-002 (wizard-side acceptance), REQ-005 (wizard-side enforcement — pairs with the page's password-mask + IME-hints + post-submit-clear).
**Composes with:** DES-001 (concrete `WorkerAuthClient` adapter lives there), DES-003 (every page that needs SSO takes this interface), DES-008 (the `translatedMessage` field is DES-008-translated before reaching the page).

### Header location

`src/Cloud/IGarminAuthClient.h` — pure header-only contract. Must compile with `GC_WANT_GARMINCONNECT=OFF` (it is a contract, not an implementation; under flag-OFF nothing constructs it). No transitive Python dependencies; safe for the `garmin-fast` CTest label.

### Surface (RED-locked in TEST-003)

```cpp
// IGarminAuthClient.h
#pragma once
#include <QObject>
#include <QString>
#include <QUuid>

struct GarminAuthSuccess {
    QString garminUserId;     // stable Garmin account id (used by DES-002 per-account sidecars)
    QString displayName;      // human-readable, for the wizard's success page
};

struct GarminAuthFailure {
    enum Kind {
        Auth,                 // bad email/password
        Connection,           // network unreachable
        RateLimit,            // Garmin 429
        Captcha,              // CAPTCHA required — page should route to GarminCaptchaPage
        MfaRequired,          // page should push GarminMfaPage (REQ-003 slice)
        TokenPermissions,     // tokens.json mode/ACL refused
        Unknown,
    };
    Kind kind = Unknown;
    QString translatedMessage; // already DES-008-translated; pages surface verbatim
};

class IGarminAuthClient : public QObject {
    Q_OBJECT
public:
    explicit IGarminAuthClient(QObject* parent = nullptr) : QObject(parent) {}
    ~IGarminAuthClient() override = default;

    // Dispatch an SSO request. Async — never returns the result inline.
    // The requestId lets the page correlate the response (defence against
    // stale-response acceptance after the user re-edits credentials).
    virtual void authenticate(const QString& email, const QString& password, QUuid requestId) = 0;

signals:
    void finished(QUuid requestId, GarminAuthSuccess result);
    void failed(QUuid requestId, GarminAuthFailure err);
    // Extended in the REQ-003 / REQ-015 slices:
    //   void mfaRequired(QUuid requestId);
    //   void captchaDetected(QUuid requestId);
};
```

### Realized MFA seam (REQ-003 Slice A — D-R003-01)

The surface above sketched MFA as a `GarminAuthFailure::MfaRequired` *kind*. The
realized seam instead routes MFA-required through a **dedicated `mfaRequired(QUuid)`
signal** (added to both `IGarminAuthClient` and `GarminWorker`) plus a
`submitMfa(const QString& code, QUuid requestId)` dispatch method — so an
MFA challenge is neither a "success" nor a "failure" but its own outcome the
(Slice-B) page reacts to by pushing `GarminMfaPage`. `GarminAuthFailure::Kind`
was therefore left `{Auth, Network, Unknown}` and NOT grown with `MfaRequired`;
a *bad* OTP surfaces as an ordinary `kind=Auth` failure so the page re-prompts.
The commented `//   void mfaRequired(QUuid requestId);` in the sketch is now live.

### Why an interface, not a Qt-signal-only seam

DEC-012 weighed (A) injected interface, (B) page emits signals + wizard glues, (C) page holds `GarminWorker*` directly. (A) won on:
- compile-time guarantees (typos in the seam are build-breaks, not runtime no-ops);
- keeps `garmin-fast` CTest Python-free by construction (the page never transitively pulls `GarminWorker.h` or the embedded-Python headers);
- extends to MFA/CAPTCHA pages with no rework (REQ-003 / REQ-015 slices reuse the same fake).

### Lifecycle and ownership

The wizard creates a single `WorkerAuthClient` (GREEN) bound to the wizard's lifetime; each Garmin page receives the same pointer. The interface does *not* own the worker — destruction is the wizard's concern. In tests, the fake's lifetime is the test slot's.

### What is **not** specified by this interface

- Token persistence (DES-002 owns; the worker writes tokens *after* `finished` fires).
- Rate-limit / retry decoration (DES-005 / DES-012 own — at the Python layer; the interface sees only the post-retry outcome).
- Concurrency / GIL semantics (DES-001 owns).

These stay one level below the interface so the page tests need not stand any of them up.

---

## DES-004 — Cloud/GarminConnect: CloudService subclass

**Implements:** DEC-001 (Phase 1 staged ship), DEC-005 (capabilities), DEC-006 (file format), DEC-023 (explicit read-failure channel).
**Serves:** REQ-001, REQ-007, REQ-008, REQ-011, REQ-012, REQ-017, REQ-018.

### Download compression — the base default is WRONG for Garmin [added 2026-08-05, REQ-018 / B-R017-09]

`CloudService`'s ctor defaults `downloadCompression` to `zip` (CloudService.cpp:54), and `uncompressRide` rejects,
as its FIRST guard, any name not ending `.zip` (CloudService.cpp:239-241, `tr("expected compressed activity file.")`).
GarminConnect stages and lists UNCOMPRESSED names — `garmin-<id>.fit` and the DEC-016 `.tcx` fallback — so **both
ctors set `downloadCompression = none`** (GarminConnect.cpp:102, :110). Without it every *successful* download is
rejected at that guard and the whole feature is inert.

This shipped broken through REQ-007 and REQ-008 behind a fully green suite, because every Garmin test stopped at
`readFile`'s staging and none crossed into the consumer's `uncompressRide` call. TEST-067 is written specifically to
cross that boundary with real `test/rides/` payloads; a staging-only assertion does not satisfy REQ-018.

### Reporting a refusal — never a bare `return false` [added 2026-08-05, DEC-023 / REQ-017(a)]

`CloudServiceSyncDialog::syncNext`/`downloadNext` **discard** `readFile`'s bool and advance only on a completion
signal, so a bare `return false` is a HANG plus a leaked caller buffer, not a refusal (LSN-033). Every non-success
exit from `GarminConnect::readFile` therefore posts exactly one signal before returning:

- the two ENTRY guards (session superseded / account no longer connected) post `readComplete` with a labelled
  message (DEC-022) — **see the residual note below**;
- the five mid-flight exits (post-download discard FIT, RateLimit fast-fail, TCX-not-attempted, post-download
  discard TCX, neither-format) post `readFailed` with five DISTINCT reasons (DEC-023).

Both post QUEUED through `m_completionContext`, never synchronously — `readFile` runs inside a nested `QEventLoop`
and a synchronous emit is the A3-R007-01 use-after-free hazard.

**Recorded residual (B-R023-01):** the two entry guards still report via `postReadComplete` with an EMPTY payload —
which is the empty-payload heuristic DEC-023 exists to reject. `readFile` consequently has two mechanisms for one
job. Moving them onto `readFailed` is a small slice (it touches TEST-065's assertions), not a one-liner.

### Class shape

```cpp
class GarminConnect : public CloudService {
    Q_OBJECT
public:
    GarminConnect(Context *context);
    ~GarminConnect() override;

    QString id() const override { return "Garmin Connect"; }
    QString uiName() const override;       // tr("Garmin Connect")
    QString description() const override;  // tr-localized blurb
    int capabilities() const override {
        return Query | Download;           // REQ-011 — no Upload, no Sync, no OAuth, no Delete
    }
    int type() const override { return Activities; }

    // open/close are async-via-worker (DES-001):
    bool open(QStringList &errors) override;     // wires up worker; loads tokens; refreshes if needed
    bool close() override;                       // tears down worker; tokens persist

    // listing + download go through the worker:
    QList<CloudServiceEntry*> readdir(QString path, QStringList &errors,
                                      QDateTime from, QDateTime to) override;
    bool readFile(QByteArray *data, QString remotename, QString remoteid) override;
};
```

### Registration

- Static-init registers the class in `CloudServiceFactory` under id `"Garmin Connect"`, guarded by `#ifdef GC_WANT_GARMINCONNECT` (DES-007).
- `CloudServiceFactory::saveSettings()` and friends already handle per-athlete config persistence keyed on `id()` — we inherit that.

### Download format choice (DEC-006)

- `readFile()` calls `download_activity(id, dl_fmt=ORIGINAL)`. The library returns FIT for nearly all activities; for the small minority where Garmin returns non-FIT (some manually-entered activities), we fall back to TCX via `download_activity(id, dl_fmt=TCX)`.
- Bytes land in GC's existing import staging dir as `garmin-<activity_id>.<ext>`, where `<ext>` ∈ {`.fit`, `.tcx`}. The existing `FitRideFile` / `TcxRideFile` parsers produce the `RideItem`.

> **Download path (REQ-007) — design.** The chain below the CloudService is:
> `garmin_client.download_activity` (DES-012) → `PyEmbeddedAdapter` bytes-marshalling (DES-013) →
> `GarminWorker::downloadActivity` op emitting `downloaded`/`downloadFailed` (DES-001), with `fmt`
> forwarded verbatim so a caller may request `ORIGINAL` or `TCX`. `readFile()` stages the returned
> bytes as `garmin-<id>.<ext>` and applies the FIT→TCX fallback (DES-004); the fallback trigger
> ("FIT not available") depends on `python-garminconnect` behaviour that PRD Assumption B flags as
> unvalidated, and the path requires the worker-in-CloudService lifecycle + loaded tokens
> (REQ-004/006). Build/deployment status for these slices lives in the traceability matrix, not here.
>
> **DEC-016 resolution (Option C — the readFile retry table).** readFile requests ORIGINAL (FIT)
> via the worker, then: on `downloadFailed` — `RateLimited` fails fast (no retry, anti retry-storm);
> `Network`/`Unknown` retry once as TCX (the coarse library exception taxonomy makes `Network`
> legitimately conflate a real network failure with a 404 "no FIT original" — a documented LSN-006
> deviation). On success — the ORIGINAL payload is ZIP-wrapped, so readFile unzips (via `ZipReader`,
> `src/qzip`) and sniffs FIT magic (".FIT" at byte offset 8); FIT stages `garmin-<id>.fit`, anything
> else retries once as TCX (content-sniff backstop) and stages `garmin-<id>.tcx`.
>
> **Completion contract (readFile ↔ CloudService caller).** The CloudService auto-download caller
> connects `readComplete → loop.quit()` and starts a 30 s watchdog *before* calling readFile, then
> blocks on `loop.exec()`. readFile's `readComplete` notification MUST therefore be delivered *after*
> that event loop begins — a synchronous emit issued before `exec()` is ignored by Qt and wedges the
> caller to its timeout. The intended design is a queued/deferred completion (a self-posted notify on
> the CloudService QObject) so per-item download latency meets REQ-NF-Perf-003. See the traceability
> matrix / findings for which slice carries this.

### Session binding, failure channel, and store ownership (DES-014) [added 2026-08-05, DEC-021/023 + REQ-017(b)(e)]

Three contracts layer onto the seam below:

1. **Session binding (DEC-021).** `readFile`/`readdir` are gated by `sessionSuperseded()` (the in-memory epoch,
   DES-002) BEFORE DEC-020's `accountStillConnected()` disk re-check. The disk check is KEPT as a second layer —
   the epoch is not a replacement. REQ-017(a) is proven by neutralising `accountStillConnected()` and confirming the
   epoch alone still refuses; a fix that only works because the DEC-020 guard is present does not satisfy the clause.
2. **Explicit failure channel (DEC-023).** `CloudService` carries `readFailed(QByteArray* data, QString name,
   QString reason)` beside `readComplete`, with a `notifyReadFailed` helper mirroring `notifyReadComplete`. It
   exists because failure cannot be inferred from the existing `message` argument: **every** sibling service passes
   `tr("Completed.")` on success, so "message is non-empty" means success, not failure. The ~15 siblings never emit
   `readFailed` and are untouched — that is what bounds the blast radius. Both consumers connect it and each does the
   same three things: show the reason, free the buffer, advance the loop. `CloudServiceAutoDownload`'s blocking
   `QEventLoop` releases on `readFailed` as well as `readComplete` (CloudService.cpp:1944); without that, a refusal
   would merely relocate the hang.
3. **Store ownership (REQ-017 b/e).** An owner destroys what it opens. `closeAndDeleteStore(Store*& store)`
   (CloudService.h) clears the caller's pointer FIRST, then `close()`s, then deletes — that ordering IS the
   double-delete and re-entrancy guard. `~CloudServiceSyncDialog()` uses it (and `store->disconnect(this)` first, so
   a completion cannot land in a half-destroyed dialog); `MainWindow::syncCloud` and `uploadCloud` use it too.
   `WA_DeleteOnClose` is set at the HEAP call site (AddCloudWizard.cpp) and deliberately NOT in the dialog ctor —
   `syncCloud`'s dialog is stack-allocated and that would be a stack double-free.
   Because DEC-021's invalidation is lazy, this destructor is the guaranteed teardown trigger for an idle live
   session, which is what makes REQ-017 clause (b)'s functional reading honest.

### The download-client seam (DES-014)

readFile/open reach the worker through a dedicated download+restore seam (`IGarminDownloadClient`),
mirroring how the credentials page reaches auth through `IGarminAuthClient` (DES-003a). `GarminConnect`
owns a `PyEmbeddedAdapter` and a `GarminDownloadChain` (RAII: QThread + `GarminWorker` +
`GarminDownloadClient` around the non-owned adapter; bounded `quit()`+`wait()` teardown per DES-001
invariant 3, destruction order host(worker) before adapter per DES-001a). `open()` restores an
authenticated session from the stored token blob (`GarminTokenStore::loadChecked` →
`IGarminPyAdapter::loadTokens` → Python `GarminClient.from_tokens`, password-free per REQ-005), so no
password is ever needed for download. The seam and host are Python-free (LSN-007).

### Disconnect

- **[Corrected 2026-07-20, D-R008-01 / DEC-019]** There is NO `CloudService::removeSettings(id)` virtual — that surface never existed (the earlier prose assumed it). The real disconnect UI is `CredentialsPage::deleteClicked()` (src/Gui/AthletePages.cpp:143-161), which historically only flipped the active/sync appsettings flags and deleted no token (as do all sibling services). DEC-019 (Option C) adds a new generic `CloudService::disconnectService()` virtual (default no-op); `GarminConnect::disconnectService()` overrides it to delete `tokens.json` + `active-account.json` (DEC-018) via `GarminTokenStore::clearAccount(resolveConfigDir())`; `deleteClicked()` invokes it generically (`newService(id, context)->disconnectService()`, no Garmin special-case). **[Renamed 2026-07-20 per A3-R008-F4 / LSN-027 — `disconnect()` name-hid `QObject::disconnect()` across all ~15 CloudService subclasses; the shipped name is `disconnectService()`. Prose cascade completed 2026-08-02 (VAL-016 check-5 WARN).]** Per-account sidecars `imported-<uid>.json` + `backfill-state-<uid>.json` are **left alone** (REQ-012, DES-002). Persist-on-connect is the symmetric `CloudService::persistConnectSuccess()` virtual, driven by a single `AddCloudWizard` capture of both pages' id-gated `succeeded(GarminAuthSuccess)` signals (both direct + post-MFA).

---

## DES-005 — gc_rate.py: Python-side rate-limit + retry

**Implements:** DEC-007.
**Serves:** REQ-NF-Perf-002, REQ-NF-Reliab-001.
**Composes with:** DES-012 (decorators applied to adapter methods, not directly to the underlying library).

A small Python module shipped alongside `garminconnect`. Single chokepoint at the Python boundary; impossible to bypass from C++.

```python
# gc_rate.py
import functools, time, random

_BUCKET = {"last": 0.0, "min_interval": 1.0}   # REQ-NF-Perf-002 — 1 req/s

def rate_limited(fn):
    @functools.wraps(fn)
    def wrapped(*args, **kwargs):
        elapsed = time.monotonic() - _BUCKET["last"]
        wait = _BUCKET["min_interval"] - elapsed
        if wait > 0:
            time.sleep(wait)              # releases GIL — cancellation responsive
        try:
            return fn(*args, **kwargs)
        finally:
            _BUCKET["last"] = time.monotonic()
    return wrapped

def with_retry(fn, *, max_attempts=3, base=0.25, cap=2.0):
    @functools.wraps(fn)
    def wrapped(*args, **kwargs):
        attempt = 0
        while True:
            attempt += 1
            try:
                return fn(*args, **kwargs)
            except _TRANSIENT as e:               # REQ-NF-Reliab-001
                if attempt >= max_attempts:
                    raise
                delay = min(cap, base * (2 ** (attempt - 1)))
                delay += random.uniform(0, base) # jitter
                time.sleep(delay)
    return wrapped
```

`_TRANSIENT` is the narrow set of exceptions defined as transient: `GarminConnectConnectionError`, `requests.exceptions.Timeout`, `GarminConnectTooManyRequestsError` (429 with `Retry-After`). `GarminConnectAuthenticationError` is **not** transient — it surfaces immediately and triggers a forced re-login flow in DES-003.

### Composition

Decorators are applied to the **GC adapter** (DES-012), not the underlying library:

```python
# inside garmin_client.py (DES-012)
from gc_rate import rate_limited, with_retry

class GarminClient:
    login            = with_retry(rate_limited(_login_impl))
    list_activities_since = with_retry(rate_limited(_list_impl))
    download_activity     = with_retry(rate_limited(_download_impl))
    get_profile           = with_retry(rate_limited(_profile_impl))
    disconnect            = with_retry(rate_limited(_disconnect_impl))
```

Tests assert call-timing with a fake `time.monotonic` and `time.sleep`. Tests also assert that *every* public adapter method is wrapped — a method added without decoration would bypass the rate limit (regression-protected by a test that iterates `GarminClient.__dict__`).

---

## DES-006 — Atomic-write helper

**Serves:** REQ-NF-Reliab-002.
**Composes with:** DEC-003 (per-athlete storage layout — atomic writes are how DEC-003's invariants hold under crash).

Cross-platform tmp-and-rename helper used by DES-002 (tokens, sidecar, backfill state) and DES-009 (per-activity FIT file landings).

```cpp
// AtomicFile.h
class AtomicFile {
public:
    // Writes contents to <dest>.tmp, fsyncs, then renames over <dest>.
    // Returns false on any failure; never leaves a corrupted <dest>.
    static bool writeOver(const QString& dest, const QByteArray& contents,
                          QFileDevice::Permissions perms = QFileDevice::ReadOwner |
                                                          QFileDevice::WriteOwner);
};
```

### Cross-platform notes

- POSIX: `rename(2)` is atomic on the same filesystem. We assert tmp and dest share a parent dir.
- Windows: `MoveFileExW(..., MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH)` is the atomic equivalent; `QFile::rename` wraps this on Win32.
- `fsync` (POSIX) / `FlushFileBuffers` (Win) before rename — `QFile::flush()` + `fsync(handle())` covers both.
- Permissions are set on the tmp file **before** rename so the destination is never briefly world-readable.

### Read-side corruption handling

Callers (DES-002 for tokens, DES-009 for FIT files) catch parse failure and:
- For tokens / sidecar: emit error → force re-login or re-scan.
- For staged FIT files: discard, mark the activity for re-download on next sync.

---

## DES-007 — CMake feature flag + installer manifest

**Implements:** DEC-011.
**Serves:** REQ-NF-Build-001, REQ-NF-Pkg-001.

### CMake

```cmake
# CMakeLists.txt (top-level)
option(GC_WANT_GARMINCONNECT
       "Enable Garmin Connect cloud-service integration (Phase 1, read-only)"
       OFF)

if(GC_WANT_GARMINCONNECT)
    add_compile_definitions(GC_WANT_GARMINCONNECT)
endif()
```

```cmake
# src/CMakeLists.txt
if(GC_WANT_GARMINCONNECT)
    target_sources(GoldenCheetah PRIVATE
        Cloud/GarminConnect.cpp
        Cloud/GarminConnect.h
        Cloud/GarminWizardPages.cpp
        Cloud/GarminWizardPages.h
        Python/GarminWorker.cpp
        Python/GarminWorker.h
    )
endif()
```

When OFF: source files are not compiled, `CloudServiceFactory` never registers the tile, no Python-network-dep tests are run, installer omits `garminconnect` + `curl_cffi`.

### Installer manifest delta

- Win NSIS / macOS DMG / Linux AppImage: each installer's Python-bundle manifest gets two extra entries (the wheels for `garminconnect` and `curl_cffi`) when the build flag is ON.
- Phase 1: a **manual smoke checklist** in CONTRIBUTING runs `import garminconnect` from each installer on a clean VM (REQ-NF-Pkg-001 disposition).
- Phase 2 follow-up: CI smoke jobs per platform — tracked as a separate ticket, not gating Phase 1.

---

## DES-008 — Error translation + ErrorBus integration

**Serves:** REQ-014, REQ-NF-Obs-001.
**Composes with:** DEC-004 (wizard pages consume the translated messages), DEC-002 (worker emits errors that pass through here).

### Switch table

```cpp
// GarminErrors.cpp
static const QHash<QString, QString> kErrorMessages = {
    {"GarminConnectAuthenticationError",
        tr("Garmin Connect rejected your email or password. "
           "Please check and try again.")},
    {"GarminConnectTooManyRequestsError",
        tr("Garmin Connect is rate-limiting GoldenCheetah. "
           "Sync will retry automatically; please wait a moment.")},
    {"GarminConnectConnectionError",
        tr("Couldn't reach Garmin Connect. Check your internet connection and try again.")},
    {"CaptchaRequired",
        tr("Garmin Connect is asking for a CAPTCHA. "
           "Please log in once at https://connect.garmin.com in your browser, then try again here.")},
    {"MfaRequired",
        tr("Garmin Connect needs a verification code.")},  // not user-facing as error; triggers MFA page
    {"TokenPermissionsRejected",
        tr("GoldenCheetah refused to load Garmin tokens because the file '%1' "
           "is readable by other users. Expected owner-only (mode 0600). "
           "Please re-enter your Garmin credentials; GoldenCheetah will rewrite the file with the correct permissions. "
           "If this keeps happening, check whether a backup or sync tool is widening file permissions.")},  // [A2-005 fix]
    {"SidecarPermissionsRejected",
        tr("GoldenCheetah refused to load the Garmin sync history file '%1' "
           "because it is readable by other users. Expected owner-only (mode 0600). "
           "The file will be rebuilt on the next sync.")},  // [A2-005 fix]
};
QString translate(const QString& exceptionName, int httpStatus = 0,
                  const QString& contextArg = QString()) {
    auto it = kErrorMessages.find(exceptionName);
    if (it != kErrorMessages.end()) {
        return contextArg.isEmpty() ? *it : it->arg(contextArg);
    }
    return tr("Connection to Garmin Connect failed (%1).").arg(exceptionName);
}
```

`contextArg` carries the offending file path for the permission-rejection cases; the existing translations use it via `%1`. Callers in DES-002 pass the path. **[A2-005 fix]**

### ErrorBus events

Every sync operation emits, via the existing `ErrorBus` channel (the same one Strava/Dropbox use):

```cpp
ErrorBus::emit({
    .service = "Garmin Connect",
    .op = "sync_incremental",                    // or sync_backfill, auth, refresh, download
    .severity = ErrorBus::Severity::Info|Warn|Error,
    .duration_ms = ...,
    .activity_count = ...,
    .garmin_error_code = "GarminConnectTooManyRequestsError",  // empty on success
    .message = translate(...),
});
```

Structured `qDebug` mirrors the same fields for developer trace logging (REQ-NF-Obs-001).

---

## DES-009 — Bulk backfill controller

**Serves:** REQ-010, REQ-NF-Cancel-001.
**Composes with:** DEC-002 (runs inside the worker), DEC-003 (per-account state files), DEC-007 (paces calls through the rate limiter).

### State machine

```
        Idle
         │ start(range_start, range_end)
         ▼
   Paging ───── 20 activities/page, 1s inter-page delay (DES-005 enforces)
         │
   ┌─────┴─────┐
   ▼           │
 Per-activity │ for each activity in page:
   download    │   1. download_activity(...) via worker
   (atomic)    │   2. parse → produce RideItem
   ──────────► │   3. write per-activity FIT via DES-006 atomic-writer
               │   4. update backfill-state.json (DES-006) with this startTimeGMT
               │   5. update imported.json (DES-006) with mapping
               │   6. check cancellation flag
               │
   ┌───────────┘
   ▼
 Page exhausted? → next page (1s delay)
 Range exhausted? → Done
 Cancelled?      → Paused (state file preserved)
 Transient err?  → retry per DES-005; if exhausted → Paused + ErrorBus error
```

### Resume behaviour

On `start()`, if `backfill-state-<uid>.json` exists for the **currently-connected Garmin account** and `last_success_startTimeGMT` falls inside the requested range, paging *starts from* that timestamp instead of `range_start`. The user is told via a small banner: "Resuming backfill from <date>". A backfill state file for a *different* Garmin account is ignored (sits on disk untouched).

### Four interruption classes (REQ-010 acceptance)

| Class | Detection | Recovery |
|-------|-----------|----------|
| User cancel | cancellation flag set | Paused. UI shows resume / restart options on next launch. |
| Hard crash / SIGKILL | next-launch sees `backfill-state-<uid>.json` with no Done marker | Same as cancel — resume offered. |
| Transient HTTP error | DES-005 retry exhausted | Paused + ErrorBus error. User can retry manually. |
| Torn FIT write | DES-006 read-side detects parse failure | Activity discarded; re-fetched on resume. |

### Cancellation latency caveat

Cancellation is checked at DES-005 sleep release + the per-activity loop head. If the *per-activity parse* (inside GC's existing `FitRideFile`) takes ≥10s on an unusually large file (e.g., a 24h ultra), cancellation will not interrupt that parse — the user-perceived cancel latency is up to one parse-cycle. Accepted as out-of-scope (not a new code path; same parse latency that applies to all FIT imports). **[A2-007 disposition]**

### Empty result is success

A `ListActivitiesSince` returning zero activities for the range is a normal success state, *not* an error (PRD REQ-010 acceptance).

---

## DES-010 — Incremental sync flow

**Serves:** REQ-008, REQ-NF-Perf-003.
**Composes with:** DEC-002 (runs inside the worker), DEC-003 (per-account sidecar), DEC-006 (FIT staging path).

### Trigger sources

- User clicks "Sync" on the CloudService settings page.
- GC's existing auto-sync scheduler triggers (same hook used by other CloudServices).

### Flow

```
1. Check: any sync already in progress? → reject with "sync already in progress" (REQ-NF-Perf-002)
2. Resolve the active garmin_user_id from tokens.json.
3. Read last_success_ts from backfill-state-<uid>.json — default to now() - 7d if absent.
4. Worker: adapter.list_activities_since(last_success_ts)
5. For each returned activity:
     a. Look up garmin_activity_id in imported-<uid>.json (DES-002 active-account sidecar)
        - Found → skip (short-circuit, no download)
     b. Else → adapter.download_activity(id, 'ORIGINAL')
     c. Stage as garmin-<id>.<ext> in GC's import staging dir (DES-004)
     d. GC's existing import path produces RideItem; RideCache dedupe
        catches any second-line collision (defense in depth).
     e. Update imported-<uid>.json (DES-006 atomic-writer)
6. Update last_success_ts in backfill-state-<uid>.json.
7. ErrorBus success event with count + duration.
```

### Dedup keys (two-tier, REQ-008)

- **Tier 1 (fast):** `imported-<uid>.json[garmin_activity_id] → startTimeGMT, local_filename`. Avoids any re-download. The sidecar is *per active Garmin account* — entries from previously-connected accounts are not consulted, so cross-account `garmin_activity_id` collisions cannot cause silent skips. **[A2-006 fix, Option C]**
- **Tier 2 (authoritative):** GC's `RideCache` dedup, which matches on filename/timestamp/duration the same way it does for any file import. This is the safety net if the active-account sidecar is missing or torn.

`startTimeGMT` is **Garmin's server-side timestamp**, not the local clock — protects against clock-skew duplicates if the user crosses time zones mid-sync.

---

## DES-012 — `garmin_client.py`: stable adapter over `python-garminconnect`

**Serves:** REQ-002, REQ-007, REQ-008, REQ-013, REQ-014, REQ-NF-Compat-001.
**Composes with:** DEC-002 (worker is the sole caller), DEC-007 (rate-limit/retry decorators decorate adapter methods, not the underlying library); DES-001 (worker calls only the adapter), DES-005 (decorators applied to adapter methods), DES-008 (error switch table keyed on adapter-translated `kind` values, not raw library exceptions).
**Origin:** A2-004 (migration trap).

A thin Python module that wraps `garminconnect.Garmin` and exposes a GC-stable interface. **All other Python code in this feature calls only the adapter; nothing else imports `garminconnect` directly.** Swapping the underlying library (e.g., to a community fork if upstream goes unmaintained) becomes a one-file change to the adapter.

### Surface

```python
# garmin_client.py
from typing import Iterator, Optional
import garminconnect as _gc        # the only import of the underlying library

class GarminError(Exception):
    """GC-stable error type. The .kind field is one of:
       'auth' | 'rate_limit' | 'connection' | 'captcha' | 'mfa_required' |
       'token_permissions' | 'unknown'."""
    def __init__(self, kind: str, message: str, original: Optional[Exception] = None):
        self.kind = kind
        self.message = message
        self.original = original
        super().__init__(message)

class GarminClient:
    def __init__(self, email: str, password: str): ...    # auth-only (DEC-014 Option B / REQ-006 Slice B 3edb705cb); no tokenstore path
    def login(self) -> dict: ...                          # returns {'garmin_user_id': '...', 'display_name': '...'}
    def submit_mfa(self, code: str) -> dict: ...          # same return shape
    def list_activities_since(self, ts_gmt: str) -> Iterator[dict]: ...
    def download_activity(self, activity_id: str, fmt: str = 'ORIGINAL') -> bytes: ...
    def get_profile(self) -> dict: ...
    def disconnect(self) -> None: ...                     # forgets in-memory state; file deletion is C++-side
```

### Translation responsibilities

The adapter catches every `garminconnect.*` exception and re-raises as `GarminError(kind=...)`. The mapping table lives **inside the adapter**:

```python
_EXCEPTION_MAP = {
    _gc.exceptions.GarminConnectAuthenticationError:    'auth',
    _gc.exceptions.GarminConnectTooManyRequestsError:   'rate_limit',
    _gc.exceptions.GarminConnectConnectionError:        'connection',
    # CAPTCHA + MFA detection are heuristic — adapter inspects the exception
    # message / response body and produces 'captcha' / 'mfa_required'.
}
```

DES-008's switch table is keyed on the **GC-stable `kind` values**, not on `garminconnect`'s concrete class names. Swapping `garminconnect` → `garth` or a fork means rewriting `_EXCEPTION_MAP` and the per-method impl — nothing in C++ changes, nothing in DES-005's decorator setup changes, no test assertion against a library-specific class name changes.

### What lives where

| Concern | Lives in | Why |
|---------|----------|-----|
| Concrete library import | `garmin_client.py` only | Single point of swap. |
| Rate-limit + retry decoration | DES-005, applied to adapter methods | Decorator setup doesn't touch the library directly. |
| Error-class name translation | `_EXCEPTION_MAP` in the adapter | C++ never sees the library's exception names. |
| Token-store path | C++ (GarminTokenStore) owns the path + the single 0600 write; adapter forwards NONE to the library (auth-only, DEC-014 Option B / REQ-006 Slice B) | library self-writes no token file. |
| MFA / CAPTCHA detection | Adapter (heuristic over exception message + HTTP body) | Library doesn't surface these cleanly; we centralize the heuristics. |

### Tests (anticipated, Phase 2)

- Construct adapter against an in-process mock library with `monkeypatch`; assert each `kind` is produced for the expected exception.
- Assert `_EXCEPTION_MAP` is exhaustive over the library's currently-known exception subclasses (via introspection) — catches the case where a new library version adds exceptions we don't translate.
- Adapter-internal decoration test: every public method on `GarminClient` is wrapped by `rate_limited` (DES-005 regression-protection test, but lives here).

### Migration cost (revisited)

With the seam in place, the swap-library cost drops from "5 Python files + 1 C++ file + test churn" to "1 file (the adapter) + the new library's import" — independent of how many call sites accrue in Phase 2/3.

### DEC-014 refinement (2026-07-11) — token persistence moves to C++ (REQ-004/006)

Under DEC-014 (Option B) the adapter, not the library, owns the disk boundary:
`GarminClient` is constructed **auth-only** (no `tokenstore` path). **REQ-006 Slice B status (2026-07-11): DONE.**
`GarminClient.__init__(email, password)` constructs `_gc.Garmin(email, password)` (C-API `"ss"`);
`PyEmbeddedAdapter(modulePath)` forwards no path (findings B-R004-01 + A3-R004-M3 RESOLVED). With no
tokenstore path handed to it, the library self-writes no token file — REQ-NF-Sec-002 is end-to-end MET,
the C++-owned 0600 write (GarminTokenStore, Slice A perm-checks its load) being the sole token file.
REQ-004 landed the two methods so C++ can persist the blob via `AtomicFile` at 0600 (DES-002/DES-006):

```python
def dump_tokens(self) -> str: ...        # export the authenticated session as an opaque blob
def load_tokens(self, token_str: str) -> None: ...  # restore a session on resume, in place of password login
```

`dump_tokens()` wraps the library's in-memory string export (`dumps()` on the current
`python-garminconnect` native engine; **OQ1**: confirm the exact name against the bundled wheel —
tests pin the contract via the pystub, so this is not GREEN-blocking). `load_tokens()` failure when
Garmin has invalidated the session server-side raises `GarminError(kind='session_expired')`
(**OQ2**, mapped to REQ-NF-Compat-001(b) "prompt full re-login") — distinct from REQ-006's
`token_permissions` rejection so DES-008 messages the two differently. Tests: +2 pytest
(round-trip dump→load; reject-tampered-blob) — garmin:T-013.

---

## DES-011 — Optional profile auto-fill

**Serves:** REQ-013.
**Composes with:** DEC-004 (wizard post-flow), DEC-002 (FetchProfile request runs on the worker).

Post-connect dialog, shown once on first successful connect, **defaults off**:

```
┌─ Use Garmin profile data to fill in your Athlete profile ────┐
│                                                                │
│  ☐ Yes, use my Garmin profile to fill missing GC fields       │
│                                                                │
│  GoldenCheetah will only fill fields that are currently empty.│
│  Your existing data will not be changed.                       │
│                                                                │
│                              [ Skip ]   [ Apply ]              │
└────────────────────────────────────────────────────────────────┘
```

If checked + Apply:
- Worker: `FetchProfile` → returns `{dob, weight_kg, height_cm, hr_max, ftp_w (if any)}`.
- For each field: if the matching `Athlete` field is **empty/unset**, fill it. Otherwise no-op.
- No overwrite of user-provided data; not retried on subsequent connects.

---

## DES-013 — `PyEmbeddedAdapter`: embedded-CPython bridge (production IGarminPyAdapter)

**Implements:** production side of DEC-013 (the seam's real implementation); executes under DEC-002's worker-thread model.
**Serves:** REQ-002 (production closure of the Authenticate slice), REQ-006 (auth-only construction — STOPS forwarding the tokenstore path; DEC-014 Option B), and — additively — every later worker op (REQ-003/007/010/012/014).
**Composes with:** DES-001 (sole caller is the worker, on the worker thread), DES-001a (implements `IGarminPyAdapter`), DES-012 (calls only `garmin_client.GarminClient`, never `garminconnect`), DES-008 (returns raw kinds + raw messages; no translation).

### Shape

`src/Cloud/PyEmbeddedAdapter.{h,cpp}`. The **header is Python-free** (no `Python.h`,
no interpreter symbols — same invariant as `IGarminPyAdapter.h`); only the `.cpp`
includes `Python.h`. Compiled into the app **only when `GC_WANT_GARMINCONNECT=ON`**,
and links CPython (`Python3::Python`) — the flag gains a Python link dependency here,
which DES-007's flag section anticipated.

```cpp
class PyEmbeddedAdapter : public IGarminPyAdapter {
  public:
    // modulePath: directory prepended to sys.path so `garmin_client` resolves
    //             (C++ owns path policy, per DES-012 "what lives where").
    // AUTH-ONLY (DEC-014 Option B / REQ-006 Slice B, 3edb705cb): no tokenstore
    // path — the adapter constructs GarminClient(email, password) so the library
    // self-writes no token file; C++ owns the 0600 write (GarminTokenStore).
    explicit PyEmbeddedAdapter(const QString& modulePath);
    ~PyEmbeddedAdapter() override;                 // REQ-007: DECREF m_client under GIL if Py up
    PyAuthOutcome authenticate(const QString& email, const QString& password) override;
    PyDownloadOutcome downloadActivity(const QString& activityId, const QString& fmt) override; // REQ-007
  private:
    PyObject* m_client = nullptr;                  // retained authenticated GarminClient; owned
};
```

The header stays Python-free: `m_client` is declared via an opaque `struct _object;`
forward-decl (`using PyObject = _object;`), so no `Python.h` leaks into includers.
Non-copyable (owns a `PyObject*`).

### `authenticate()` sequence (worker thread)

1. `Py_IsInitialized()` false → return `Unknown` / `"embedded Python unavailable"`
   (fail-safe; matches DES-001 "init failure → capabilities 0"; never throws).
2. `PyGILState_Ensure()` via RAII guard — released on **every** exit path.
3. Prepend `modulePath` to `sys.path` if absent; `import garmin_client`.
4. `GarminClient(email, password)` → `.login()` (AUTH-ONLY, C-API `"ss"` — REQ-006 Slice B).
5. Success dict → `PyAuthOutcome{Success, garmin_user_id, display_name}`.
6. Exception → classify **by type then kind, never by message** (LSN-006 / A3-R002-M6):
   - `garmin_client.GarminError` → read `.kind`: `'auth'`→`AuthFailed`,
     `'connection'`→`Network`, anything else (`'rate_limit'`, `'captcha'`, …)
     →`Unknown` until its owning slice extends the enum. `.message`→`rawMessage`.
   - any other Python exception → `Unknown` + `str(e)`. **Never** mapped to
     `AuthFailed`.

On success (step 5) the client PyObject is **retained** in `m_client` (prior ref
DECREF'd) so `downloadActivity()` can reuse the session — REQ-005 keeps no password,
so a fresh per-download client is impossible.

### `downloadActivity()` sequence (REQ-007, worker thread)

1. `Py_IsInitialized()` false → `Unknown` / `"embedded Python unavailable"` (same
   fail-safe as authenticate; never throws).
2. `PyGILState_Ensure()` via the same RAII guard.
3. `m_client == nullptr` (no prior successful auth) → `Unknown` / `"not authenticated"`.
4. `m_client.download_activity(activityId, fmt)` (positional `"ss"`; `fmt` is the
   GC-stable string, mapped to the library enum inside `garmin_client` — DES-012).
5. Exception → classify **by type then kind** (shared `takeRaisedException` helper,
   same discipline as authenticate step 6): `GarminError` `'connection'`→`Network`,
   `'rate_limit'`→`RateLimited`, anything else / foreign →`Unknown`; `.message`→`rawMessage`.
6. Non-`bytes` result → `Unknown` / "returned a non-bytes result" (DES-012 contract
   breach, never a spurious `Success`).
7. `bytes` result → `PyDownloadOutcome{Success, data}` via `PyBytes_AsStringAndSize`
   (ptr,len copy → **binary-safe**, keeps embedded NUL; a strlen copy would truncate).

The **FIT→TCX fallback is NOT here** — DES-004's `readFile` (slice 3) calls this once
with `"ORIGINAL"` and, on a non-`Success`/empty result, again with `"TCX"`. This method
is a thin single-format fetch.

### Interpreter topology (scope note)

Phase-1 slice uses `PyGILState_Ensure` against the **main interpreter** (the
PythonEmbed host's). Per-feature sub-interpreter isolation is the A2-001 /
Phase-1.5 hardening follow-up — the adapter's surface does not change when it
lands, so this is deliberately not blocking VAL-007.

### What this design does NOT cover

- Threading (DES-001 — the worker guarantees non-GUI-thread invocation).
- Retry/rate-limit (DES-005 — Python-side decorators, invisible here).
- Token file *layout* (DES-002 owns). **DEC-014 refinement (REQ-006 Slice B, `3edb705cb`):** this class is now
  constructed AUTH-ONLY — `PyEmbeddedAdapter(modulePath)` (1-arg), C-API `"ss"`, forwarding NO `tokenstorePath`
  to the library, so the library self-writes no token file (findings B-R004-01 + A3-R004-M3 RESOLVED;
  REQ-NF-Sec-002 end-to-end MET). On `Success` the class surfaces the session blob via `PyAuthOutcome.tokenBlob`
  (calling `dump_tokens()`); C++ (GarminTokenStore) owns the single atomic 0600 write, and its load-side
  refuse-on-wider-than-owner check (loadChecked → TokenPermissionsRejected, TEST-014) is the sole token-file
  boundary. On resume the adapter calls `load_tokens(blob)`. The retained `m_client` (REQ-005/REQ-007 session)
  is unchanged. Tests: garmin:T-014 (load-side), T-015/T-016 (auth-only construction).
- Message translation (DES-008 — page layer).

### Tests (TEST garmin:T-005)

New CTest executable `testGarminConnectPyAdapter`, label **`garmin-py`** (needs a
real linked CPython — must NOT carry `garmin-fast`). It initializes CPython in
`initTestCase`, points `modulePath` at a **scriptable stub** `garmin_client.py`
fixture (`unittests/Core/garminconnect/pystubs/`), and asserts the marshalling
matrix: success-dict mapping, `kind='auth'`→`AuthFailed`, `kind='connection'`→
`Network`, `kind='rate_limit'`→`Unknown` (not `AuthFailed`), `ValueError`→
`Unknown` (not `AuthFailed`), missing-module→`Unknown`, and GIL-balance across
repeated calls. The *real* `garmin_client.py` behavior stays covered by its own
pytest suite (`src/Python/garminconnect/tests/`) — two seams, tested on their
own sides, meeting at the DES-012 contract.

---

## Failure modes

| Component | Failure mode | Blast radius | Mitigation |
|-----------|--------------|--------------|------------|
| DES-001 worker | Embedded Python uncaught exception | Single request fails; worker survives | Top-level try/except in worker → GcError via `error()` signal. |
| DES-001 worker | Sub-interpreter init failure | Feature degrades to disabled until process restart | `capabilities()` returns 0; tile is non-functional; logged via DES-008. |
| DES-001 worker | Cancellation request arrives mid-Python-call | In-flight call completes; subsequent skipped | Cancellation flag checked at DES-005 sleep release + DES-009 loop head (REQ-NF-Cancel-001 acceptance). |
| DES-001 worker | Sub-interpreter wedges (deadlock / native crash / GIL stuck) | Feature non-functional until process restart | Phase 1: detect via UI report; user restarts GC. Phase 1.5 follow-up: thread-heartbeat with kill-and-recreate. **[A2-001]** |
| DES-009 backfill | Per-activity parse > 10 s blocks cancellation | Up to one-parse-cycle of cancel latency | Accepted — same parse latency that applies to all FIT imports; not new code path. **[A2-007]** |
| DES-002 / DES-010 sidecar | Cross-account `garmin_activity_id` collision after Disconnect-and-reconnect-with-different-account | Per-account sidecar files prevent stale entries from being consulted | Each Garmin account has its own `imported-<uid>.json`; active session reads/writes only the active account's file. **[A2-006 fix]** |
| DES-002 sidecar | JSON file grows to 10–50 MB for long-history users | Sync time degrades from <5s to 10–30s | Phase 1: surfaced via observability signals (sidecar-read-time, entry-count, file-size). Phase 1.5 follow-up: migrate to sqlite when watch threshold breached. **[A2-003]** |
| DES-002 tokens.json | Torn write (crash mid-write) | Tokens lost; user re-logs in | DES-006 tmp-and-rename; read-side parse failure → forced re-login. |
| DES-002 tokens.json | File permissions widened externally | Tokens not trusted | Refuse-on-mismatch + forced re-login (REQ-006). |
| DES-002 imported.json | Torn write or corruption | Possible re-download of already-imported activities (but RideCache dedup catches the second-line) | Two-tier dedup (DES-010); torn read falls back to "treat as empty" + rebuild on next sync. |
| DES-005 rate limit | Garmin returns 429 despite our 1 req/s | Sync paused; ErrorBus error | Exponential backoff respects `Retry-After`; max 3 retries then surface (REQ-NF-Reliab-001). |
| DES-005 rate limit | `_BUCKET` module-level state lost on sub-interpreter restart | Brief burst possible | Negligible — sub-interpreter doesn't restart per-request; first call after restart sees `last=0` and proceeds without wait, which is acceptable. |
| DES-006 atomic-writer | Disk full | Write fails atomically; no corruption | Returns false; caller surfaces via ErrorBus. |
| DES-006 atomic-writer | Cross-FS tmp + dest (NFS-style mounts) | `rename` not atomic | Assert same-FS in debug; documented in CONTRIBUTING that athlete-config-dir must be on a single FS. **[Accepted residual risk — same as other CloudServices]** |
| DES-007 build gate | User builds with flag OFF on a machine with stale Python deps installed | Nothing — flag-OFF code path doesn't import them | Verified by REQ-NF-Build-001 acceptance test. |
| DES-008 error translation | Unknown Garmin error code | Generic fallback message + raw code shown | Acceptable — informative enough; periodic table update from beta reports. |
| DES-009 backfill | `backfill-state.json` from a previous range overlaps with new range | Resume picks up from prior `last_success_ts` (may skip past new range_start) | Acceptable: documented as "completing the previous backfill first"; user can clear state via a "Restart backfill" button. |
| DES-010 incremental | Garmin returns activities out of `startTimeGMT` order | Sidecar may record activities out-of-sequence | Each per-activity write is atomic; the `last_success_ts` is updated to the *max* startTimeGMT processed, not the last-in-the-loop. |
| All paths | Network outage during sync | Sync fails; partial progress preserved | Retry per DES-005 then pause; no data loss. |

---

## Threat model (STRIDE, scoped to Phase 1)

| Threat | Asset | Vector | Mitigation |
|--------|-------|--------|------------|
| **S**poofing | User's Garmin account | DNS hijack / MITM impersonates `connect.garmin.com` | HTTPS via `curl_cffi` against OS trust store (REQ-NF-Sec-003). `verify=False` forbidden, enforced by grep audit. |
| **T**ampering | tokens.json on disk | Crash mid-write → corrupted file → load fails open with empty creds | Atomic tmp-and-rename (DES-006) + read-side validation. |
| **T**ampering | Token replay by same-user malware | Another process running as same user reads `tokens.json` | File permissions 0600/owner-only (REQ-NF-Sec-002). **Residual risk: same-user malware** — documented in README + first-connect notice (REQ-NF-Sec-004). OS-keychain integration left to Phase 1.5 if beta surfaces concerns. |
| **R**epudiation | N/A — single-user desktop tool, no audit-log requirement | — | Out of scope. |
| **I**nformation disclosure | Garmin password | Held in memory during SSO; never persisted (REQ-005). | Zeroed after SSO completes; not logged; not written to ErrorBus. |
| **I**nformation disclosure | Activity contents in staging dir | Same-user read access | Acceptable — activity files already live in GC's existing staging dir with the same exposure as Strava/Dropbox/etc. |
| **D**enial of Service | Garmin temporarily restricts the account due to suspected scraping | Aggressive request rate | 1 req/s rate limit (REQ-NF-Perf-002) + first-connect ToS notice (REQ-009) sets user expectation. Success metric: 0 reported suspensions. |
| **D**enial of Service | Bug in worker spawns infinite request loop | High request rate → potential Garmin restriction | Single-worker chokepoint + queue bound (DES-001) + rate-limiter (DES-005) cap the worst-case throughput. |
| **E**levation of privilege | Embedded Python extends GC's process privileges | Python loads native modules with capabilities GC lacks | No new privileges — same uid as GC process. Threat applies to the existing PythonEmbed runtime as a whole; no new surface introduced here. |

---

## Observability plan

| Signal | Source | Channel | Alert? | Threshold / target |
|--------|--------|---------|--------|---------------------|
| Sync started | DES-010 / DES-009 | `qDebug` structured + ErrorBus Info | No | — |
| Sync succeeded | DES-010 / DES-009 | `qDebug` + ErrorBus Info | No | — |
| Sync error (transient) | DES-005 retry exhausted | ErrorBus Warn | No | — |
| Sync error (auth) | DES-008 translation | ErrorBus Error | No | — |
| Connect-attempt success/fail | DES-003 wizard | `qDebug` | No | success-metric target ≥80% beta |
| GIL stall > 100 ms | dev-build frame-time monitor | `qDebug` | Dev-build only | REQ-NF-Threads-001 — any occurrence is a bug |
| Rate-limit wait | DES-005 | `qDebug` (debug build) | No | informational |
| Backfill progress | DES-009 → UI signal | progress() Qt signal | No | UI label |
| Per-activity download duration | DES-001 worker timing | `qDebug` structured | No | informational |
| Garmin 429 (TooManyRequests) | DES-005 retry caught | ErrorBus Warn | No | watched in beta — non-zero count is a signal |
| Sidecar read time + entry count | DES-002 on every sync | `qDebug` structured | No | **[A2-003 fix]** beta-watch metric: read-time > 500 ms or entry-count > 10 000 is the early-warning signal for the sqlite-sidecar Phase 1.5 trigger |
| Sidecar file size | DES-002 on every write | `qDebug` structured | No | **[A2-003 fix]** same trigger; size > 5 MB is the watch threshold |
| Token-permission rejection | DES-002 → DES-008 | ErrorBus Error | No | **[A2-005 fix]** non-zero in steady state suggests a backup tool widening files; surface to user with the specific message |
| Wedged sub-interpreter (Phase 1.5) | DES-001 heartbeat | not yet implemented | n/a | **[A2-001 → Phase 1.5]** |

**Aggregation:** Phase 1 has no centralized telemetry — observability is local-only (logs + ErrorBus). Phase 2+ may add opt-in anonymized metrics once we know what to look at; that's a separate DEC.

---

## Phase 1.5 follow-up backlog (non-blocking, tracked here for visibility)

These are referenced by REQs/dispositions but explicitly *not* built in Phase 1; they are not DES entries because they don't exist yet:

- OS-keychain integration for token storage (REQ-NF-Sec-004 residual-risk follow-up if beta surfaces concrete concerns).
- CI smoke jobs per platform asserting `import garminconnect` works inside the installed bundle (REQ-NF-Pkg-001 Phase 2 step).
- "Restart backfill" button — Phase-1.5 nice-to-have flagged in DES-009.
- **Sub-interpreter heartbeat + kill/recreate** — [A2-001] follow-up to DES-001 failure mode.
- **Sqlite-backed sidecar** — [A2-003] triggered when beta observability shows sidecar-read-time > 500 ms or entry-count > 10 000 on real users.
- **"Manage previous-account data" UI** — [A2-006] follow-up for users with multiple Garmin accounts who want to delete old sidecars from disk.
