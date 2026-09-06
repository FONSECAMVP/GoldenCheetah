/*
 * Copyright (c) 2026 GoldenCheetah Contributor
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the Free
 * Software Foundation; either version 2 of the License, or (at your option)
 * any later version.
 */

// TEST garmin:TEST-143 - arming order / boundedness, 22 sites.
// TEST garmin:TEST-144 - false-success prevention, 22 sites.
// TEST garmin:TEST-145 - exactly-once ACTUAL reply destruction, 22 sites.
// TEST garmin:TEST-146 - BUILT IN FULL. Transition-table rows 1-7, including
//                        row 4 (cancellation, no finish observed) and row 6's
//                        Cancelled arm (an abort induced by cancellation, not
//                        timeout, whose synthetic finished() must not promote
//                        the outcome). DEC-040 Stage 2 wired CancelToken onto
//                        CloudService and blockingRequest's cancelPoll timer,
//                        so RequestOutcome::Cancelled is now reachable and
//                        these two rows exercise it directly.
// TEST garmin:TEST-147 - BUILT IN FULL. TIMEOUT propagation across the
//                        shape-B/C set (shapeBC_timeoutBreaksTheListing), and
//                        its Cancelled-mid-listing counterpart, Stage 2 piece 3
//                        (shapeBC_cancellationBreaksTheListing), over the same
//                        seven sites.
// TEST garmin:TEST-149 - Stage 2 cancellation genericity + lifetime safety:
//                        the observation interval is <= 500 ms with the
//                        timeout override at 5000 ms, so a real timeout cannot
//                        be what ends the wait.
// TEST garmin:TEST-150 - non-vacuity control: a RESPONDING endpoint yields
//                        Finished with the correct body at every migrated site.
// TEST garmin:TEST-151 - error propagation at CloudServiceAutoDownload::run()'s
//                        discarded-readdir-errors branch.
// TEST garmin:TEST-152 - Strava::addSamples completion-emission + buffer
//                        ownership.
// TEST garmin:TEST-153 - S-1 manager lifecycle across all THIRTEEN migrated
//                        providers.
//
// TEST garmin:TEST-148 - Stage 2 piece 3b: the three GUI-thread call sites that
//                        never call setCancelToken() (Dropbox::createFolder,
//                        Azum::listAthletes, Nolio::listAthletes) still succeed
//                        against a responding endpoint under the implicit,
//                        default-constructed CancelToken().
//
// ===========================================================================
// WHAT IS REAL HERE, AND WHY THAT IS THE WHOLE POINT
// ===========================================================================
// The REAL src/Cloud/CloudService.cpp and the REAL thirteen migrated providers
// are compiled into this binary, so every row below drives a PRODUCTION ENTRY
// POINT - Strava::open, Xert::readdir, Dropbox::createFolder, Nolio::listAthletes,
// SixCycle::readdir, ... - and reaches blockingRequest the way the application
// reaches it.
//
// A row that called blockingRequest directly would cover the HELPER and not the
// SITE: it would pass unchanged if a provider had been left unmigrated, which is
// exactly the mistake this slice is guarding against. So the only rows in this
// file that touch the helper directly are TEST-146's transition-table rows,
// which are ABOUT the helper's table and cannot be expressed any other way, and
// TEST-153(d), whose thread-affinity logic exists in exactly one place (the
// base) and cannot be observed at a site without a real network.
//
// The seam is S-1's `injectedNam` constructor argument. It is ordinary defaulted-
// argument construction in the shipped code, not a test-only preprocessor branch,
// and there is no #ifdef anywhere in this file or in what it compiles.
// ===========================================================================

#include "Athlete.h"
#include "Azum.h"
#include "CloudService.h"
#include "Context.h"
#include "CyclingAnalytics.h"
#include "Dropbox.h"
#include "MainWindow.h"
#include "Nolio.h"
#include "PolarFlow.h"
#include "RideCache.h"
#include "RideFile.h"
#include "RideWithGPS.h"
#include "Selfloops.h"
#include "Settings.h"
#include "SixCycle.h"
#include "SportTracks.h"
#include "SportsPlusHealth.h"
#include "Strava.h"
#include "TrainingsTageBuch.h"
#include "Xert.h"

#include <QApplication>
#include <QBuffer>
#include <QElapsedTimer>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QPointer>
#include <QProcess>
#include <QSemaphore>
#include <QSignalSpy>
#include <QThread>
#include <QTimer>
#include <QtTest>

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <optional>
#include <utility>

// Defined in stubs/ProviderSeamStubs.cpp - clears the in-memory GSettings store
// between slots so one row cannot inherit another row's credentials.
void gcStubClearSettings();

// ===========================================================================
// PART 0 - CHILD MODES, AND THE PRE-main() OBSERVATION FOR TEST-153 (a)
// ===========================================================================
//
// (a) says factory-template construction before QCoreApplication creates NO
// manager and emits NO warnings. That construction has ALREADY HAPPENED by the
// time any test slot can run: every provider .cpp ends in
// `static bool add = addXxx();` and those run during static initialisation. So
// the observation cannot be a call made early inside main() that constructs a
// provider and looks at it - that would be observing a DIFFERENT construction,
// on a path production never takes.
//
// WHAT THIS USED TO DO, AND WHY IT IS GONE.
//
// The first revision observed the residue with a `__attribute__((init_priority(101)))`
// probe that installed a message handler and appended to a namespace-scope
// QStringList. That is static-initialisation-order undefined behaviour of its
// own: a prioritised initialiser is sequenced BEFORE the ordinary dynamic
// initialisation of objects in the same translation unit, so the probe could
// write into storage that had not been initialised yet. An apparatus that is
// itself UB cannot certify anything, and it would have run green.
//
// It is replaced by an OUT-OF-PROCESS observation. There is no pre-main
// handler, no init_priority, and no dynamically initialised global capture
// storage anywhere in this file. A parent slot launches a child copy of this
// executable; the child answers at the very top of main(), BEFORE QApplication
// exists, prints a machine-readable census on stdout and exits. Warnings the
// static initialisers emit go to the child's stderr, which the parent captures
// from process creation - so nothing has to be installed early to see them.
namespace childmode {

// The modes are mutually exclusive and each is entered exactly once, at the top
// of main(), before anything else runs.
enum Mode {
    None = 0,
    PreMainCensus,       // (a) - census + stderr, before QApplication
    StderrControl,       // positive control - proves stderr capture works
    OffAffinityNam,      // death test - DEC040_NAM_OFF_AFFINITY
    InjectedNamAffinity, // death test - DEC040_INJECTED_NAM_AFFINITY
    Row7SpuriousExec     // TEST-146 row 7, isolated from the main process
};

// argv, not an environment variable, so a mode can never be inherited by
// accident; and stripped before QApplication or QTest ever see it, because
// QTest treats an unknown option as a fatal usage error.
static const char* kFlag = "--dec040-child=";

// RECURSION GUARD. A child must never spawn a child: a bug in mode dispatch
// would otherwise fork-bomb the machine rather than fail a test. The parent
// sets this in the child's environment; spawnChild() refuses to run if it is
// already set in ours.
static const char* kDepthEnv = "DEC040_CHILD_DEPTH";

// Every child announces the mode it actually entered, so the parent can prove
// the child did the intended thing rather than merely dying. "It exited
// nonzero" is not evidence of anything on its own.
static const char* kEnteredPrefix = "DEC040_CHILD_MODE_ENTERED=";

const char* name(Mode m)
{
    switch (m) {
    case PreMainCensus:
        return "premain-census";
    case StderrControl:
        return "stderr-control";
    case OffAffinityNam:
        return "off-affinity-nam";
    case InjectedNamAffinity:
        return "injected-nam-affinity";
    case Row7SpuriousExec:
        return "row7-spurious-exec";
    case None:
        break;
    }
    return "none";
}

// Detected with plain string comparison on argv: this runs before any Qt
// application object exists, so nothing here may touch Qt's argument handling.
Mode detect(int argc, char* argv[])
{
    const size_t flagLen = strlen(kFlag);
    for (int i = 1; i < argc; i++) {
        if (strncmp(argv[i], kFlag, flagLen) != 0)
            continue;
        const char* value = argv[i] + flagLen;
        for (int m = PreMainCensus; m <= Row7SpuriousExec; m++)
            if (strcmp(value, name(static_cast<Mode>(m))) == 0)
                return static_cast<Mode>(m);
        fprintf(stderr, "unknown child mode '%s'\n", value);
        exit(97);
    }
    return None;
}

// Remove every --dec040-child= argument in place, so what reaches QApplication
// and QTest::qExec is exactly what an ordinary run would have passed.
void stripFlags(int& argc, char* argv[])
{
    const size_t flagLen = strlen(kFlag);
    int out = 1;
    for (int i = 1; i < argc; i++) {
        if (strncmp(argv[i], kFlag, flagLen) == 0)
            continue;
        argv[out++] = argv[i];
    }
    for (int i = out; i < argc; i++)
        argv[i] = nullptr;
    argc = out;
}

void announce(Mode m)
{
    fprintf(stderr, "%s%s\n", kEnteredPrefix, name(m));
    fflush(stderr);
}

// The three modes that need a QApplication. Declared here so main()'s dispatch
// reads as one table; DEFINED further down, after the fixtures they use
// (ProbeService, net::FakeNam, fixture::Harness) exist.
int runOffAffinityNam();
int runInjectedNamAffinity();
int runRow7SpuriousExec();

} // namespace childmode

// ===========================================================================
// THE EXPECTED MIGRATED PROVIDERS, BY IDENTITY.
//
// A count is not a census. `serviceCount() >= 12` passes if a provider is
// missing and two unrelated services registered, and it passes if the wrong
// twelve registered. What (a) actually claims is a statement about NAMED
// templates, so the expectation is written as names and checked as names.
//
// THE KEY IS id(), NOT THE CLASS NAME, and they are not the same string for
// four of the thirteen - CyclingAnalytics registers as "Cycling Analytics",
// SixCycle as "Sixcycle", SportTracks as "SportTracks.mobi" and
// SportsPlusHealth as "SportPlusHealth". The factory is keyed by id(), so the
// id is what gets looked up; the class name is carried alongside purely so a
// failure message names the provider a human is looking for.
//
// PolarFlow is the one explicitly recorded exception: addPolarFlow() and its
// `static bool add` are wrapped in `#if 0` at the bottom of PolarFlow.cpp, so
// PolarFlow is not a registered service in ANY build of this tree. It is still
// one of the thirteen manager migrations; it is simply not one of the pre-main
// registrations, and it is reported separately rather than folded into a
// tolerance on a number.
// ===========================================================================
struct MigratedTemplate
{
    const char* cls; // the class, for human-readable failures
    const char* id;  // the factory key - CloudService::id()
};

static const MigratedTemplate kExpectedRegisteredMigrated[] = {{"Azum", "Azum"},
                                                               {"CyclingAnalytics", "Cycling Analytics"},
                                                               {"Dropbox", "Dropbox"},
                                                               {"Nolio", "Nolio"},
                                                               {"RideWithGPS", "RideWithGPS"},
                                                               {"Selfloops", "Selfloops"},
                                                               {"SixCycle", "Sixcycle"},
                                                               {"SportTracks", "SportTracks.mobi"},
                                                               {"SportsPlusHealth", "SportPlusHealth"},
                                                               {"Strava", "Strava"},
                                                               {"TrainingsTageBuch", "TrainingsTageBuch"},
                                                               {"Xert", "Xert"}};
static const int kExpectedRegisteredMigratedCount =
    int(sizeof(kExpectedRegisteredMigrated) / sizeof(kExpectedRegisteredMigrated[0]));

static const MigratedTemplate kRegistrationExceptions[] = {{"PolarFlow", "PolarFlow"}};
static const int kRegistrationExceptionCount =
    int(sizeof(kRegistrationExceptions) / sizeof(kRegistrationExceptions[0]));

// ===========================================================================
// PART 1 - THE FAKE NETWORK
// ===========================================================================
namespace net {

enum Behaviour {
    Ok,     // answers, no error
    Failed, // answers, carrying a network error
    Silent  // accepts the request and never answers - the endpoint this slice exists for
};

struct Plan
{
    Behaviour behaviour = Ok;
    QByteArray body;
    int httpStatus = 200;
    QNetworkReply::NetworkError error = QNetworkReply::NoError;
    int delayMs = 0;
};

class FakeReply;

// Per-run state. Global rather than per-manager because the manager is ADOPTED
// by the service under test and dies with it, and several of these counters have
// to be readable after that.
QList<Plan> plans; // consumed in order; the LAST plan repeats
int planCursor = 0;
int repliesCreated = 0;            // replies handed out
int repliesDestroyed = 0;          // replies actually destroyed
int abortCalls = 0;                // abort() calls
QList<QPointer<FakeReply>> issued; // every reply, as a weak pointer
QList<QPointer<FakeReply>> live;   // the ones still waiting

bool rescueFired = false; // did the harness have to end a wait?

void reset()
{
    plans.clear();
    planCursor = 0;
    repliesCreated = repliesDestroyed = abortCalls = 0;
    issued.clear();
    live.clear();
    rescueFired = false;
}

Plan nextPlan()
{
    if (plans.isEmpty())
        return Plan();
    const int i = qMin(planCursor, plans.size() - 1);
    planCursor++;
    return plans.at(i);
}

// A QNetworkReply that behaves like one, including the part that matters most
// here: abort() synthesises a finished(), exactly as a real reply does. That
// synthetic finish is what transition-table row 6 is about, and a fake that did
// not emit it would make the row unfalsifiable.
class FakeReply : public QNetworkReply
{
    Q_OBJECT

  public:
    FakeReply(QNetworkAccessManager::Operation op, const QNetworkRequest& request, const Plan& plan) : plan_(plan)
    {
        setRequest(request);
        setUrl(request.url());
        setOperation(op);
        setOpenMode(QIODevice::ReadOnly);
        repliesCreated++;

        if (plan_.behaviour != Silent)
            QTimer::singleShot(plan_.delayMs, this, &FakeReply::deliver);
    }

    ~FakeReply() override { repliesDestroyed++; }

    void abort() override
    {
        abortCalls++;
        if (aborted_ || finishedAlready_)
            return;
        aborted_ = true;
        setError(QNetworkReply::OperationCanceledError, QStringLiteral("aborted"));
        finish();
    }

    qint64 bytesAvailable() const override { return buffer_.bytesAvailable() + QNetworkReply::bytesAvailable(); }

    bool isSequential() const override { return true; }

    // Used by TEST-146's row 5 and by the harness rescue: end the wait from
    // outside, on the caller's schedule rather than on a timer of our own.
    void deliverNow() { deliver(); }

  public slots:
    void deliver()
    {
        if (aborted_ || finishedAlready_)
            return;
        if (plan_.behaviour == Failed) {
            setError(plan_.error == QNetworkReply::NoError ? QNetworkReply::ProtocolInvalidOperationError : plan_.error,
                     QStringLiteral("fake network error"));
        }
        buffer_.setData(plan_.body);
        buffer_.open(QIODevice::ReadOnly);
        finish();
    }

  protected:
    qint64 readData(char* data, qint64 maxlen) override
    {
        const qint64 n = buffer_.read(data, maxlen);
        return n > 0 ? n : -1;
    }

  private:
    void finish()
    {
        finishedAlready_ = true;
        setAttribute(QNetworkRequest::HttpStatusCodeAttribute, plan_.httpStatus);

        // readyRead BEFORE finished, because that is the order a real reply uses
        // and because the ASYNC sites depend on it: Strava::readFile connects
        // readyRead to a slot that accumulates the body into the caller's buffer,
        // and its completion slot then parses that buffer. A fake that only
        // emitted finished() would hand every async site an empty buffer, and
        // every row over those sites would be measuring the fake.
        if (!plan_.body.isEmpty())
            emit readyRead();

        setFinished(true);
        live.removeAll(QPointer<FakeReply>(this));
        emit finished();
    }

    Plan plan_;
    QBuffer buffer_;
    bool aborted_ = false;
    bool finishedAlready_ = false;
};

class FakeNam : public QNetworkAccessManager
{
    Q_OBJECT

  public:
    explicit FakeNam(QObject* parent = nullptr) : QNetworkAccessManager(parent) {}
    ~FakeNam() override { instancesDestroyed++; }

    static int instancesDestroyed;

  protected:
    QNetworkReply* createRequest(Operation op, const QNetworkRequest& request, QIODevice* outgoingData) override
    {
        Q_UNUSED(outgoingData);
        FakeReply* reply = new FakeReply(op, request, nextPlan());
        issued << QPointer<FakeReply>(reply);
        if (!reply->isFinished())
            live << QPointer<FakeReply>(reply);
        return reply;
    }
};

int FakeNam::instancesDestroyed = 0;

// THE HARNESS RESCUE, and it is an ASSERTION APPARATUS rather than a helper.
//
// A test for "a never-responding endpoint is bounded" has a problem: if the
// bound is broken the production wait never returns, and the run hangs instead
// of failing. So the harness arms its own, much later timer; if it ever fires,
// it means PRODUCTION did not end its own wait, and `rescueFired` is the
// verdict. Rows assert it stayed false. It also unwedges the run so the
// remaining rows still report.
class Rescue
{
  public:
    explicit Rescue(int ms)
    {
        rescueFired = false;
        timer_.setSingleShot(true);
        QObject::connect(&timer_, &QTimer::timeout, [] {
            rescueFired = true;
            const QList<QPointer<FakeReply>> waiting = live;
            for (const QPointer<FakeReply>& r : waiting)
                if (r)
                    r->deliverNow();
        });
        timer_.start(ms);
    }
    ~Rescue() { timer_.stop(); }

  private:
    QTimer timer_;
};

// Deferred deletes posted by blockingRequest's disposer are delivered here, at
// the top event-loop level, which is where the caller returns to.
void drainDeferredDeletes()
{
    QCoreApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);
    QCoreApplication::processEvents(QEventLoop::AllEvents, 10);
    QCoreApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);
}

} // namespace net

// ===========================================================================
// PART 2 - THE 22 SITES
// ===========================================================================
//
// SHAPE COLUMN, RE-DERIVED BY READING EACH ENCLOSING FUNCTION (not by grep and
// not by analogy). A = one request, one wait. B = a wait inside a PAGINATION
// loop, so N pages means N waits. C = a wait in a function called from inside a
// per-activity loop, so N activities means N waits.
//
//   1  Dropbox::createFolder           A
//   2  Dropbox::readdir                B   while(listHasMoreEntries), cursor paging
//   3  Azum::open                      A
//   4  Azum::readdir                   B   do/while(!next.isEmpty())
//   5  Azum::listAthletes              B   do/while(!next.isEmpty())
//   6  SportTracks::open               A
//   7  SportTracks::readdir            B   while(wantmore), page paging
//   8  Xert::open                      A
//   9  Xert::readdir                   A   *** ONE request; see the note below
//  10  Xert::readActivityDetail        C   called from readdir's per-activity for
//  11  Nolio::open                     A
//  12  Nolio::readdir                  A   *** ONE request; from/to range, no paging
//  13  Nolio::listAthletes             A
//  14  PolarFlow::open                 A
//  15  Strava::open                    A
//  16  Strava::readdir                 B   while(offset < resultCount), per_page paging
//  17  Strava::addSamples              C   per-activity, under prepareResponse
//  18  TrainingsTageBuch::open (1st)   A
//  19  TrainingsTageBuch::open (2nd)   A
//  20  CyclingAnalytics::readdir       A   *** "fetch in one hit"; no paging
//  21  SixCycle::open                  A
//  22  SixCycle::readdir               A   *** ONE post; no paging
//
// The three starred rows are DISAGREEMENTS with the census this slice was
// briefed with, which listed 9, 12, 20 and 22 as shape B. They are not:
//   * Xert::readdir issues exactly one GET and then loops over the ACTIVITIES in
//     the single response. The loop is real, but the wait inside it is site 10,
//     not site 9.
//   * Nolio::readdir passes from/to as query parameters and reads the whole
//     range back in one response. There is no `next`, offset or page anywhere in
//     the function.
//   * CyclingAnalytics::readdir says "fetch in one hit" in its own comment and
//     does exactly that.
//   * SixCycle::readdir posts once for the whole activity summary list.
// So the shape-B/C set is {2, 4, 5, 7, 10, 16, 17} - SEVEN sites, not eight and
// not the set that was briefed. TEST-147 is built over exactly that set.
enum SiteId {
    Site_Dropbox_createFolder = 1,
    Site_Dropbox_readdir,
    Site_Azum_open,
    Site_Azum_readdir,
    Site_Azum_listAthletes,
    Site_SportTracks_open,
    Site_SportTracks_readdir,
    Site_Xert_open,
    Site_Xert_readdir,
    Site_Xert_readActivityDetail,
    Site_Nolio_open,
    Site_Nolio_readdir,
    Site_Nolio_listAthletes,
    Site_PolarFlow_open,
    Site_Strava_open,
    Site_Strava_readdir,
    Site_Strava_addSamples,
    Site_TTB_open_first,
    Site_TTB_open_second,
    Site_CyclingAnalytics_readdir,
    Site_SixCycle_open,
    Site_SixCycle_readdir,
    Site_COUNT_ = Site_SixCycle_readdir
};

struct SiteOutcome
{
    bool callerSuccess = false;   // did the production entry point report success?
    int items = 0;                // entries / athletes produced
    QStringList errors;           // the site's own errors channel, where it has one
    int requests = 0;             // replies the fake manager handed out
    bool hasErrorsChannel = true; // false for the sites that cannot report at all
};

// The bodies each site's REAL parser accepts, each carrying exactly ONE entry so
// that "the body got through" is observable as a count and not as a shrug.
namespace body {

const char* azumOpen = "{\"access_token\":\"AT\",\"refresh_token\":\"RT\"}";
const char* tokenPair = "{\"access_token\":\"AT\",\"refresh_token\":\"RT\"}";

const char* dropboxFolder = "{\"metadata\":{\"name\":\"x\"}}";
const char* dropboxDir =
    "{\"entries\":[{\".tag\":\"file\",\"path_display\":\"/gc/2020_01_01_10_00_00.json\",\"bytes\":10}],"
    "\"has_more\":false,\"cursor\":\"\"}";

const char* azumDir = "{\"next\":\"\",\"results\":[{\"id\":\"7\",\"export_name\":\"a.fit\","
                      "\"start\":\"2020-01-01T10:00:00\",\"distance\":1000,\"timer_time\":\"P0DT00H25M57S\"}]}";
const char* azumAthletes = "{\"next\":\"\",\"results\":[{\"user\":7,\"full_name\":\"A Rider\"}]}";

const char* sportTracksDir = "{\"items\":[{\"duration\":\"600.00\",\"name\":\"Cycling\","
                             "\"start_time\":\"2020-01-01T10:00:00+00:00\",\"total_distance\":\"1000\","
                             "\"uri\":\"https://api.sporttracks.mobi/api/v2/fitnessActivities/7\",\"user_id\":\"1\"}]}";

const char* xertDir = "{\"activities\":[{\"name\":\"n\",\"start_date\":{\"date\":\"2020-01-01 10:00:00.000000\"},"
                      "\"path\":\"P1\",\"activity_type\":\"Cycling\"}]}";
const char* xertDetail = "{\"summary\":{\"distance\":12.5,\"duration\":600}}";

const char* nolioDir = "[{\"name\":\"n.json\",\"nolio_id\":7,\"distance\":1000,\"duration\":600,"
                       "\"date_start\":\"2020-01-01T10:00:00Z\"}]";
const char* nolioAthletes = "[{\"nolio_id\":7,\"name\":\"A Rider\"}]";

const char* polarFlowOpen = "{\"delegates\":[]}";

const char* stravaDir = "[{\"id\":7,\"name\":\"n\",\"distance\":1000,\"elapsed_time\":600,"
                        "\"start_date_local\":\"2020-01-01T10:00:00\"}]";
const char* stravaDirEmpty = "[]";
const char* stravaActivity = "{\"id\":7,\"type\":\"Ride\",\"name\":\"n\",\"manual\":false,"
                             "\"start_date_local\":\"2020-01-01T10:00:00\"}";
const char* stravaStreams = "[{\"type\":\"time\",\"data\":[0,1,2]},{\"type\":\"watts\",\"data\":[100,110,120]}]";

const char* ttbSettingsNoSession = "<result><pro>0</pro><session></session></result>";
const char* ttbSettingsWithSession = "<result><pro>0</pro><session>SESSION-1</session></result>";
const char* ttbSession = "<result><session>SESSION-2</session></result>";

const char* cyclingAnalyticsDir = "{\"rides\":[{\"id\":7,\"local_datetime\":\"2020-01-01T10:00:00\",\"format\":\"tcx\","
                                  "\"summary\":{\"distance\":12.5,\"duration\":600}}]}";

const char* sixCycleOpen = "{\"key\":\"TOKEN\",\"user\":\"USER\"}";
const char* sixCycleDir = "[{\"url\":\"https://s3.amazonaws.com/sixcycle/rawFiles/a.fit\",\"id\":7,"
                          "\"activityStartDateTime\":\"2020-01-01T10:00:00Z\","
                          "\"meters_in_distance\":1000,\"seconds_in_activity\":600}]";

} // namespace body

// The plan list a site needs in order to SUCCEED. Sites 9/10 and 16 and 17 and
// 19 need more than one, because they make more than one request.
static QList<net::Plan> okPlans(SiteId site)
{
    QList<net::Plan> plans;
    net::Plan p;

    switch (site) {
    case Site_Dropbox_createFolder:
        p.body = body::dropboxFolder;
        plans << p;
        break;
    case Site_Dropbox_readdir:
        p.body = body::dropboxDir;
        plans << p;
        break;
    case Site_Azum_open:
        p.body = body::azumOpen;
        plans << p;
        break;
    case Site_Azum_readdir:
        p.body = body::azumDir;
        plans << p;
        break;
    case Site_Azum_listAthletes:
        p.body = body::azumAthletes;
        plans << p;
        break;
    case Site_SportTracks_open:
        p.body = body::tokenPair;
        plans << p;
        break;
    case Site_SportTracks_readdir:
        p.body = body::sportTracksDir;
        plans << p;
        break;
    case Site_Xert_open:
        p.body = body::tokenPair;
        plans << p;
        break;
    case Site_Xert_readdir:
    case Site_Xert_readActivityDetail: {
        net::Plan listing;
        listing.body = body::xertDir;
        net::Plan detail;
        detail.body = body::xertDetail;
        plans << listing << detail;
        break;
    }
    case Site_Nolio_open:
        p.body = body::tokenPair;
        plans << p;
        break;
    case Site_Nolio_readdir:
        p.body = body::nolioDir;
        plans << p;
        break;
    case Site_Nolio_listAthletes:
        p.body = body::nolioAthletes;
        plans << p;
        break;
    case Site_PolarFlow_open:
        p.body = body::polarFlowOpen;
        plans << p;
        break;
    case Site_Strava_open:
        p.body = body::tokenPair;
        plans << p;
        break;
    case Site_Strava_readdir: {
        // Strava's loop only stops when a page comes back EMPTY, so a one-entry
        // listing is genuinely two requests.
        net::Plan page1;
        page1.body = body::stravaDir;
        net::Plan page2;
        page2.body = body::stravaDirEmpty;
        plans << page1 << page2;
        break;
    }
    case Site_Strava_addSamples: {
        net::Plan activity;
        activity.body = body::stravaActivity;
        net::Plan streams;
        streams.body = body::stravaStreams;
        plans << activity << streams;
        break;
    }
    case Site_TTB_open_first:
        p.body = body::ttbSettingsWithSession;
        plans << p;
        break;
    case Site_TTB_open_second: {
        net::Plan settings;
        settings.body = body::ttbSettingsNoSession;
        net::Plan session;
        session.body = body::ttbSession;
        plans << settings << session;
        break;
    }
    case Site_CyclingAnalytics_readdir:
        p.body = body::cyclingAnalyticsDir;
        plans << p;
        break;
    case Site_SixCycle_open:
        p.body = body::sixCycleOpen;
        plans << p;
        break;
    case Site_SixCycle_readdir: {
        // readdir needs a session, and the only way to get one is open().
        net::Plan open;
        open.body = body::sixCycleOpen;
        net::Plan dir;
        dir.body = body::sixCycleDir;
        plans << open << dir;
        break;
    }
    }
    return plans;
}

// How many entries the ok-plan above should produce, and whether the entry point
// reports success. -1 means "this site has no item count".
static int okItems(SiteId site)
{
    switch (site) {
    case Site_Dropbox_readdir:
    case Site_Azum_readdir:
    case Site_Azum_listAthletes:
    case Site_SportTracks_readdir:
    case Site_Xert_readdir:
    case Site_Xert_readActivityDetail:
    case Site_Nolio_readdir:
    case Site_Nolio_listAthletes:
    case Site_Strava_readdir:
    case Site_CyclingAnalytics_readdir:
    case Site_SixCycle_readdir:
        return 1;
    default:
        return -1;
    }
}

// How many requests the ok-plan should consume. This is what makes the shape
// census a MEASUREMENT rather than a reading: a site claimed to be paginated
// that turns out to issue one request shows up here.
static int okRequests(SiteId site)
{
    switch (site) {
    case Site_Xert_readdir:
    case Site_Xert_readActivityDetail:
    case Site_Strava_readdir:
    case Site_Strava_addSamples:
    case Site_TTB_open_second:
    case Site_SixCycle_readdir:
        return 2;
    default:
        return 1;
    }
}

// The site whose FIRST request is the one under test issues its request
// immediately; sites 10, 19 and 22 are reached only after an earlier request has
// succeeded, so their "make the site under test time out" plan needs the earlier
// request to answer normally first.
static int silentRequestIndex(SiteId site)
{
    switch (site) {
    case Site_Xert_readActivityDetail:
        return 1; // the listing answers, the detail does not
    case Site_Strava_addSamples:
        return 1; // the activity answers, the streams do not
    case Site_TTB_open_second:
        return 1; // settings answer, the session does not
    case Site_SixCycle_readdir:
        return 1; // open answers, the listing does not
    default:
        return 0;
    }
}

// The plan list that makes the site under test hit its watchdog: everything
// before it answers, the site itself never does.
static QList<net::Plan> silentPlans(SiteId site)
{
    QList<net::Plan> plans = okPlans(site);
    const int idx = silentRequestIndex(site);
    while (plans.size() <= idx)
        plans << net::Plan();
    plans[idx].behaviour = net::Silent;
    plans[idx].body = QByteArray();
    // Anything the site might request AFTER the one under test must also stay
    // silent, so a mutant cannot pass by racing ahead.
    for (int i = idx + 1; i < plans.size(); i++)
        plans[i].behaviour = net::Silent;
    while (plans.size() < idx + 2) {
        net::Plan tail;
        tail.behaviour = net::Silent;
        plans << tail;
    }
    return plans;
}

// The plan list that makes the site under test come back with a NETWORK ERROR
// rather than a timeout - the other half of "non-Finished".
static QList<net::Plan> erroredPlans(SiteId site)
{
    QList<net::Plan> plans = okPlans(site);
    const int idx = silentRequestIndex(site);
    while (plans.size() <= idx)
        plans << net::Plan();
    plans[idx].behaviour = net::Failed;
    plans[idx].error = QNetworkReply::ContentNotFoundError;
    plans[idx].httpStatus = 404;
    plans[idx].body = QByteArray();
    for (int i = idx + 1; i < plans.size(); i++) {
        plans[i].behaviour = net::Failed;
        plans[i].error = QNetworkReply::ContentNotFoundError;
        plans[i].body = QByteArray();
    }
    while (plans.size() < idx + 2) {
        net::Plan tail;
        tail.behaviour = net::Failed;
        tail.error = QNetworkReply::ContentNotFoundError;
        plans << tail;
    }
    return plans;
}

// HOW MANY OF THE REPLIES A SITE'S DRIVE CREATES ARE **NOT** blockingRequest'S.
//
// Site 17 is the only one: reaching Strava::addSamples means going in through
// the ASYNCHRONOUS Strava::readFile, which creates its own reply, connects
// finished() to a completion slot, and - like the nine other readFile
// implementations in this tree, all written the same way - NEVER DELETES IT.
// That reply is not disposed of by blockingRequest and this slice does not
// change its ownership, so TEST-145 must not claim it.
//
// *** FINDING, DECLARED AND OUT OF SCOPE: the async readFile reply leaks in
// *** Azum, CyclingAnalytics, Dropbox, Nolio, PolarFlow, SixCycle, SportTracks,
// *** Strava and Xert (grep: `connect(reply, SIGNAL(finished()), this,
// *** SLOT(readFileCompleted()))` with no matching deleteLater). It is
// *** pre-existing, it is not what W2 is about, and inventing an owner for it
// *** here would be a second lifetime change smuggled into a lifetime slice.
static int repliesNotOwnedByBlockingRequest(SiteId site)
{
    return site == Site_Strava_addSamples ? 1 : 0;
}

static const char* siteName(SiteId site)
{
    switch (site) {
    case Site_Dropbox_createFolder:
        return "01 Dropbox::createFolder";
    case Site_Dropbox_readdir:
        return "02 Dropbox::readdir";
    case Site_Azum_open:
        return "03 Azum::open";
    case Site_Azum_readdir:
        return "04 Azum::readdir";
    case Site_Azum_listAthletes:
        return "05 Azum::listAthletes";
    case Site_SportTracks_open:
        return "06 SportTracks::open";
    case Site_SportTracks_readdir:
        return "07 SportTracks::readdir";
    case Site_Xert_open:
        return "08 Xert::open";
    case Site_Xert_readdir:
        return "09 Xert::readdir";
    case Site_Xert_readActivityDetail:
        return "10 Xert::readActivityDetail";
    case Site_Nolio_open:
        return "11 Nolio::open";
    case Site_Nolio_readdir:
        return "12 Nolio::readdir";
    case Site_Nolio_listAthletes:
        return "13 Nolio::listAthletes";
    case Site_PolarFlow_open:
        return "14 PolarFlow::open";
    case Site_Strava_open:
        return "15 Strava::open";
    case Site_Strava_readdir:
        return "16 Strava::readdir";
    case Site_Strava_addSamples:
        return "17 Strava::addSamples";
    case Site_TTB_open_first:
        return "18 TrainingsTageBuch::open (settings)";
    case Site_TTB_open_second:
        return "19 TrainingsTageBuch::open (session)";
    case Site_CyclingAnalytics_readdir:
        return "20 CyclingAnalytics::readdir";
    case Site_SixCycle_open:
        return "21 SixCycle::open";
    case Site_SixCycle_readdir:
        return "22 SixCycle::readdir";
    }
    return "?";
}

// ===========================================================================
// PART 3 - THE FIXTURE
// ===========================================================================
namespace fixture {

// The minimum of the real object graph the providers touch: saveSettings() reads
// context->athlete->cyclist on every successful open().
class Harness
{
  public:
    Harness()
    {
        home_.setAutoRemove(true);
        window_ = new QWidget;
        context_ = new Context(reinterpret_cast<MainWindow*>(window_));
        athlete_ = new Athlete(context_, QDir(home_.path()));
        athlete_->cyclist = QStringLiteral("TestRider");
        context_->athlete = athlete_;

        // DEC-040 Stage 1 (A3-F1). CloudServiceAutoDownload::run()'s
        // found.count()>0 branch walks athlete->rideCache->rides()
        // unconditionally; the Athlete stand-in leaves it null. Production
        // athletes always have one, so the harness supplies an EMPTY one -
        // an athlete with nothing local, which is what makes a returned
        // entry something run() actually wants.
        rideCache_ = new RideCache(context_);
        athlete_->rideCache = rideCache_;
    }
    ~Harness()
    {
        // OWNERSHIP ORDER. The athlete only BORROWS the cache (production has
        // the athlete own it, but the stand-in Athlete dtor deletes only
        // `home`, so nothing here would free it). Clear the borrow FIRST so
        // no later teardown step can reach a freed cache, then free it, and
        // only then destroy the athlete that pointed at it.
        athlete_->rideCache = nullptr;
        delete rideCache_;
        delete athlete_;
        delete context_;
        delete window_;
    }
    Context* context() { return context_; }

  private:
    QTemporaryDir home_;
    QWidget* window_ = nullptr;
    Context* context_ = nullptr;
    Athlete* athlete_ = nullptr;
    RideCache* rideCache_ = nullptr;
};

// The credentials each site needs in order to get PAST its own early return and
// actually issue a request. A site that early-returns is not covered, so these
// are load-bearing, not decoration.
void configure(SiteId site, CloudService* service)
{
    switch (site) {
    case Site_Dropbox_createFolder:
    case Site_Dropbox_readdir:
        service->setSetting(GC_DROPBOX_TOKEN, "TOKEN");
        break;
    case Site_Azum_open:
    case Site_Azum_readdir:
    case Site_Azum_listAthletes:
        service->setSetting(GC_AZUM_ACCESS_TOKEN, "TOKEN");
        service->setSetting(GC_AZUM_REFRESH_TOKEN, "REFRESH");
        service->setSetting(GC_AZUM_ATHLETE_ID, "7");
        break;
    case Site_SportTracks_open:
        service->setSetting(GC_SPORTTRACKS_REFRESH_TOKEN, "REFRESH");
        break;
    case Site_SportTracks_readdir:
        service->setSetting(GC_SPORTTRACKS_TOKEN, "TOKEN");
        break;
    case Site_Xert_open:
        service->setSetting(GC_XERT_REFRESH_TOKEN, "REFRESH");
        break;
    case Site_Xert_readdir:
    case Site_Xert_readActivityDetail:
        service->setSetting(GC_XERT_TOKEN, "TOKEN");
        break;
    case Site_Nolio_open:
        // Nolio reads the GLOBAL settings, not the per-service configuration.
        appsettings->setValue(GC_NOLIO_REFRESH_TOKEN, "REFRESH");
        appsettings->setValue(GC_NOLIO_LAST_REFRESH, QDateTime::currentDateTime().addDays(-30).toString());
        break;
    case Site_Nolio_readdir:
    case Site_Nolio_listAthletes:
        appsettings->setValue(GC_NOLIO_ACCESS_TOKEN, "TOKEN");
        break;
    case Site_PolarFlow_open:
        service->setSetting(GC_POLARFLOW_TOKEN, "TOKEN");
        break;
    case Site_Strava_open:
        service->setSetting(GC_STRAVA_REFRESH_TOKEN, "REFRESH");
        break;
    case Site_Strava_readdir:
    case Site_Strava_addSamples:
        service->setSetting(GC_STRAVA_TOKEN, "TOKEN");
        break;
    case Site_TTB_open_first:
    case Site_TTB_open_second:
        service->setSetting(GC_TTBUSER, "user");
        service->setSetting(GC_TTBPASS, "pass");
        break;
    case Site_CyclingAnalytics_readdir:
        service->setSetting(GC_CYCLINGANALYTICS_TOKEN, "TOKEN");
        break;
    case Site_SixCycle_open:
    case Site_SixCycle_readdir:
        service->setSetting(GC_SIXCYCLE_USER, "user@example.com");
        service->setSetting(GC_SIXCYCLE_PASS, "pass");
        break;
    }
}

CloudService* makeService(SiteId site, Context* context, net::FakeNam* nam)
{
    switch (site) {
    case Site_Dropbox_createFolder:
    case Site_Dropbox_readdir:
        return new Dropbox(context, nam);
    case Site_Azum_open:
    case Site_Azum_readdir:
    case Site_Azum_listAthletes:
        return new Azum(context, nam);
    case Site_SportTracks_open:
    case Site_SportTracks_readdir:
        return new SportTracks(context, nam);
    case Site_Xert_open:
    case Site_Xert_readdir:
    case Site_Xert_readActivityDetail:
        return new Xert(context, nam);
    case Site_Nolio_open:
    case Site_Nolio_readdir:
    case Site_Nolio_listAthletes:
        return new Nolio(context, nam);
    case Site_PolarFlow_open:
        return new PolarFlow(context, nam);
    case Site_Strava_open:
    case Site_Strava_readdir:
    case Site_Strava_addSamples:
        return new Strava(context, nam);
    case Site_TTB_open_first:
    case Site_TTB_open_second:
        return new TrainingsTageBuch(context, nam);
    case Site_CyclingAnalytics_readdir:
        return new CyclingAnalytics(context, nam);
    case Site_SixCycle_open:
    case Site_SixCycle_readdir:
        return new SixCycle(context, nam);
    }
    return nullptr;
}

// Strava's site 17 is not a synchronous entry point. readFile() registers an
// async reply; the completion slot runs prepareResponse, which runs addSamples,
// which is the wait under test. So the driver has to let the first reply finish
// and then observe what came out of the completion channel.
class StravaCompletionWatcher : public QObject
{
    Q_OBJECT

  public:
    explicit StravaCompletionWatcher(CloudService* service)
    {
        connect(service, &CloudService::readComplete, this, [this](QByteArray* data, QString, QString) {
            completes++;
            lastPointer = data;
            stagedBytes = data ? *data : QByteArray();
        });
        connect(service, &CloudService::readFailed, this, [this](QByteArray* data, QString, QString reason) {
            failures++;
            lastPointer = data;
            stagedBytes = data ? *data : QByteArray();
            lastReason = reason;
        });
    }

    int completes = 0;
    int failures = 0;
    QByteArray* lastPointer = nullptr;
    QByteArray stagedBytes;
    QString lastReason;
};

// Drive ONE production call site to completion and report what the CALLER saw.
//
// `cancelToken` defaults to std::nullopt, which means "do not call
// setCancelToken() at all" - the caller relies entirely on CloudService's own
// construction-time default for `cancelToken_` (ORCH-055: a defaulted
// CancelToken() argument passed to an UNCONDITIONAL setCancelToken() call
// overwrites that member with a fresh default before the site method ever
// runs, so a broken member initializer would be silently papered over and
// never observed by TEST-148's rows). Passing an explicit CancelToken (as the
// Stage-2-piece-3 Cancelled-mid-listing rows do) still works unchanged -
// std::optional converts implicitly from its value type.
SiteOutcome driveSite(SiteId site, const QList<net::Plan>& plans, int timeoutOverrideMs,
                      std::optional<CancelToken> cancelToken = std::nullopt)
{
    SiteOutcome out;

    gcStubClearSettings();
    net::reset();
    net::plans = plans;

    Harness harness;
    net::FakeNam* nam = new net::FakeNam();
    CloudService* service = makeService(site, harness.context(), nam);
    configure(site, service);
    service->setRequestTimeoutOverrideMs(timeoutOverrideMs);
    if (cancelToken)
        service->setCancelToken(*cancelToken);

    // The apparatus watchdog. Generously later than any bound a row uses, so it
    // only ever fires when PRODUCTION failed to end its own wait.
    net::Rescue rescue(3000);

    QStringList errors;
    const QDateTime from = QDateTime::currentDateTime().addYears(-2);
    const QDateTime to = QDateTime::currentDateTime();

    switch (site) {
    case Site_Dropbox_createFolder:
        out.callerSuccess = static_cast<Dropbox*>(service)->createFolder("/gc");
        out.hasErrorsChannel = false; // createFolder returns bool and nothing else
        break;

    case Site_Dropbox_readdir:
        out.items = static_cast<Dropbox*>(service)->readdir("/gc", errors).count();
        out.callerSuccess = errors.isEmpty();
        break;

    case Site_Azum_open:
        out.callerSuccess = service->open(errors);
        break;

    case Site_Azum_readdir:
        out.items = static_cast<Azum*>(service)->readdir("", errors, from, to).count();
        out.callerSuccess = errors.isEmpty();
        break;

    case Site_Azum_listAthletes:
        out.items = service->listAthletes().count();
        out.callerSuccess = out.items > 0;
        out.hasErrorsChannel = false; // listAthletes returns the list and nothing else
        break;

    case Site_SportTracks_open:
    case Site_Xert_open:
    case Site_Nolio_open:
    case Site_PolarFlow_open:
    case Site_Strava_open:
    case Site_TTB_open_first:
    case Site_TTB_open_second:
    case Site_SixCycle_open:
        out.callerSuccess = service->open(errors);
        break;

    case Site_SportTracks_readdir:
        out.items = static_cast<SportTracks*>(service)->readdir("", errors, from, to).count();
        out.callerSuccess = errors.isEmpty();
        break;

    case Site_Xert_readdir:
    case Site_Xert_readActivityDetail:
        out.items = static_cast<Xert*>(service)->readdir("", errors, from, to).count();
        out.callerSuccess = errors.isEmpty();
        break;

    case Site_Nolio_readdir:
        out.items = static_cast<Nolio*>(service)->readdir("", errors, from, to).count();
        out.callerSuccess = errors.isEmpty();
        break;

    case Site_Nolio_listAthletes:
        out.items = service->listAthletes().count();
        out.callerSuccess = out.items > 0;
        out.hasErrorsChannel = false;
        break;

    case Site_Strava_readdir:
        out.items = static_cast<Strava*>(service)->readdir("", errors, from, to).count();
        out.callerSuccess = errors.isEmpty();
        break;

    case Site_Strava_addSamples: {
        StravaCompletionWatcher watcher(service);
        QByteArray* buffer = new QByteArray();
        static_cast<Strava*>(service)->readFile(buffer, "2020_01_01_10_00_00.json", "7");
        // Let the first (activity) reply finish; its completion slot is what
        // reaches addSamples.
        QElapsedTimer waiting;
        waiting.start();
        while (watcher.completes + watcher.failures == 0 && waiting.elapsed() < 3000)
            QCoreApplication::processEvents(QEventLoop::AllEvents, 20);
        out.callerSuccess = watcher.completes == 1 && watcher.failures == 0;
        out.items = watcher.completes;
        if (!watcher.lastReason.isEmpty())
            errors << watcher.lastReason;
        delete buffer;
        break;
    }

    case Site_CyclingAnalytics_readdir:
        out.items = static_cast<CyclingAnalytics*>(service)->readdir("", errors, from, to).count();
        out.callerSuccess = errors.isEmpty();
        break;

    case Site_SixCycle_readdir: {
        SixCycle* sixcycle = static_cast<SixCycle*>(service);
        QStringList openErrors;
        sixcycle->open(openErrors); // the only way to get a session token
        out.items = sixcycle->readdir("", errors, from, to).count();
        out.callerSuccess = errors.isEmpty();
        break;
    }
    }

    out.errors = errors;
    out.requests = net::repliesCreated;

    delete service; // takes the adopted FakeNam with it
    net::drainDeferredDeletes();
    return out;
}

} // namespace fixture

// A REAL CloudService subclass, using the REAL base implementation, that exposes
// the two protected members TEST-146 and TEST-153(d) are about. It adds no
// behaviour: it overrides nothing, and every line it exercises is production's.
class ProbeService : public CloudService
{
    Q_OBJECT

  public:
    explicit ProbeService(Context* context, QNetworkAccessManager* injected = nullptr) : CloudService(context, injected)
    {
    }

    CloudService* clone(Context* context) override { return new ProbeService(context); }
    QImage logo() const override { return QImage(); }
    QString id() const override { return QStringLiteral("PROBE"); }
    QString uiName() const override { return QStringLiteral("Probe"); }

    using CloudService::blockingRequest;
    using CloudService::nam;

    int wireCalls = 0;
    QNetworkAccessManager* wiredWith = nullptr;

  protected:
    void wireNam(QNetworkAccessManager* manager) override
    {
        wireCalls++;
        wiredWith = manager;
    }
};

// ===========================================================================
// THE CHILD-MODE BODIES, and the parent-side launcher.
// ===========================================================================
namespace childmode {

// --- the two death tests ---------------------------------------------------
//
// Each one sets up the ONE illegal condition and then performs the operation
// that must be rejected. Both are expected to die inside production code with a
// named sentinel; neither may die of anything else, which is what the parent's
// five separate assertions are for.

int runOffAffinityNam()
{
    announce(OffAffinityNam);

    // A service that genuinely lives somewhere else. The thread is STARTED, so
    // this is a real affinity difference and not a technicality about an object
    // parked on an inert QThread.
    QThread worker;
    worker.start();

    ProbeService* service = new ProbeService(nullptr);
    service->moveToThread(&worker);

    // First use, from the WRONG thread. This must not return.
    service->nam();

    // Only reached if the guard did not fire - which is itself the failure the
    // parent is looking for, reported distinguishably rather than as a silent
    // zero exit.
    fprintf(stderr, "DEC040_GUARD_DID_NOT_FIRE off-affinity nam() returned\n");
    fflush(stderr);
    worker.quit();
    worker.wait();
    return 90;
}

int runInjectedNamAffinity()
{
    announce(InjectedNamAffinity);

    // A manager built on, and belonging to, another thread.
    QThread owner;
    owner.start();

    net::FakeNam* foreign = new net::FakeNam();
    foreign->moveToThread(&owner);

    // Adoption must be refused. This must not return.
    ProbeService* service = new ProbeService(nullptr, foreign);
    Q_UNUSED(service);

    fprintf(stderr, "DEC040_GUARD_DID_NOT_FIRE injected manager was adopted\n");
    fflush(stderr);
    owner.quit();
    owner.wait();
    return 91;
}

// --- row 7, isolated -------------------------------------------------------
//
// QCoreApplication::exit() sets the thread's quitNow flag, and QEventLoop::exec()
// then returns -1 immediately while it is set. Only QCoreApplication::exec()
// clears it, and this binary never enters one - so the poison is PERMANENT for
// the process that swallows it. Running this in the main test process made every
// later nested event loop unusable and forced the slot to be declared last.
//
// Here it runs in a child that exists only for this, and dies with it. The
// parent process never sets the flag at all.
int runRow7SpuriousExec()
{
    announce(Row7SpuriousExec);

    fixture::Harness harness;
    net::reset();
    net::FakeNam* nam = new net::FakeNam();
    ProbeService service(harness.context(), nam);

    // APPARATUS CHECK: a nested loop works here BEFORE the poison, so what the
    // assertion below measures is production's handling of a spurious return
    // and not a loop that was already broken.
    {
        QEventLoop probe;
        QTimer::singleShot(0, &probe, &QEventLoop::quit);
        probe.exec();
    }

    net::Plan plan;
    plan.behaviour = net::Silent;
    plan.body = "MUST NEVER BE READ";
    net::plans << plan;

    QCoreApplication::exit(0);

    const RequestResult result = service.blockingRequest(nam->get(QNetworkRequest(QUrl("http://x/"))), 5000);

    // WE DO NOT KNOW THAT THE REQUEST SUCCEEDED, SO WE MUST NOT SAY THAT IT DID.
    // Reported as machine-readable facts; the parent does the asserting.
    printf("DEC040_ROW7 outcomeIsNetworkError=%d ok=%d bodyEmpty=%d hasNoOutcomeText=%d\n",
           result.outcome == RequestOutcome::NetworkError ? 1 : 0, result.ok() ? 1 : 0, result.body.isEmpty() ? 1 : 0,
           result.errorString.contains(QStringLiteral("without an outcome")) ? 1 : 0);
    printf("DEC040_ROW7_END\n");
    fflush(stdout);
    return 0;
}

} // namespace childmode

// ===========================================================================
// THE PARENT SIDE - launching a child and reading its verdict.
// ===========================================================================
namespace childproc {

struct Result
{
    bool started = false;
    bool finishedInTime = false;
    int exitCode = -1;
    QProcess::ExitStatus exitStatus = QProcess::NormalExit;
    QString out;
    QString err;

    bool entered(childmode::Mode m) const
    {
        return err.contains(QString::fromLatin1(childmode::kEnteredPrefix) + QString::fromLatin1(childmode::name(m)));
    }
    bool died() const { return exitStatus == QProcess::CrashExit || exitCode != 0; }

    // stderr with the child's own mode-announcement removed, so a slot can
    // assert that NOTHING ELSE was written. Without this the strongest
    // available claim would be "the one warning I thought to grep for is
    // absent", which is a much weaker statement than "the pre-main window is
    // silent" - and only the second is what TEST-153 (a) actually asserts.
    QString errWithoutMarkers() const
    {
        QStringList kept;
        const QStringList all = err.split(QLatin1Char('\n'));
        for (const QString& line : all) {
            if (line.startsWith(QString::fromLatin1(childmode::kEnteredPrefix)))
                continue;
            if (line.trimmed().isEmpty())
                continue;
            kept << line;
        }
        return kept.join(QLatin1Char('\n'));
    }
    bool sawAsan() const { return err.contains(QStringLiteral("AddressSanitizer")); }
    bool guardFailedToFire() const { return err.contains(QStringLiteral("DEC040_GUARD_DID_NOT_FIRE")); }
};

// Strict timeout on every child: a guard that HANGS instead of firing must fail
// the test, not stall the suite until CI kills it.
static const int kChildTimeoutMs = 30000;

Result run(childmode::Mode mode)
{
    Result r;

    // RECURSION GUARD. If this process is itself a child, refuse to spawn.
    if (!qEnvironmentVariableIsEmpty(childmode::kDepthEnv)) {
        r.err = QStringLiteral("refusing to spawn: already a child");
        return r;
    }

    QProcessEnvironment env = QProcessEnvironment::systemEnvironment();
    env.insert(QString::fromLatin1(childmode::kDepthEnv), QStringLiteral("1"));
    // Inherited EXPLICITLY rather than by hope: the child must run under the
    // same sanitizer and platform settings as the parent, or "no AddressSanitizer
    // output" in the child says nothing about the build the gate cares about.
    env.insert(QStringLiteral("ASAN_OPTIONS"),
               qEnvironmentVariable("ASAN_OPTIONS", QStringLiteral("detect_leaks=0:abort_on_error=0:halt_on_error=1")));
    env.insert(QStringLiteral("QT_QPA_PLATFORM"), qEnvironmentVariable("QT_QPA_PLATFORM", QStringLiteral("offscreen")));

    QProcess child;
    child.setProcessEnvironment(env);
    child.setProgram(QCoreApplication::applicationFilePath()); // never argv[0]
    child.setArguments(
        QStringList() << (QString::fromLatin1(childmode::kFlag) + QString::fromLatin1(childmode::name(mode))));
    child.start();

    r.started = child.waitForStarted(kChildTimeoutMs);
    if (!r.started)
        return r;

    r.finishedInTime = child.waitForFinished(kChildTimeoutMs);
    if (!r.finishedInTime) {
        child.kill();
        child.waitForFinished(kChildTimeoutMs);
        r.err = QString::fromLocal8Bit(child.readAllStandardError());
        return r;
    }

    r.exitCode = child.exitCode();
    r.exitStatus = child.exitStatus();
    r.out = QString::fromLocal8Bit(child.readAllStandardOutput());
    r.err = QString::fromLocal8Bit(child.readAllStandardError());
    return r;
}

// The five conditions a death test must satisfy. Any one of them missing means
// the child died of something other than the invariant under test, and the row
// must not pass on "it exited nonzero".
void assertDiedWith(const Result& r, childmode::Mode mode, const QString& sentinel)
{
    QVERIFY2(r.started, "child process did not start");
    QVERIFY2(r.finishedInTime, "child process did not finish inside its timeout");
    QVERIFY2(r.entered(mode), qPrintable(QStringLiteral("child never announced mode %1; stderr was:\n%2")
                                             .arg(QString::fromLatin1(childmode::name(mode)), r.err)));
    QVERIFY2(!r.guardFailedToFire(), qPrintable(QStringLiteral("the guard did not fire:\n%1").arg(r.err)));
    QVERIFY2(r.died(), "child exited 0 - the invariant did not stop it");
    QVERIFY2(r.err.contains(sentinel),
             qPrintable(QStringLiteral("stderr did not carry %1; stderr was:\n%2").arg(sentinel, r.err)));
    QVERIFY2(!r.sawAsan(), qPrintable(QStringLiteral("AddressSanitizer reported a finding:\n%1").arg(r.err)));
}

} // namespace childproc

// ===========================================================================
// PART 4 - THE TESTS
// ===========================================================================
class TestCloudProviderWatchdog : public QObject
{
    Q_OBJECT

  private slots:

    void initTestCase();

    // --- TEST-153 (a), out of process --------------------------------------
    void preMain_captureApparatusWorks(); // positive control, runs FIRST
    void preMain_exactMigratedIdentitiesAreRegistered();
    void preMain_noManagerAndNoWarnings();

    // --- TEST-153 (d) and its negative cases -------------------------------
    void manager_offAffinityFirstUseIsRejected();
    void manager_injectedAffinityMismatchIsRejected();

    // --- TEST-143 / 144 / 145 / 150, per site ------------------------------
    void site_bounded_data();
    void site_bounded(); // TEST-143
    void site_noFalseSuccess_data();
    void site_noFalseSuccess(); // TEST-144
    void site_replyDestroyedExactlyOnce_data();
    void site_replyDestroyedExactlyOnce(); // TEST-145
    void site_respondingEndpointSucceeds_data();
    void site_respondingEndpointSucceeds(); // TEST-150

    // --- TEST-147 (PARTIAL: timeout rows only) -----------------------------
    void shapeBC_timeoutBreaksTheListing_data();
    void shapeBC_timeoutBreaksTheListing();

    // --- TEST-147 (Stage 2 piece 3: the Cancelled-mid-listing counterpart) -
    void shapeBC_cancellationBreaksTheListing_data();
    void shapeBC_cancellationBreaksTheListing();

    // --- TEST-148 (Stage 2 piece 3b: the default-token GUI-thread sites) ---
    void guiThread_defaultTokenNeverCancels_data();
    void guiThread_defaultTokenNeverCancels();

    // --- TEST-146 (BUILT IN FULL: all seven rows, including 4 and 6-cancel) ---
    void table_row1_naturalFinishNoError();
    void table_row2_naturalFinishWithError();
    void table_row3_timeoutNoFinish();
    void table_row4_cancellationNoFinishObserved();
    void table_row5_timerAndFinishTogether();
    void table_row6_abortInducedFinishLeavesOutcomeUnchanged();
    void table_row6_cancelInducedFinishLeavesOutcomeUnchanged();

    // --- TEST-149 (Stage 2 cancellation genericity / lifetime / interval) -----
    void cancel_observationIntervalWithinBound();

    // --- TEST-152 ----------------------------------------------------------
    void strava_completionIsExactlyOne_data();
    void strava_completionIsExactlyOne();
    void strava_partialRideIsNeitherStagedNorSuccessful();

    // --- TEST-151 ----------------------------------------------------------
    void autoDownload_distinguishesEmptyWithErrors();

    // --- TEST-159 (DEC-043 Piece 1) -----------------------------------------
    void readComplete_afterOwnerTornDownMustNotDereferenceFreedMemory();

    // --- TEST-160 (DEC-043 Piece 2) -----------------------------------------
    void athleteClose_cancelsAndJoinsAutoDownloadThenDeletesIt();

    // --- TEST-153 (b)-(e) --------------------------------------------------
    void manager_notBuiltUntilFirstUse_data();
    void manager_notBuiltUntilFirstUse();
    void manager_injectedUsedExactlyAsSupplied_data();
    void manager_injectedUsedExactlyAsSupplied();
    void manager_destroyedExactlyOnce_data();
    void manager_destroyedExactlyOnce();
    void manager_createdOnceOnFirstUse();
    void manager_createdInObjectAffinityThread();
    void manager_wiredExactlyOnce();

    // --- TEST-146 row 7, and the proof that it no longer poisons anything ---
    //
    // Row 7 needs QCoreApplication::exit(), which sets the thread's quitNow flag
    // permanently for this process. It therefore runs in a CHILD, and this slot
    // only reads the child's verdict. The declaration order of these two no
    // longer matters - which is the point, and the slot AFTER it proves so by
    // running an ordinary nested event loop that the old arrangement would have
    // made impossible.
    void table_row7_spuriousExecReturn_isolated();
    void nestedEventLoopStillWorksAfterRow7();

  private:
    fixture::Harness* harness_ = nullptr;
};

void TestCloudProviderWatchdog::initTestCase()
{
    // Nothing pre-main is asserted here any more. The pre-main claims are made
    // against a CHILD process (below), because the only safe way to observe the
    // state left by static initialisation is from outside the process that has
    // already moved past it.
    QVERIFY2(!QCoreApplication::applicationFilePath().isEmpty(),
             "applicationFilePath() is empty; the child-mode launcher needs it");
}

// ---------------------------------------------------------------------------
// TEST-153 (a) - out of process.
// ---------------------------------------------------------------------------

void TestCloudProviderWatchdog::preMain_captureApparatusWorks()
{
    // THE POSITIVE CONTROL, AND IT RUNS BEFORE THE ASSERTION IT UNDERWRITES.
    //
    // The slot below asserts a warning is ABSENT from a child's stderr. That
    // reading is worthless unless this pipe can carry such a warning when one is
    // really emitted. So a child emits exactly the text in question, before
    // QApplication, and the parent proves it arrives.
    const childproc::Result r = childproc::run(childmode::StderrControl);

    QVERIFY2(r.started, "control child did not start");
    QVERIFY2(r.finishedInTime, "control child did not finish inside its timeout");
    QVERIFY(r.entered(childmode::StderrControl));
    QCOMPARE(r.exitCode, 0);
    QVERIFY2(r.err.contains(QStringLiteral("invalid nullptr parameter")),
             qPrintable(QStringLiteral("stderr capture is broken; stderr was:\n%1").arg(r.err)));
    QVERIFY2(!r.sawAsan(), qPrintable(r.err));
}

void TestCloudProviderWatchdog::preMain_exactMigratedIdentitiesAreRegistered()
{
    // IDENTITIES, NOT A COUNT. The previous form of this check was
    // `serviceCount() >= 12`, which passes if the wrong twelve registered, and
    // passes if one of the migrated providers vanished and two unrelated
    // services appeared. Every expected template is now named and proven
    // present individually.
    const childproc::Result r = childproc::run(childmode::PreMainCensus);

    QVERIFY2(r.started, "census child did not start");
    QVERIFY2(r.finishedInTime, "census child did not finish inside its timeout");
    QVERIFY(r.entered(childmode::PreMainCensus));
    QCOMPARE(r.exitCode, 0);
    QVERIFY2(r.out.contains(QStringLiteral("DEC040_CENSUS_END")),
             qPrintable(QStringLiteral("census did not run to completion:\n%1").arg(r.out)));

    for (int i = 0; i < kExpectedRegisteredMigratedCount; i++) {
        const QString expected = QStringLiteral("DEC040_TEMPLATE id=[%1] present=1")
                                     .arg(QString::fromLatin1(kExpectedRegisteredMigrated[i].id));
        QVERIFY2(r.out.contains(expected),
                 qPrintable(QStringLiteral("%1 (class %2) was not registered pre-main; census was:\n%3")
                                .arg(expected, QString::fromLatin1(kExpectedRegisteredMigrated[i].cls), r.out)));
    }

    // THE EXCEPTION IS REPORTED, NOT ABSORBED. PolarFlow's registration block is
    // `#if 0`'d out, so it is expected to be ABSENT - and that is asserted as its
    // own fact rather than hidden inside a tolerance on a total.
    QVERIFY2(r.out.contains(QStringLiteral("DEC040_EXCEPTION id=[PolarFlow] present=0")),
             qPrintable(QStringLiteral("PolarFlow's recorded registration exception does not hold:\n%1").arg(r.out)));
}

void TestCloudProviderWatchdog::preMain_noManagerAndNoWarnings()
{
    const childproc::Result r = childproc::run(childmode::PreMainCensus);

    QVERIFY2(r.started && r.finishedInTime, "census child did not run");
    QVERIFY(r.entered(childmode::PreMainCensus));
    QCOMPARE(r.exitCode, 0);

    // (a) NO MANAGER - PER TEMPLATE, not as an aggregate count. An aggregate of
    // zero is also what you get if the census enumerated nothing at all.
    for (int i = 0; i < kExpectedRegisteredMigratedCount; i++) {
        const QString expected = QStringLiteral("DEC040_TEMPLATE id=[%1] present=1 namCreated=0")
                                     .arg(QString::fromLatin1(kExpectedRegisteredMigrated[i].id));
        QVERIFY2(r.out.contains(expected),
                 qPrintable(QStringLiteral("%1 (class %2) held a manager pre-main; census was:\n%3")
                                .arg(expected, QString::fromLatin1(kExpectedRegisteredMigrated[i].cls), r.out)));
    }

    // (a) NO WARNINGS - AND THE CLAIM IS EXACTLY THAT, NOT "no warning of the
    // one kind I remembered to look for".
    //
    // Eager construction in the base constructor emits two
    // "QObject::connect(QObject, Unknown): invalid nullptr parameter" per
    // service as the manager tries to wire itself to an application object that
    // does not exist yet. Grepping for that specific string would prove only
    // its absence and would say nothing about any OTHER diagnostic the pre-main
    // window might emit. So the assertion is made on the whole channel: with
    // the child's own mode-announcement removed, the remaining stderr must be
    // EMPTY. The capture path is proven live by preMain_captureApparatusWorks,
    // which runs first and shows this same pipe carrying a real warning.
    const QString residualErr = r.errWithoutMarkers();
    QVERIFY2(residualErr.isEmpty(),
             qPrintable(QStringLiteral("the pre-main window was not silent; stderr was:\n%1").arg(residualErr)));
}

// ---------------------------------------------------------------------------
// TEST-153 (d) negatives - the affinity contract, proven by rejection.
// ---------------------------------------------------------------------------

void TestCloudProviderWatchdog::manager_offAffinityFirstUseIsRejected()
{
    // The positive case (nam() called correctly, on the object's own thread)
    // lives in manager_createdInObjectAffinityThread. This is the other half:
    // the SAME call from the wrong thread must be refused, deterministically,
    // in a build with -DNDEBUG - which is why the production check is qFatal()
    // and not Q_ASSERT.
    const childproc::Result r = childproc::run(childmode::OffAffinityNam);
    childproc::assertDiedWith(r, childmode::OffAffinityNam, QStringLiteral("DEC040_NAM_OFF_AFFINITY"));
}

void TestCloudProviderWatchdog::manager_injectedAffinityMismatchIsRejected()
{
    // An injected manager belonging to another thread must be REJECTED rather
    // than silently reparented or moved.
    const childproc::Result r = childproc::run(childmode::InjectedNamAffinity);
    childproc::assertDiedWith(r, childmode::InjectedNamAffinity, QStringLiteral("DEC040_INJECTED_NAM_AFFINITY"));
}

// ---------------------------------------------------------------------------
// The 22-site data set, shared by TEST-143/144/145/150.
// ---------------------------------------------------------------------------
static void addAllSites()
{
    QTest::addColumn<int>("site");
    for (int s = Site_Dropbox_createFolder; s <= Site_COUNT_; s++)
        QTest::newRow(siteName(static_cast<SiteId>(s))) << s;
}

// ---------------------------------------------------------------------------
// TEST-143 - a never-responding endpoint yields a bounded failure.
// ---------------------------------------------------------------------------
void TestCloudProviderWatchdog::site_bounded_data()
{
    addAllSites();
}

void TestCloudProviderWatchdog::site_bounded()
{
    QFETCH(int, site);
    const SiteId id = static_cast<SiteId>(site);

    QElapsedTimer elapsed;
    elapsed.start();
    const SiteOutcome out = fixture::driveSite(id, silentPlans(id), 60);
    const qint64 took = elapsed.elapsed();

    // The apparatus never had to step in: production ended its own wait.
    QVERIFY2(!net::rescueFired, "the harness had to end the wait - production did not bound it");

    // TIGHT. The override is 60ms and the site makes at most two requests, so
    // anything past half a second means the fast path was not taken. If the
    // watchdog were armed after exec() this is 3000ms (the rescue) or never.
    QVERIFY2(took < 800, qPrintable(QStringLiteral("wait took %1ms, expected < 800ms").arg(took)));

    // And it did reach the network, so the row is about the site and not about
    // an early return that never issued a request.
    QVERIFY2(net::repliesCreated >= 1, "the site never issued a request - it early-returned");
}

// ---------------------------------------------------------------------------
// TEST-144 - non-Finished implies empty body AND the caller reports failure.
// ---------------------------------------------------------------------------
void TestCloudProviderWatchdog::site_noFalseSuccess_data()
{
    addAllSites();
}

void TestCloudProviderWatchdog::site_noFalseSuccess()
{
    QFETCH(int, site);
    const SiteId id = static_cast<SiteId>(site);

    // Both halves of "non-Finished": the endpoint that never answers, and the
    // one that answers with an error.
    const SiteOutcome timedOut = fixture::driveSite(id, silentPlans(id), 60);
    QVERIFY(!net::rescueFired);
    QVERIFY2(!timedOut.callerSuccess,
             qPrintable(QStringLiteral("%1 reported SUCCESS after a timeout").arg(siteName(id))));
    QCOMPARE(timedOut.items, 0);
    if (timedOut.hasErrorsChannel)
        QVERIFY2(!timedOut.errors.isEmpty(),
                 qPrintable(QStringLiteral("%1 timed out silently - nothing on its errors channel").arg(siteName(id))));

    const SiteOutcome errored = fixture::driveSite(id, erroredPlans(id), 5000);
    QVERIFY2(!errored.callerSuccess,
             qPrintable(QStringLiteral("%1 reported SUCCESS after a network error").arg(siteName(id))));
    QCOMPARE(errored.items, 0);
    if (errored.hasErrorsChannel)
        QVERIFY2(!errored.errors.isEmpty(),
                 qPrintable(QStringLiteral("%1 failed silently - nothing on its errors channel").arg(siteName(id))));
}

// ---------------------------------------------------------------------------
// TEST-145 - exactly-once destruction of the ACTUAL reply objects.
// ---------------------------------------------------------------------------
void TestCloudProviderWatchdog::site_replyDestroyedExactlyOnce_data()
{
    addAllSites();
}

void TestCloudProviderWatchdog::site_replyDestroyedExactlyOnce()
{
    QFETCH(int, site);
    const SiteId id = static_cast<SiteId>(site);

    const int skip = repliesNotOwnedByBlockingRequest(id);

    // Every reply blockingRequest was handed must be destroyed, exactly once, on
    // all three paths. `skip` excludes the async readFile reply at site 17, which
    // blockingRequest never owned - see repliesNotOwnedByBlockingRequest.
    const char* paths[] = {"responding", "silent", "errored"};
    for (int path = 0; path < 3; path++) {
        const QList<net::Plan> plans = path == 0 ? okPlans(id) : path == 1 ? silentPlans(id) : erroredPlans(id);
        fixture::driveSite(id, plans, path == 1 ? 60 : 5000);
        QVERIFY2(!net::rescueFired, paths[path]);
        QVERIFY2(
            net::repliesCreated > skip,
            qPrintable(QStringLiteral("%1 [%2]: no reply reached blockingRequest").arg(siteName(id)).arg(paths[path])));
        QCOMPARE(net::repliesDestroyed, net::repliesCreated - skip);
        for (int i = skip; i < net::issued.size(); i++)
            QVERIFY2(net::issued.at(i).isNull(),
                     qPrintable(QStringLiteral("%1 [%2]: reply %3 outlived the call that owned it")
                                    .arg(siteName(id))
                                    .arg(paths[path])
                                    .arg(i)));
    }
}

// ---------------------------------------------------------------------------
// TEST-150 - THE NON-VACUITY CONTROL.
//
// Without this, TEST-143 and TEST-144 are both satisfied by a helper that
// returns TimedOut unconditionally, and the whole file would be measuring
// nothing. This row insists that a responding endpoint gets all the way through
// to a parsed result at every one of the 22 sites.
// ---------------------------------------------------------------------------
void TestCloudProviderWatchdog::site_respondingEndpointSucceeds_data()
{
    addAllSites();
}

void TestCloudProviderWatchdog::site_respondingEndpointSucceeds()
{
    QFETCH(int, site);
    const SiteId id = static_cast<SiteId>(site);

    const SiteOutcome out = fixture::driveSite(id, okPlans(id), 5000);

    QVERIFY2(out.callerSuccess, qPrintable(QStringLiteral("%1 failed against a RESPONDING endpoint; errors: %2")
                                               .arg(siteName(id))
                                               .arg(out.errors.join("; "))));
    if (out.hasErrorsChannel)
        QVERIFY2(out.errors.isEmpty(),
                 qPrintable(
                     QStringLiteral("%1 reported errors on success: %2").arg(siteName(id)).arg(out.errors.join("; "))));

    // THE BODY GOT THROUGH, not merely "the call returned true": the entry the
    // response described came out of the site's own parser.
    const int expectedItems = okItems(id);
    if (expectedItems >= 0)
        QCOMPARE(out.items, expectedItems);

    // And the shape census, measured: how many waits this site really performs.
    QCOMPARE(out.requests, okRequests(id));
}

// ---------------------------------------------------------------------------
// TEST-147 - the TIMEOUT rows over the shape-B/C set.
//
// The set is {2, 4, 5, 7, 10, 16, 17}, re-derived by reading each enclosing
// function - see the shape census at the top of this file. The Cancelled-
// mid-listing counterpart (Stage 2 piece 3) is
// shapeBC_cancellationBreaksTheListing_data()/() immediately below this test.
//
// The rule: a non-Finished outcome mid-listing must BREAK the loop and
// propagate. Never continue. So a listing whose SECOND page times out must come
// back with the first page's entries AND an error - never with the first page's
// entries alone, which reads exactly like a complete short directory.
// ---------------------------------------------------------------------------
void TestCloudProviderWatchdog::shapeBC_timeoutBreaksTheListing_data()
{
    QTest::addColumn<int>("site");
    QTest::addColumn<int>("silentIndex");   // which request goes quiet
    QTest::addColumn<int>("expectedItems"); // what the earlier pages produced
    QTest::addColumn<bool>("canPropagate"); // does this site HAVE a failure channel?

    QTest::newRow("02 Dropbox::readdir") << int(Site_Dropbox_readdir) << 1 << 1 << true;
    QTest::newRow("04 Azum::readdir") << int(Site_Azum_readdir) << 1 << 1 << true;
    // *** RESIDUAL, DECLARED. Azum::listAthletes returns QList<CloudServiceAthlete>
    // *** and NOTHING ELSE - no errors list, no bool, no out-parameter. So a
    // *** listing that broke after page one is, at this site, indistinguishable
    // *** from a complete one-page listing, and no test can make it otherwise
    // *** without changing the signature. What IS assertable here is that the
    // *** loop BROKE rather than paging on, and that is what this row asserts.
    // *** The same gap applies to site 13, Nolio::listAthletes, which is shape A
    // *** and therefore has no mid-listing row at all.
    QTest::newRow("05 Azum::listAthletes") << int(Site_Azum_listAthletes) << 1 << 1 << false;
    QTest::newRow("07 SportTracks::readdir") << int(Site_SportTracks_readdir) << 1 << 25 << true;
    QTest::newRow("10 Xert::readActivityDetail") << int(Site_Xert_readActivityDetail) << 1 << 0 << true;
    QTest::newRow("16 Strava::readdir") << int(Site_Strava_readdir) << 1 << 30 << true;
}

void TestCloudProviderWatchdog::shapeBC_timeoutBreaksTheListing()
{
    QFETCH(int, site);
    QFETCH(int, silentIndex);
    QFETCH(int, expectedItems);
    QFETCH(bool, canPropagate);
    const SiteId id = static_cast<SiteId>(site);

    // A FIRST page that is full enough to make the site ask for a second one,
    // then silence.
    QList<net::Plan> plans;
    net::Plan first;
    switch (id) {
    case Site_Dropbox_readdir:
        first.body = "{\"entries\":[{\".tag\":\"file\",\"path_display\":\"/gc/2020_01_01_10_00_00.json\","
                     "\"bytes\":10}],\"has_more\":true,\"cursor\":\"CURSOR\"}";
        break;
    case Site_Azum_readdir:
        first.body = "{\"next\":\"https://training.azum.com/next\",\"results\":[{\"id\":\"7\","
                     "\"export_name\":\"a.fit\",\"start\":\"2020-01-01T10:00:00\",\"distance\":1000,"
                     "\"timer_time\":\"P0DT00H25M57S\"}]}";
        break;
    case Site_Azum_listAthletes:
        first.body = "{\"next\":\"https://training.azum.com/next\",\"results\":[{\"user\":7,"
                     "\"full_name\":\"A Rider\"}]}";
        break;
    case Site_SportTracks_readdir: {
        // SportTracks only asks for another page when it received a FULL page of
        // 25, so the fixture has to hand it 25.
        QString items;
        for (int i = 0; i < 25; i++) {
            if (i)
                items += ",";
            items += QStringLiteral("{\"duration\":\"600.00\",\"name\":\"Cycling\","
                                    "\"start_time\":\"2020-01-01T10:%1:00+00:00\","
                                    "\"total_distance\":\"1000\","
                                    "\"uri\":\"https://api.sporttracks.mobi/api/v2/fitnessActivities/%2\","
                                    "\"user_id\":\"1\"}")
                         .arg(i, 2, 10, QChar('0'))
                         .arg(i);
        }
        first.body = QStringLiteral("{\"items\":[%1]}").arg(items).toUtf8();
        break;
    }
    case Site_Xert_readActivityDetail:
        first.body = body::xertDir; // one activity -> one detail request
        break;
    case Site_Strava_readdir: {
        QString items;
        for (int i = 0; i < 30; i++) {
            if (i)
                items += ",";
            items += QStringLiteral("{\"id\":%1,\"name\":\"n\",\"distance\":1000,"
                                    "\"elapsed_time\":600,"
                                    "\"start_date_local\":\"2020-01-01T10:%2:00\"}")
                         .arg(i)
                         .arg(i, 2, 10, QChar('0'));
        }
        first.body = QStringLiteral("[%1]").arg(items).toUtf8();
        break;
    }
    default:
        QFAIL("unhandled shape-B/C row");
    }
    plans << first;
    for (int i = 1; i <= silentIndex; i++) {
        net::Plan silent;
        silent.behaviour = net::Silent;
        plans << silent;
    }

    const SiteOutcome out = fixture::driveSite(id, plans, 60);

    QVERIFY2(!net::rescueFired, "the mid-listing wait was not bounded");

    // IT BROKE OUT. If the loop had CONTINUED it would have asked for a third
    // page (and a fourth, and ...), so the request count pins the break.
    QCOMPARE(out.requests, silentIndex + 1);

    // AND IT PROPAGATED. The pages that did arrive are still returned - that is
    // deliberate, they are real - but the caller is told the listing is
    // incomplete rather than being handed a short directory that reads as whole.
    QCOMPARE(out.items, expectedItems);
    if (canPropagate) {
        QVERIFY2(!out.errors.isEmpty(), qPrintable(QStringLiteral("%1 ended its listing SILENTLY").arg(siteName(id))));
        QVERIFY(!out.callerSuccess);
    }
}

// ---------------------------------------------------------------------------
// TEST-147 (Stage 2 piece 3) - the Cancelled counterpart of the row above.
//
// Same rule, different terminator: a listing whose second page is cancelled
// mid-wait (rather than timing out) must ALSO break the loop and propagate,
// never continue. The mechanism that actually reaches RequestOutcome::Cancelled
// is table_row4_cancellationNoFinishObserved's: a shared_ptr<atomic_bool> flag
// flipped ~50ms in from a zero-delay QTimer::singleShot, observed by
// blockingRequest's own cancelPoll timer (250ms cadence) well inside the
// generous 5000ms timeout override - so a real timeout cannot be what ends the
// wait, only cancellation can. driveSite's own CancelToken parameter (added
// alongside this test) plumbs the token onto the real provider object before
// the site-level call runs, the same way setRequestTimeoutOverrideMs already
// did for the timeout row above.
//
// The first-page fixture bodies, per-site expected item counts and
// canPropagate values are intentionally COPIED from
// shapeBC_timeoutBreaksTheListing_data/() rather than shared through a common
// helper, with ONE deliberate exception: site 10 (Xert::readActivityDetail's
// per-activity for loop). See the comment on that case below for why a single-
// activity fixture - which is what the timeout row uses - cannot distinguish
// `return` from `continue` at that site, and therefore cannot be reused
// as-is for a real mutation-proof here.
// ---------------------------------------------------------------------------
void TestCloudProviderWatchdog::shapeBC_cancellationBreaksTheListing_data()
{
    QTest::addColumn<int>("site");
    QTest::addColumn<int>("silentIndex");   // which request goes quiet, then cancelled
    QTest::addColumn<int>("expectedItems"); // what the earlier pages produced
    QTest::addColumn<bool>("canPropagate"); // does this site HAVE a failure channel?

    QTest::newRow("02 Dropbox::readdir") << int(Site_Dropbox_readdir) << 1 << 1 << true;
    QTest::newRow("04 Azum::readdir") << int(Site_Azum_readdir) << 1 << 1 << true;
    // *** RESIDUAL, DECLARED - identical gap to the timeout row's, for the
    // *** identical reason: Azum::listAthletes returns QList<CloudServiceAthlete>
    // *** and nothing else, so a listing that broke after page one reads exactly
    // *** like a complete one-page listing. What IS assertable is that the loop
    // *** BROKE rather than paging on, which is what this row asserts.
    QTest::newRow("05 Azum::listAthletes") << int(Site_Azum_listAthletes) << 1 << 1 << false;
    QTest::newRow("07 SportTracks::readdir") << int(Site_SportTracks_readdir) << 1 << 25 << true;
    // Site 10 uses a TWO-activity first page (the timeout row's fixture has
    // only one) - see the test body's switch for why.
    QTest::newRow("10 Xert::readActivityDetail") << int(Site_Xert_readActivityDetail) << 1 << 0 << true;
    QTest::newRow("16 Strava::readdir") << int(Site_Strava_readdir) << 1 << 30 << true;
}

void TestCloudProviderWatchdog::shapeBC_cancellationBreaksTheListing()
{
    QFETCH(int, site);
    QFETCH(int, silentIndex);
    QFETCH(int, expectedItems);
    QFETCH(bool, canPropagate);
    const SiteId id = static_cast<SiteId>(site);

    QList<net::Plan> plans;
    net::Plan first;
    switch (id) {
    case Site_Dropbox_readdir:
        first.body = "{\"entries\":[{\".tag\":\"file\",\"path_display\":\"/gc/2020_01_01_10_00_00.json\","
                     "\"bytes\":10}],\"has_more\":true,\"cursor\":\"CURSOR\"}";
        break;
    case Site_Azum_readdir:
        first.body = "{\"next\":\"https://training.azum.com/next\",\"results\":[{\"id\":\"7\","
                     "\"export_name\":\"a.fit\",\"start\":\"2020-01-01T10:00:00\",\"distance\":1000,"
                     "\"timer_time\":\"P0DT00H25M57S\"}]}";
        break;
    case Site_Azum_listAthletes:
        first.body = "{\"next\":\"https://training.azum.com/next\",\"results\":[{\"user\":7,"
                     "\"full_name\":\"A Rider\"}]}";
        break;
    case Site_SportTracks_readdir: {
        // SportTracks only asks for another page when it received a FULL page of
        // 25, so the fixture has to hand it 25 - identical to the timeout row.
        QString items;
        for (int i = 0; i < 25; i++) {
            if (i)
                items += ",";
            items += QStringLiteral("{\"duration\":\"600.00\",\"name\":\"Cycling\","
                                    "\"start_time\":\"2020-01-01T10:%1:00+00:00\","
                                    "\"total_distance\":\"1000\","
                                    "\"uri\":\"https://api.sporttracks.mobi/api/v2/fitnessActivities/%2\","
                                    "\"user_id\":\"1\"}")
                         .arg(i, 2, 10, QChar('0'))
                         .arg(i);
        }
        first.body = QStringLiteral("{\"items\":[%1]}").arg(items).toUtf8();
        break;
    }
    case Site_Xert_readActivityDetail:
        // TWO activities, not xertDir's one. With a single activity the for
        // loop's own iteration count is 1 regardless of `return` vs `continue`
        // on the failed detail fetch (there is no second activity left to skip
        // to), so out.requests and out.items are IDENTICAL under both the
        // correct code and a `continue` mutation - the mutation would be
        // invisible to this row's assertions. With two activities, a `continue`
        // mutation lets the loop go on to fetch activity #2's detail too (an
        // extra, unwanted third request), which shows up as out.requests == 3
        // instead of the correct code's out.requests == 2. out.items stays 0
        // either way (neither activity's detail ever succeeds), which is why
        // the request count, not the item count, is this row's killing
        // assertion for site 10.
        first.body = "{\"activities\":["
                     "{\"name\":\"n1\",\"start_date\":{\"date\":\"2020-01-01 10:00:00.000000\"},"
                     "\"path\":\"P1\",\"activity_type\":\"Cycling\"},"
                     "{\"name\":\"n2\",\"start_date\":{\"date\":\"2020-01-01 11:00:00.000000\"},"
                     "\"path\":\"P2\",\"activity_type\":\"Cycling\"}]}";
        break;
    case Site_Strava_readdir: {
        QString items;
        for (int i = 0; i < 30; i++) {
            if (i)
                items += ",";
            items += QStringLiteral("{\"id\":%1,\"name\":\"n\",\"distance\":1000,"
                                    "\"elapsed_time\":600,"
                                    "\"start_date_local\":\"2020-01-01T10:%2:00\"}")
                         .arg(i)
                         .arg(i, 2, 10, QChar('0'));
        }
        first.body = QStringLiteral("[%1]").arg(items).toUtf8();
        break;
    }
    default:
        QFAIL("unhandled shape-B/C row");
    }
    plans << first;
    for (int i = 1; i <= silentIndex; i++) {
        net::Plan silent;
        silent.behaviour = net::Silent;
        plans << silent;
    }

    // The cancellation mechanism - mirrors table_row4_cancellationNoFinishObserved
    // exactly: flip a shared flag ~50ms in, well inside the silent second
    // request's wait, and let blockingRequest's own cancelPoll timer (250ms
    // cadence) observe it. The 5000ms override on driveSite makes sure a real
    // timeout cannot be what ends the wait.
    auto flag = std::make_shared<std::atomic_bool>(false);
    QTimer::singleShot(50, [flag] { flag->store(true, std::memory_order_release); });

    QElapsedTimer elapsed;
    elapsed.start();
    const SiteOutcome out = fixture::driveSite(id, plans, 5000, CancelToken(flag));
    const qint64 took = elapsed.elapsed();

    QVERIFY(!net::rescueFired);
    // Bounded by cancelPoll's cadence, not the 5000ms override or driveSite's
    // own 3000ms apparatus rescue: the flag flips at ~50ms and cancelPoll
    // checks every 250ms, so a correctly-broken listing returns in well under
    // a second even after the extra per-site JSON parsing work.
    QVERIFY2(took < 1500, qPrintable(QStringLiteral("%1 took %2ms").arg(siteName(id)).arg(took)));

    // IT BROKE OUT. If the loop had CONTINUED it would have asked for another
    // page (or, at site 10, another activity's detail), so the request count
    // pins the break.
    QCOMPARE(out.requests, silentIndex + 1);

    // AND IT PROPAGATED. The pages that did arrive are still returned - that is
    // deliberate, they are real - but the caller is told the listing is
    // incomplete rather than being handed a short directory that reads as whole.
    QCOMPARE(out.items, expectedItems);
    if (canPropagate) {
        QVERIFY2(!out.errors.isEmpty(),
                 qPrintable(QStringLiteral("%1 ended its listing SILENTLY after cancellation").arg(siteName(id))));
        QVERIFY(!out.callerSuccess);
    }
}

// ---------------------------------------------------------------------------
// TEST-148 - GUI-thread caller bound with the DEFAULT (never-cancelling) token.
//
// The three GUI-thread call sites named in the DEC-040 Stage 2 C6 census -
// Dropbox::createFolder (site 1), Azum::listAthletes (site 5) and
// Nolio::listAthletes (site 13) - never call setCancelToken() themselves.
// They rely entirely on CloudService's own member default and CancelToken's
// default constructor to make cancellation a no-op. This is a DIFFERENT
// property from TEST-150's site_respondingEndpointSucceeds: that test proves
// the harness is non-vacuous (a responding endpoint gets all the way through)
// at all 22 sites; THIS test exists so there is a targeted, commented,
// specifically-named row per C6 site whose sole job is to catch a regression
// in the default itself - e.g. CancelToken() or CloudService's own
// cancelToken_ member initialisation silently flipping from "no flag = never
// cancels" to "no flag = always cancelled". driveSite() is called WITHOUT a
// cancelToken argument on every row here, so the 4th (defaulted) parameter is
// exercised deliberately, not incidentally.
//
// Each row uses a plan that would succeed if - and only if - cancellation is
// correctly inert for a default-constructed token; the shared okPlans()/
// okItems() helpers already encode exactly that "one responding page" shape
// for these three sites, so this test reuses their bodies rather than
// inventing new fixture JSON.
//
// DELAY IS LOAD-BEARING. cancelSnapshot.cancelled() is only ever consulted
// from inside cancelPoll's OWN timeout lambda (CloudService.cpp), which fires
// on a 250ms (kCancelPollMs) cadence - never at loop.exec() entry. okPlans()'s
// replies deliver with delayMs == 0, i.e. on the very first event-loop
// dispatch, which beats cancelPoll's first tick by a wide margin: the wait
// ends via naturalFinish_ before cancelled() is ever called at all, and a
// "make CancelToken() default to cancelled" mutation would then be INVISIBLE
// to this row - the mutated cancelled() is simply never asked. Each plan here
// is therefore reused from okPlans() but stamped with a delayMs comfortably
// PAST one cancelPoll tick (400ms > 250ms) and comfortably BEFORE the 5000ms
// watchdog override, so cancelPoll fires at least once while the request is
// still outstanding and the mutation has something to bite on.
// ---------------------------------------------------------------------------
void TestCloudProviderWatchdog::guiThread_defaultTokenNeverCancels_data()
{
    QTest::addColumn<int>("site");

    QTest::newRow("01 Dropbox::createFolder") << int(Site_Dropbox_createFolder);
    QTest::newRow("05 Azum::listAthletes") << int(Site_Azum_listAthletes);
    QTest::newRow("13 Nolio::listAthletes") << int(Site_Nolio_listAthletes);
}

void TestCloudProviderWatchdog::guiThread_defaultTokenNeverCancels()
{
    QFETCH(int, site);
    const SiteId id = static_cast<SiteId>(site);

    QList<net::Plan> plans = okPlans(id);
    for (net::Plan& p : plans)
        p.delayMs = 400; // past cancelPoll's 250ms cadence - see comment above

    // No CancelToken argument passed - driveSite's 4th parameter takes its
    // default, std::nullopt, so setCancelToken() is never called at all and
    // `service` is left running on CloudService's own construction-time
    // default for cancelToken_, exactly as every real GUI-thread caller of
    // these three sites does today (ORCH-055: this genuinely exercises the
    // member's own default, not a freshly-constructed CancelToken() standing
    // in for it).
    const SiteOutcome out = fixture::driveSite(id, plans, 5000);

    QVERIFY(!net::rescueFired);
    QVERIFY2(out.callerSuccess, qPrintable(QStringLiteral("%1 failed against a RESPONDING endpoint under the default "
                                                          "(never-cancelling) token; errors: %2")
                                               .arg(siteName(id))
                                               .arg(out.errors.join("; "))));
    if (out.hasErrorsChannel)
        QVERIFY2(out.errors.isEmpty(),
                 qPrintable(
                     QStringLiteral("%1 reported errors on success: %2").arg(siteName(id)).arg(out.errors.join("; "))));

    const int expectedItems = okItems(id);
    if (expectedItems >= 0)
        QCOMPARE(out.items, expectedItems);
}

// ---------------------------------------------------------------------------
// TEST-146 (BUILT IN FULL) - the transition table, at the helper.
//
// These rows are ABOUT blockingRequest's own state machine (naturalFinish_,
// pending_, abortIssued_) and there is no site-level projection of, for example,
// "the timer and the natural finish landed in the same dispatch". ProbeService is
// a real CloudService subclass using the real base implementation.
//
// ROW 4 (cancellation) and ROW 6's Cancelled arm are NOW BUILT. DEC-040 Stage 2
// member-plumbed CancelToken onto CloudService (setCancelToken) and wired a
// repeating cancelPoll timer into blockingRequest's wait loop, so
// RequestOutcome::Cancelled is reachable via the same first-writer-wins
// pending_ slot the watchdog already used for TimedOut - no other code path
// changed. See CloudService.cpp's cancelPoll comment for the declaration-order
// reasoning shared with watchdog.
// ---------------------------------------------------------------------------
void TestCloudProviderWatchdog::table_row1_naturalFinishNoError()
{
    fixture::Harness harness;
    net::reset();
    net::FakeNam* nam = new net::FakeNam();
    ProbeService service(harness.context(), nam);

    net::Plan plan;
    plan.body = "HELLO";
    net::plans << plan;

    const RequestResult result = service.blockingRequest(nam->get(QNetworkRequest(QUrl("http://x/"))), 5000);

    QVERIFY(result.ok());
    QCOMPARE(result.outcome, RequestOutcome::Finished);
    QCOMPARE(result.body, QByteArray("HELLO"));
    QCOMPARE(result.httpStatus, 200);
    net::drainDeferredDeletes();
    QCOMPARE(net::repliesDestroyed, 1);
}

void TestCloudProviderWatchdog::table_row2_naturalFinishWithError()
{
    fixture::Harness harness;
    net::reset();
    net::FakeNam* nam = new net::FakeNam();
    ProbeService service(harness.context(), nam);

    net::Plan plan;
    plan.behaviour = net::Failed;
    plan.error = QNetworkReply::ContentNotFoundError;
    plan.httpStatus = 404;
    plan.body = "NOT THE RIDE YOU WANTED";
    net::plans << plan;

    const RequestResult result = service.blockingRequest(nam->get(QNetworkRequest(QUrl("http://x/"))), 5000);

    // A NATURAL FINISH CARRYING AN ERROR IS NetworkError, NEVER Finished - and
    // the body it carried is NOT handed to the caller.
    QCOMPARE(result.outcome, RequestOutcome::NetworkError);
    QVERIFY(!result.ok());
    QVERIFY(result.body.isEmpty());
    QCOMPARE(result.error, QNetworkReply::ContentNotFoundError);
    QCOMPARE(result.httpStatus, 404);
}

void TestCloudProviderWatchdog::table_row3_timeoutNoFinish()
{
    fixture::Harness harness;
    net::reset();
    net::FakeNam* nam = new net::FakeNam();
    ProbeService service(harness.context(), nam);

    net::Plan plan;
    plan.behaviour = net::Silent;
    net::plans << plan;

    net::Rescue rescue(3000);
    QElapsedTimer elapsed;
    elapsed.start();
    const RequestResult result = service.blockingRequest(nam->get(QNetworkRequest(QUrl("http://x/"))), 60);
    const qint64 took = elapsed.elapsed();

    QVERIFY(!net::rescueFired);
    QCOMPARE(result.outcome, RequestOutcome::TimedOut);
    QVERIFY(result.body.isEmpty());
    QVERIFY2(took < 800, qPrintable(QStringLiteral("took %1ms").arg(took)));
    QVERIFY(result.errorString.contains(QStringLiteral("Timed out")));
    // And the reply WAS aborted, which is what makes the abort-induced finish of
    // row 6 reachable at all.
    QCOMPARE(net::abortCalls, 1);
}

void TestCloudProviderWatchdog::table_row4_cancellationNoFinishObserved()
{
    // ROW 4: cancellation, no finish ever observed. Mirrors row 3
    // (table_row3_timeoutNoFinish) exactly in shape - net::Silent so the fake
    // reply never answers - except that what ends the wait is a cancellation
    // flag flipping true, not the watchdog. The timeout override is set
    // generously (5000ms) so a real timeout genuinely cannot be what ends the
    // wait; cancelPoll's 250ms cadence must get there first.
    fixture::Harness harness;
    net::reset();
    net::FakeNam* nam = new net::FakeNam();
    ProbeService service(harness.context(), nam);
    service.setRequestTimeoutOverrideMs(5000);

    auto flag = std::make_shared<std::atomic_bool>(false);
    service.setCancelToken(CancelToken(flag));

    net::Plan plan;
    plan.behaviour = net::Silent;
    net::plans << plan;

    // Injected asynchronously, the same way row 5 injects its busy-wait: a
    // zero-delay singleShot queued before blockingRequest's own nested
    // loop.exec() runs, so it fires from inside that loop rather than before it.
    QTimer::singleShot(50, [flag] { flag->store(true, std::memory_order_release); });

    net::Rescue rescue(3000);
    QElapsedTimer elapsed;
    elapsed.start();
    const RequestResult result = service.blockingRequest(nam->get(QNetworkRequest(QUrl("http://x/"))), 60);
    const qint64 took = elapsed.elapsed();

    QVERIFY(!net::rescueFired);
    QCOMPARE(result.outcome, RequestOutcome::Cancelled);
    QVERIFY(result.body.isEmpty());
    // Bounded by the cancel poll cadence, not the 5000ms timeout override: the
    // flag flips at ~50ms and cancelPoll checks every 250ms, so this returns
    // long before either the override or the harness rescue could matter.
    QVERIFY2(took < 800, qPrintable(QStringLiteral("took %1ms").arg(took)));
    // And the reply WAS aborted, which is what makes row 6's Cancelled arm
    // reachable at all - same disposal mechanism as the timeout path.
    QCOMPARE(net::abortCalls, 1);
}

void TestCloudProviderWatchdog::table_row5_timerAndFinishTogether()
{
    // ROW 5: the watchdog fires FIRST (so pending_ holds TimedOut) and a natural
    // finish arrives before exec() returns. The outcome must follow the natural
    // finish, because a finish that really happened is a finish whatever the
    // timer did in the same dispatch. This is NOT a promotion: row 5 defers to
    // rows 1 and 2, so an errored finish here would still be NetworkError.
    //
    // MADE DETERMINISTIC, not raced. A zero-timer busy-waits past BOTH expiries
    // before the loop's first wakeup, so when the loop next activates timers it
    // activates both in one pass, in expiry order: watchdog at 60ms, then the
    // reply's own delivery at 90ms.
    fixture::Harness harness;
    net::reset();
    net::FakeNam* nam = new net::FakeNam();
    ProbeService service(harness.context(), nam);

    net::Plan plan;
    plan.body = "LATE BUT REAL";
    plan.delayMs = 90;
    net::plans << plan;

    QTimer::singleShot(0, [] {
        QElapsedTimer busy;
        busy.start();
        while (busy.elapsed() < 200) { /* hold the loop past both expiries */
        }
    });

    const RequestResult result = service.blockingRequest(nam->get(QNetworkRequest(QUrl("http://x/"))), 60);

    QCOMPARE(result.outcome, RequestOutcome::Finished);
    QCOMPARE(result.body, QByteArray("LATE BUT REAL"));
    // No abort was needed: the reply had already finished.
    QCOMPARE(net::abortCalls, 0);
}

void TestCloudProviderWatchdog::table_row6_abortInducedFinishLeavesOutcomeUnchanged()
{
    // ROW 6, TIMEOUT ARM. Disposal aborts the reply, the abort synthesises a
    // finished(), and that synthetic finish must NOT be able to promote the
    // timeout into a success. See table_row6_cancelInducedFinishLeavesOutcomeUnchanged
    // immediately below for the sibling Cancelled arm, now built alongside this one.
    //
    // WHAT THIS ROW USED TO PROVE: nothing. It asserted the outcome was still
    // TimedOut, which was true by construction - the outcome is reconciled and
    // snapshotted BEFORE disposal, so no post-disposal event could have changed
    // it whatever the guard did. Worse, the disposer was destroyed AFTER the
    // QEventLoop that was the finished() connection's context object, so the
    // abort echo had no receiver at all. Bypassing `if (state.abortIssued_)
    // return;` left this row GREEN. Measured, not argued.
    //
    // Both halves are now closed in production (the disposer is destroyed while
    // the loop is still alive, and a post-disposal invariant checks that the
    // fixed state did not move), so this row can finally assert the mechanism
    // instead of a tautology.
    fixture::Harness harness;
    net::reset();
    net::FakeNam* nam = new net::FakeNam();
    ProbeService service(harness.context(), nam);

    net::Plan plan;
    plan.behaviour = net::Silent;
    plan.body = "THIS MUST NEVER REACH THE CALLER";
    net::plans << plan;

    QNetworkReply* reply = nam->get(QNetworkRequest(QUrl("http://x/")));

    // THE EMISSION IS OBSERVED DIRECTLY, not inferred from abortCalls. A spy on
    // the reply's own finished() signal proves the abort really did emit it -
    // which is the event the guard exists to neutralise. Without this, "the
    // outcome did not change" could just as easily mean "nothing was ever
    // delivered", which is exactly the vacuity that got this row rewritten.
    QSignalSpy finishedSpy(reply, &QNetworkReply::finished);
    QVERIFY(finishedSpy.isValid());

    net::Rescue rescue(3000);
    const RequestResult result = service.blockingRequest(reply, 60);

    QVERIFY(!net::rescueFired);

    // The abort happened...
    QCOMPARE(net::abortCalls, 1);
    // ...and it really did emit finished(), exactly once, to a live receiver.
    QCOMPARE(finishedSpy.count(), 1);

    // ...and the outcome it could have corrupted is unchanged.
    QCOMPARE(result.outcome, RequestOutcome::TimedOut);
    QVERIFY(result.body.isEmpty());

    // The post-disposal invariant inside blockingRequest is the other half of
    // this row: if the abort guard is removed, the echo moves naturalFinish_
    // after reconciliation and production stops with
    // DEC040_POST_DISPOSAL_STATE_CHANGED. Reaching this line at all means the
    // fixed state survived disposal intact.
}

void TestCloudProviderWatchdog::table_row6_cancelInducedFinishLeavesOutcomeUnchanged()
{
    // ROW 6, CANCELLED ARM (Stage 2's own row 6 sibling). Same mechanism as the
    // timeout arm immediately above, but disposal is triggered by cancellation
    // rather than the watchdog: cancelPoll observes the flag, sets
    // pending_ = Cancelled, quits the loop, disposal aborts the reply, the abort
    // synthesises a finished(), and that synthetic finish must NOT be able to
    // promote Cancelled into a success. Uses the same QSignalSpy-on-finished()
    // technique to prove the abort's echo was really delivered and really
    // neutralised, not merely that nothing happened to fire it.
    fixture::Harness harness;
    net::reset();
    net::FakeNam* nam = new net::FakeNam();
    ProbeService service(harness.context(), nam);
    service.setRequestTimeoutOverrideMs(5000);

    auto flag = std::make_shared<std::atomic_bool>(false);
    service.setCancelToken(CancelToken(flag));

    net::Plan plan;
    plan.behaviour = net::Silent;
    plan.body = "THIS MUST NEVER REACH THE CALLER";
    net::plans << plan;

    QNetworkReply* reply = nam->get(QNetworkRequest(QUrl("http://x/")));

    QSignalSpy finishedSpy(reply, &QNetworkReply::finished);
    QVERIFY(finishedSpy.isValid());

    QTimer::singleShot(50, [flag] { flag->store(true, std::memory_order_release); });

    net::Rescue rescue(3000);
    const RequestResult result = service.blockingRequest(reply, 60);

    QVERIFY(!net::rescueFired);

    // The abort happened...
    QCOMPARE(net::abortCalls, 1);
    // ...and it really did emit finished(), exactly once, to a live receiver.
    QCOMPARE(finishedSpy.count(), 1);

    // ...and the outcome it could have corrupted is unchanged: still Cancelled,
    // not promoted to Finished or NetworkError by the abort's synthetic finish.
    QCOMPARE(result.outcome, RequestOutcome::Cancelled);
    QVERIFY(result.body.isEmpty());

    // Same post-disposal invariant as the timeout arm: reaching this line at
    // all means the fixed state survived disposal intact.
}

// ---------------------------------------------------------------------------
// TEST-149 - Stage 2 cancellation genericity + lifetime safety. The
// observation interval must be <= 500 ms with the timeout override at 5000 ms,
// so a real timeout cannot be what ends the wait - only cancelPoll's
// kCancelPollMs cadence can. Killing mutation: raise kCancelPollMs from 250 to
// 2000, which pushes the observed interval past 500 ms.
// ---------------------------------------------------------------------------
void TestCloudProviderWatchdog::cancel_observationIntervalWithinBound()
{
    fixture::Harness harness;
    net::reset();
    net::FakeNam* nam = new net::FakeNam();
    ProbeService service(harness.context(), nam);
    service.setRequestTimeoutOverrideMs(5000);

    auto flag = std::make_shared<std::atomic_bool>(false);
    service.setCancelToken(CancelToken(flag));

    net::Plan plan;
    plan.behaviour = net::Silent;
    net::plans << plan;

    // Elapsed measured from roughly when the cancel flag flips, not from the
    // call's start, so the assertion is purely about the poll's own cadence and
    // dispatch margin - not diluted by the singleShot's own delay.
    QElapsedTimer elapsed;
    QTimer::singleShot(50, [flag, &elapsed] {
        elapsed.start();
        flag->store(true, std::memory_order_release);
    });

    net::Rescue rescue(3000);
    const RequestResult result = service.blockingRequest(nam->get(QNetworkRequest(QUrl("http://x/"))), 60);
    const qint64 observed = elapsed.elapsed();

    QVERIFY(!net::rescueFired);
    QCOMPARE(result.outcome, RequestOutcome::Cancelled);
    QVERIFY2(observed <= 500, qPrintable(QStringLiteral("observed %1ms").arg(observed)));
}

// ---------------------------------------------------------------------------
// TEST-152 - Strava's site 17: exactly one completion, buffer freed once, no
// readComplete after a failure, and a partial ride neither staged nor reported
// successful.
//
// The "streams cancelled" row is ALSO TEST-147's site-17 counterpart (Stage 2
// piece 3): site 17 has no internal pagination loop (it is a single
// blockingRequest call, not a while/do-while/for), so what T-147 asserts here
// is that a Cancelled outcome propagates as a failure exactly like a timeout
// already does above it - not an internal continue-vs-break choice, because
// there is no loop to break out of.
// ---------------------------------------------------------------------------
void TestCloudProviderWatchdog::strava_completionIsExactlyOne_data()
{
    QTest::addColumn<int>("streamsBehaviour"); // net::Behaviour for the SECOND request
    QTest::addColumn<bool>("expectComplete");
    QTest::addColumn<bool>("useCancelToken"); // T-147: cancel instead of letting it time out

    QTest::newRow("streams answer") << int(net::Ok) << true << false;
    QTest::newRow("streams time out") << int(net::Silent) << false << false;
    QTest::newRow("streams error") << int(net::Failed) << false << false;
    // Same net::Silent shape as "streams time out", but ended by a cancellation
    // flag rather than the timeout override - mirrors
    // table_row4_cancellationNoFinishObserved's mechanism at the site level.
    QTest::newRow("streams cancelled") << int(net::Silent) << false << true;
}

void TestCloudProviderWatchdog::strava_completionIsExactlyOne()
{
    QFETCH(int, streamsBehaviour);
    QFETCH(bool, expectComplete);
    QFETCH(bool, useCancelToken);

    gcStubClearSettings();
    net::reset();

    net::Plan activity;
    activity.body = body::stravaActivity;
    net::Plan streams;
    streams.behaviour = static_cast<net::Behaviour>(streamsBehaviour);
    if (streams.behaviour == net::Ok)
        streams.body = body::stravaStreams;
    if (streams.behaviour == net::Failed)
        streams.error = QNetworkReply::ContentNotFoundError;
    net::plans << activity << streams;

    fixture::Harness harness;
    net::FakeNam* nam = new net::FakeNam();
    Strava* strava = new Strava(harness.context(), nam);
    strava->setSetting(GC_STRAVA_TOKEN, "TOKEN");
    // The cancelled row needs a generous override so a real timeout cannot be
    // what ends the wait - only the cancellation flag can. The other rows keep
    // the tight 60ms bound they always had.
    strava->setRequestTimeoutOverrideMs(useCancelToken ? 5000 : 60);

    std::shared_ptr<std::atomic_bool> cancelFlag;
    if (useCancelToken) {
        cancelFlag = std::make_shared<std::atomic_bool>(false);
        strava->setCancelToken(CancelToken(cancelFlag));
        QTimer::singleShot(50, [cancelFlag] { cancelFlag->store(true, std::memory_order_release); });
    }

    fixture::StravaCompletionWatcher watcher(strava);
    net::Rescue rescue(3000);

    // The caller preallocates the buffer and is its sole owner and sole deleter.
    QByteArray* buffer = new QByteArray();
    QVERIFY(strava->readFile(buffer, "2020_01_01_10_00_00.json", "7"));

    QElapsedTimer waiting;
    waiting.start();
    while (watcher.completes + watcher.failures == 0 && waiting.elapsed() < 3000)
        QCoreApplication::processEvents(QEventLoop::AllEvents, 20);

    // (a) EXACTLY ONE of readComplete / readFailed, on every path.
    QCOMPARE(watcher.completes + watcher.failures, 1);
    QCOMPARE(watcher.completes, expectComplete ? 1 : 0);
    QCOMPARE(watcher.failures, expectComplete ? 0 : 1);

    // (b) THE BUFFER CAME BACK. Same pointer, so the caller can free it exactly
    // once - a different pointer would be a leak plus a free of something we
    // never owned.
    QCOMPARE(watcher.lastPointer, buffer);

    // (c) NO LATER readComplete AFTER A FAILURE. Keep turning the loop and make
    // sure nothing else arrives.
    const int seen = watcher.completes + watcher.failures;
    for (int i = 0; i < 5; i++)
        QCoreApplication::processEvents(QEventLoop::AllEvents, 20);
    QCOMPARE(watcher.completes + watcher.failures, seen);

    if (!expectComplete)
        QVERIFY2(!watcher.lastReason.isEmpty(), "a failure with no reason is not a report");

    delete strava;
    delete buffer; // the sole free, and it is the caller's
    net::drainDeferredDeletes();

    // The reply blockingRequest owned - the STREAMS one - is gone, exactly once.
    // The readFile reply at index 0 is the async one this slice does not own; see
    // repliesNotOwnedByBlockingRequest.
    if (net::repliesCreated > 1) {
        QVERIFY2(net::issued.at(1).isNull(), "the addSamples reply outlived its call");
        QCOMPARE(net::repliesDestroyed, net::repliesCreated - 1);
    }
}

void TestCloudProviderWatchdog::strava_partialRideIsNeitherStagedNorSuccessful()
{
    // (d). The activity arrives; the STREAMS request - the shape-C wait inside
    // prepareResponse - times out. The ride at that point is a header with no
    // samples. It must not be staged into the caller's buffer, and it must not
    // be announced as a completed download.
    gcStubClearSettings();
    net::reset();

    net::Plan activity;
    activity.body = body::stravaActivity;
    net::Plan streams;
    streams.behaviour = net::Silent;
    net::plans << activity << streams;

    fixture::Harness harness;
    net::FakeNam* nam = new net::FakeNam();
    Strava* strava = new Strava(harness.context(), nam);
    strava->setSetting(GC_STRAVA_TOKEN, "TOKEN");
    strava->setRequestTimeoutOverrideMs(60);

    fixture::StravaCompletionWatcher watcher(strava);
    net::Rescue rescue(3000);

    QByteArray* buffer = new QByteArray();
    strava->readFile(buffer, "2020_01_01_10_00_00.json", "7");

    QElapsedTimer waiting;
    waiting.start();
    while (watcher.completes + watcher.failures == 0 && waiting.elapsed() < 3000)
        QCoreApplication::processEvents(QEventLoop::AllEvents, 20);

    QVERIFY(!net::rescueFired);
    QCOMPARE(watcher.failures, 1);
    QCOMPARE(watcher.completes, 0);

    // NOT STAGED. The stub JsonFileReader::toByteArray emits "STAGED-RIDE ..."
    // precisely so that this assertion can fail - see the note on it in
    // stubs/ProviderSeamStubs.cpp.
    QVERIFY2(!watcher.stagedBytes.contains("STAGED-RIDE"),
             qPrintable(
                 QStringLiteral("a ride with no samples was staged: %1").arg(QString::fromUtf8(watcher.stagedBytes))));
    QVERIFY2(watcher.stagedBytes.isEmpty(), "notifyReadFailed's contract is that nothing is staged in the buffer");

    delete strava;
    delete buffer;
    net::drainDeferredDeletes();
}

// APPARATUS CONTROL for the assertion above: the staging marker DOES appear when
// the samples request answers, so "no marker" is a verdict and not a property of
// the stub. This runs inside strava_completionIsExactlyOne's "streams answer"
// row via stagedBytes; asserted explicitly here.

// ---------------------------------------------------------------------------
// TEST-151 - CloudServiceAutoDownload::run()'s discarded readdir errors.
// ---------------------------------------------------------------------------
// A REAL registered service that produces, on demand, each listing shape row 23
// is about: an EMPTY listing with or without a reason (rows A and B), and - since
// A3-F1 - a PARTIAL listing, one real entry with or without a reason (rows C and
// D). The name predates the partial rows and is kept so the evidence trail stays
// greppable. It is the smallest thing that can put run() on the branches under
// test: what row 23 is about is the CONSUMER's inability to tell those states
// apart, so the producer only has to be able to produce them all.
class EmptyListingService : public CloudService
{
    Q_OBJECT

  public:
    static bool reportErrors; // does readdir fill `errors`?
    static int readdirCalls;
    // A3-F1. `returnEntry` makes readdir answer with a PARTIAL result: one real
    // entry AND (when reportErrors) a reason - the shape a paginated provider
    // produces when page 1 arrives and page 2 times out. readFileCalls is the
    // control that proves the valid entry was not discarded on the way.
    static bool returnEntry;
    static int readFileCalls;
    // A3f-R040-A. open() is the OTHER half of the same error channel. Eight of
    // the thirteen migrated providers issue a BOUNDED blockingRequest inside
    // open() and report its failure through this very list, so the consumer's
    // handling of it is as much a part of the contract as readdir's.
    // `failOpen` and `openErrors` are INDEPENDENT because the combinations are
    // genuinely different states - in particular a failing open with NO reason
    // must stay silent, which is the control for the row that has one.
    static bool failOpen;
    static bool openErrors;
    static int openCalls;

    explicit EmptyListingService(Context* context) : CloudService(context) {}

    CloudService* clone(Context* context) override { return new EmptyListingService(context); }
    QString id() const override { return QStringLiteral("EmptyListing"); }
    QString uiName() const override { return QStringLiteral("Empty Listing"); }
    QImage logo() const override { return QImage(); }
    int capabilities() const override { return Download | Query; }

    bool open(QStringList& errors) override
    {
        openCalls++;
        if (openErrors)
            errors << QStringLiteral("Timed out authenticating after 5000 ms");
        return !failOpen;
    }
    bool close() override { return true; }
    QString home() override { return QString(); }

    QList<CloudServiceEntry*> readdir(QString, QStringList& errors, QDateTime, QDateTime) override
    {
        readdirCalls++;
        if (reportErrors)
            errors << QStringLiteral("Timed out after 60 ms");

        QList<CloudServiceEntry*> out;
        if (returnEntry) {
            // A SYNTACTICALLY VALID ride filename, dated inside run()'s own
            // 30-day window, so it survives RideFile::parseRideFileName and the
            // date filter and reaches the "we want it!" arm. The harness's
            // RideCache is empty, so nothing matches it locally.
            CloudServiceEntry* e = newCloudServiceEntry();
            e->name =
                QDateTime::currentDateTime().addDays(-1).toString(QStringLiteral("yyyy_MM_dd_HH_mm_ss")) + ".json";
            e->id = QStringLiteral("partial-entry-1");
            e->isDir = false;
            e->size = 1;
            out << e;
        }
        return out;
    }

    // DETERMINISTIC, so neither row waits on run()'s 30-second fallback timer.
    // run() preallocates the buffer and blocks in a QEventLoop until
    // readComplete or readFailed quits it. Answering with readFailed - QUEUED,
    // so the loop is already running when it lands - both releases that loop
    // immediately and hands the buffer to CloudServiceAutoDownload::readFailed,
    // which is the only thing that frees it (DEC-garmin-023). Nothing is staged,
    // which is exactly right: the point of these rows is that the attempt
    // HAPPENED, not that the download succeeded.
    // DEC-garmin-033 (REQ-027 (e)) — mechanical signature widening: this
    // fixture always arms a completion (readFailed, queued, below), so the
    // out-param is accepted and left untouched.
    bool readFile(QByteArray* data, QString remotename, QString remoteid,
                  CloudService::ReadFileArmed* = nullptr) override
    {
        readFileCalls++;
        Q_UNUSED(remoteid);
        QMetaObject::invokeMethod(
            this,
            [this, data, remotename]() {
                emit readFailed(data, remotename, QStringLiteral("probe: deterministic refusal"));
            },
            Qt::QueuedConnection);
        return true;
    }
};

bool EmptyListingService::reportErrors = false;
int EmptyListingService::readdirCalls = 0;
bool EmptyListingService::returnEntry = false;
int EmptyListingService::readFileCalls = 0;
bool EmptyListingService::failOpen = false;
bool EmptyListingService::openErrors = false;
int EmptyListingService::openCalls = 0;

namespace {

// A3f-R040-E. THE CONTROLS ARE MUTABLE STATICS, SO WHATEVER THIS SLOT LEAVES IN
// THEM IS WHAT THE NEXT SLOT INHERITS.
//
// The service is registered with the REAL factory, which means it stays
// registered for the rest of the process; every later slot that builds a
// Harness and runs auto-download meets it again. If this slot exits with
// `failOpen` or `returnEntry` still true, the next slot is silently running
// against a different service than its comments claim.
//
// A DESTRUCTOR, NOT TRAILING STATEMENTS. QTest's macros expand to a bare
// `return;` out of the slot rather than throwing, so on exactly the runs that
// matter - the failing ones - a restore written at the bottom of the slot is
// the one thing that does NOT execute. That turns one honest row failure into
// an unbounded number of bogus ones downstream, which is how a real defect gets
// mistaken for flakiness. Unwinding a local object is the only reset QTest
// cannot skip.
//
// SCOPE: the controls only. Unregistering the factory template is deliberately
// NOT attempted here - out of scope for this repair.
struct EmptyListingServiceReset
{
    ~EmptyListingServiceReset()
    {
        EmptyListingService::reportErrors = false;
        EmptyListingService::returnEntry = false;
        EmptyListingService::failOpen = false;
        EmptyListingService::openErrors = false;
        EmptyListingService::readdirCalls = 0;
        EmptyListingService::readFileCalls = 0;
        EmptyListingService::openCalls = 0;
    }
};

} // namespace

void TestCloudProviderWatchdog::autoDownload_distinguishesEmptyWithErrors()
{
    // ROW 23 IS NOT A PROVIDER WAIT. It is the CONSUMER branch: run() asked a
    // service for a listing, got nothing back, and had two states where there
    // are three - it could tell "entries" from "no entries", but not "no entries
    // because there is nothing new" from "no entries because the listing
    // FAILED". The `errors` list readdir has been filling all along was
    // constructed, passed by reference, populated, and then dropped unread.
    //
    // That was survivable while an unbounded listing simply never returned.
    // Bounding the wait turns the hang into a prompt, silent, EMPTY listing, so
    // without this branch the failure mode W2 removes would come straight back
    // as "auto download finished, nothing to do".
    //
    // DRIVEN THROUGH THE REAL run(). A service is registered with the real
    // factory and its sync-on-startup flag set, so run() clones it, opens it,
    // calls readdir and takes the empty branch for real. Every other registered
    // service is skipped because its flag is "false", which is what the shipped
    // code does for a service the athlete has not enabled.
    CloudServiceFactory::instance().addService(new EmptyListingService(nullptr));

    // ARMED BEFORE THE FIRST ROW TOUCHES A CONTROL, so every exit from this
    // slot - including the `return` a failing QTest macro performs - leaves the
    // statics as it found them. See EmptyListingServiceReset.
    const EmptyListingServiceReset controlsReset;

    fixture::Harness harness;
    Context* context = harness.context();
    const CloudService* registered = CloudServiceFactory::instance().service(QStringLiteral("EmptyListing"));
    QVERIFY(registered != nullptr);
    appsettings->setCValue(context->athlete->cyclist, registered->syncOnStartupSettingName(), QStringLiteral("true"));

    CloudServiceAutoDownload autoDownload(context);

    // ROW A - empty, WITH a reason. The reason must survive to the observable.
    EmptyListingService::reportErrors = true;
    EmptyListingService::readdirCalls = 0;
    autoDownload.run();
    QCOMPARE(EmptyListingService::readdirCalls, 1); // the branch was really reached
    const QStringList withErrors = autoDownload.autoDownloadErrors();
    QCOMPARE(withErrors.count(), 1);
    QVERIFY2(withErrors.first().contains(QStringLiteral("Timed out")),
             qPrintable(QStringLiteral("reason lost: %1").arg(withErrors.join("; "))));
    QVERIFY2(withErrors.first().contains(QStringLiteral("Empty Listing")),
             "the report does not say WHICH service failed");

    // ROW B - empty, WITHOUT a reason. THE CONTROL: if this were not empty the
    // row above would be satisfied by a run() that reports unconditionally, and
    // the two states would still be indistinguishable.
    EmptyListingService::reportErrors = false;
    EmptyListingService::readdirCalls = 0;
    autoDownload.run();
    QCOMPARE(EmptyListingService::readdirCalls, 1);
    QVERIFY2(autoDownload.autoDownloadErrors().isEmpty(),
             qPrintable(QStringLiteral("an empty listing with no errors reported: %1")
                            .arg(autoDownload.autoDownloadErrors().join("; "))));

    // ROW C - PARTIAL, WITH a reason. THE CASE A3-F1 FOUND SILENTLY DROPPED.
    //
    // readdir answers with one real entry AND a reason, which is what a
    // paginated provider does when an early page lands and a later one times
    // out. Rows A and B only ever asked the question for an EMPTY listing, so
    // the whole found.count()>0 half of the state space was uncovered - and the
    // reason was discarded there. Both halves of the contract are asserted:
    // the reason is reported, AND the entry that did arrive is still acted on.
    EmptyListingService::reportErrors = true;
    EmptyListingService::returnEntry = true;
    EmptyListingService::readdirCalls = 0;
    EmptyListingService::readFileCalls = 0;
    autoDownload.run();
    QCOMPARE(EmptyListingService::readdirCalls, 1); // run() completed the listing
    const QStringList partialWithErrors = autoDownload.autoDownloadErrors();
    const int partialWithErrorsReads = EmptyListingService::readFileCalls;

    // ROW D - PARTIAL, WITHOUT a reason. THE CONTROL for row C's error
    // assertion: without it, a run() that reported unconditionally would
    // satisfy row C and the three states would still be indistinguishable.
    EmptyListingService::reportErrors = false;
    EmptyListingService::readdirCalls = 0;
    EmptyListingService::readFileCalls = 0;
    autoDownload.run();
    QCOMPARE(EmptyListingService::readdirCalls, 1); // run() completed the listing
    const QStringList partialQuiet = autoDownload.autoDownloadErrors();
    const int partialQuietReads = EmptyListingService::readFileCalls;

    EmptyListingService::returnEntry = false;

    // ROW E - open FAILED, WITH a reason. A3f-R040-A.
    //
    // Rows A-D all reach readdir, so all four only ever exercised the reporting
    // that happens AFTER a successful open. The open failure path is a
    // different exit: run() abandons the service and moves to the next one, and
    // anything reported after that abandonment is reported only for the
    // services that did NOT take it. This is the eight-provider case - a
    // bounded authenticating request that gave up - and post-W2 it is the
    // likeliest way a provider goes quiet, because it fails FIRST, before a
    // listing is ever attempted.
    EmptyListingService::reportErrors = false;
    EmptyListingService::returnEntry = false;
    EmptyListingService::failOpen = true;
    EmptyListingService::openErrors = true;
    EmptyListingService::openCalls = 0;
    EmptyListingService::readdirCalls = 0;
    EmptyListingService::readFileCalls = 0;
    autoDownload.run();
    const QStringList openFailedWithErrors = autoDownload.autoDownloadErrors();
    const int openFailedOpens = EmptyListingService::openCalls;
    const int openFailedReaddirs = EmptyListingService::readdirCalls;
    const int openFailedReads = EmptyListingService::readFileCalls;

    // ROW F - open FAILED, WITHOUT a reason. THE CONTROL for row E: without it,
    // row E would be satisfied by a run() that reports unconditionally on every
    // failed open, and "the provider explained itself" would again be
    // indistinguishable from "it just said no".
    EmptyListingService::failOpen = true;
    EmptyListingService::openErrors = false;
    EmptyListingService::openCalls = 0;
    EmptyListingService::readdirCalls = 0;
    EmptyListingService::readFileCalls = 0;
    autoDownload.run();
    const QStringList openFailedQuiet = autoDownload.autoDownloadErrors();
    const int openFailedQuietOpens = EmptyListingService::openCalls;
    const int openFailedQuietReaddirs = EmptyListingService::readdirCalls;
    const int openFailedQuietReads = EmptyListingService::readFileCalls;

    EmptyListingService::failOpen = false;
    EmptyListingService::openErrors = false;

    // ASSERTION ORDER IS DELIBERATE: STRUCTURAL CONTROLS FIRST, CONTENT SECOND.
    //
    // QTest returns from the slot at the FIRST failure, so whatever is asserted
    // first is the only thing a failing run tells you about. Putting the
    // controls ahead of the error-content assertion means a regression in the
    // propagation reports itself as "the controls still hold, the REASON is
    // missing" rather than masking whether the entries were also lost. Verified
    // by mutation: reverting the propagation to its pre-repair shape leaves
    // every assertion below this comment passing and fails only the last group.

    // CONTROL 1 - BOTH ROWS downloaded the valid entry rather than discarding
    // it. This is the half of the fix a propagation-only repair would leave
    // unproven: an implementation that reported the reason and then threw the
    // partial page away would satisfy every error assertion and fail these.
    QCOMPARE(partialWithErrorsReads, 1);
    QCOMPARE(partialQuietReads, 1);

    // CONTROL 2 - a partial listing with NO reason stays silent. Without this,
    // row C would be satisfied by a run() that reported unconditionally.
    QVERIFY2(partialQuiet.isEmpty(),
             qPrintable(QStringLiteral("a partial listing with no errors reported: %1").arg(partialQuiet.join("; "))));

    // THE F1 CRITERION - the reason survives a PARTIAL listing, exactly once,
    // and names the service that failed.
    QVERIFY2(partialWithErrors.count() == 1,
             qPrintable(QStringLiteral("a partial listing with a reason reported %1 error(s), "
                                       "expected exactly 1: %2")
                            .arg(partialWithErrors.count())
                            .arg(partialWithErrors.join("; "))));
    QVERIFY2(partialWithErrors.first().contains(QStringLiteral("Timed out")),
             qPrintable(QStringLiteral("reason lost: %1").arg(partialWithErrors.join("; "))));
    QVERIFY2(partialWithErrors.first().contains(QStringLiteral("Empty Listing")),
             "the report does not say WHICH service failed");

    // CONTROL 3 - BOTH open-failure rows really took the open-failure exit:
    // open was attempted exactly once, and run() then abandoned the service
    // without listing it or downloading anything. Asserted BEFORE the content
    // below so a regression reads as "it took the right path, the REASON is
    // missing" rather than leaving it ambiguous which half moved.
    QCOMPARE(openFailedOpens, 1);
    QCOMPARE(openFailedReaddirs, 0);
    QCOMPARE(openFailedReads, 0);
    QCOMPARE(openFailedQuietOpens, 1);
    QCOMPARE(openFailedQuietReaddirs, 0);
    QCOMPARE(openFailedQuietReads, 0);

    // CONTROL 4 - a failed open with NO reason stays silent.
    QVERIFY2(openFailedQuiet.isEmpty(),
             qPrintable(QStringLiteral("a failed open with no errors reported: %1").arg(openFailedQuiet.join("; "))));

    // THE A3f-R040-A CRITERION - the reason a BOUNDED open() gave up survives
    // the failure exit, exactly once, and names the service that failed.
    QVERIFY2(openFailedWithErrors.count() == 1,
             qPrintable(QStringLiteral("a failed open with a reason reported %1 error(s), "
                                       "expected exactly 1: %2")
                            .arg(openFailedWithErrors.count())
                            .arg(openFailedWithErrors.join("; "))));
    QVERIFY2(openFailedWithErrors.first().contains(QStringLiteral("Timed out")),
             qPrintable(QStringLiteral("reason lost: %1").arg(openFailedWithErrors.join("; "))));
    QVERIFY2(openFailedWithErrors.first().contains(QStringLiteral("Empty Listing")),
             "the report does not say WHICH service failed");
}

// ---------------------------------------------------------------------------
// TEST-159 (DEC-043 Piece 1) - readComplete's GUI-thread completion-slot guard.
// ---------------------------------------------------------------------------
// A REAL registered service whose readdir() answers with ONE syntactically
// valid entry and whose readFile() emits readComplete SYNCHRONOUSLY. run() is
// invoked DIRECTLY (not via start()), exactly as TEST-151 already does, so
// every connection this test touches is same-thread/direct: readComplete
// executes nested inside this readFile() call, with `downloadlist` (private
// to CloudServiceAutoDownload) still holding the matching entry - the "not
// found, ignore it" early return in readComplete cannot mask the guard here.
class TearDownDuringReadFileService : public CloudService
{
    Q_OBJECT

  public:
    static int readFileCalls;
    // Set by the test, called from inside readFile() BEFORE emitting
    // readComplete - stands in for "the athlete tab closed while this
    // download was in flight", DEC-043's exact race.
    static std::function<void()> onReadFile;

    explicit TearDownDuringReadFileService(Context* context) : CloudService(context)
    {
        // uncompressRide's FIRST line rejects an uncompressed name unless told
        // not to expect compression - without this, readComplete returns
        // before ever reaching the context->athlete dereference this test
        // targets (a fixture bug, not a guard).
        downloadCompression = none;
    }
    CloudService* clone(Context* context) override { return new TearDownDuringReadFileService(context); }
    QString id() const override { return QStringLiteral("TearDownDuringReadFile"); }
    QString uiName() const override { return QStringLiteral("Teardown During ReadFile"); }
    QImage logo() const override { return QImage(); }
    int capabilities() const override { return Download | Query; }

    bool open(QStringList&) override { return true; }
    bool close() override { return true; }
    QString home() override { return QString(); }

    QList<CloudServiceEntry*> readdir(QString, QStringList&, QDateTime, QDateTime) override
    {
        CloudServiceEntry* e = newCloudServiceEntry();
        e->name = QDateTime::currentDateTime().addDays(-1).toString(QStringLiteral("yyyy_MM_dd_HH_mm_ss")) + ".json";
        e->id = QStringLiteral("teardown-entry-1");
        e->isDir = false;
        e->size = 1;
        return {e};
    }

    bool readFile(QByteArray* data, QString remotename, QString remoteid, ReadFileArmed* = nullptr) override
    {
        readFileCalls++;
        if (onReadFile)
            onReadFile();
        // SYNCHRONOUS: run() was called directly (same thread as the test),
        // so this is a direct connection - readComplete executes right here,
        // nested inside this call, with `downloadlist` still populated.
        emit readComplete(data, remotename, remoteid);
        return true;
    }
};

int TearDownDuringReadFileService::readFileCalls = 0;
std::function<void()> TearDownDuringReadFileService::onReadFile = nullptr;

void TestCloudProviderWatchdog::readComplete_afterOwnerTornDownMustNotDereferenceFreedMemory()
{
    // A3-R028e-F1 / DEC-043: readComplete dereferences `context->athlete->home`
    // with no lifetime guard at all. "Owner torn down" is simulated by freeing
    // the HEAP-ALLOCATED `Athlete` itself (not `context`): the very first
    // touch on the hazard path, `context->athlete->home` inside
    // uncompressRide, is then a PLAIN POINTER FIELD READ compiled directly
    // into CloudService.cpp - no detour through Qt library internals
    // (QDir::absolutePath() et al, which this test's prebuilt, non-ASan Qt
    // cannot instrument) - so the read is reliably caught. `context` itself
    // stays alive, so run()'s OWN remaining lines (notifyAutoDownloadProgress/
    // End, both `context`-only, never `athlete`) do not also fault - freeing
    // the whole Context too would race run()'s own separate, explicitly
    // out-of-scope worker-thread reads ([[ORCH-057]]), contaminating this
    // test's verdict with a different, unfixed defect.
    CloudServiceFactory::instance().addService(new TearDownDuringReadFileService(nullptr));
    TearDownDuringReadFileService::readFileCalls = 0;
    TearDownDuringReadFileService::onReadFile = nullptr;

    QTemporaryDir home;
    QWidget window;
    Context context(reinterpret_cast<MainWindow*>(&window));
    Athlete* athlete = new Athlete(&context, QDir(home.path()));
    athlete->cyclist = QStringLiteral("TearDownRider");
    context.athlete = athlete;
    RideCache rideCache(&context);
    athlete->rideCache = &rideCache;

    const CloudService* registered = CloudServiceFactory::instance().service(QStringLiteral("TearDownDuringReadFile"));
    QVERIFY(registered != nullptr);
    appsettings->setCValue(athlete->cyclist, registered->syncOnStartupSettingName(), QStringLiteral("true"));

    CloudServiceAutoDownload autoDownload(&context);

    // DEC-043 Piece 1's guard mechanism IS requestStop()'s flag - the same one
    // Athlete::close() flips (Piece 2, TEST-160). Flip it, THEN free
    // `athlete`, both from inside readFile() so the sequence lands exactly
    // where a real athlete-tab close races an in-flight async download's
    // completion. `downloadlist`'s matching entry (private to
    // CloudServiceAutoDownload, populated by run() before this call) is what
    // makes readComplete's own "not found, ignore it" early return unable to
    // mask the guard here.
    TearDownDuringReadFileService::onReadFile = [&]() {
        autoDownload.requestStop();
        delete athlete; // context->athlete now dangles - the UAF this test proves is guarded
    };

    // RED (guard not yet added to readComplete): reading `context->athlete`
    // (still valid) then `->home` off the just-freed Athlete is a real
    // heap-use-after-free, not a fixture artefact. This target halts on the
    // first ASan report (halt_on_error=1), so a crash here ends the binary
    // before the QCOMPARE below ever runs; that abrupt end IS the RED
    // failure.
    autoDownload.run();

    // GREEN: reaching here at all is the criterion - readComplete bailed
    // before touching the dangling `athlete`, so run() itself never touched
    // it again either (nothing else in run()'s remaining lines does).
    QCOMPARE(TearDownDuringReadFileService::readFileCalls, 1);

    context.athlete = nullptr; // avoid a double free at scope exit
    TearDownDuringReadFileService::onReadFile = nullptr;
}

// ---------------------------------------------------------------------------
// TEST-160 (DEC-043 Piece 2) - Athlete::close() cooperatively cancels and
// joins cloudAutoDownload before deleting it (also closes the pre-existing
// leak: the constructor's `new` had no matching delete anywhere).
// ---------------------------------------------------------------------------
// A REAL registered service whose open() polls the SAME CancelToken
// blockingRequest polls (DEC-040 Stage 2), standing in for an in-flight
// network open() a real provider would be doing when Athlete::close() runs.
class BoundedWaitOpenService : public CloudService
{
    Q_OBJECT

  public:
    static QAtomicInt openCalls;
    static QAtomicInt cancelledObserved;
    // Backstop only - should never be hit once DEC-043's setCancelToken wiring
    // is in place. Keeps a broken fix from hanging the test forever.
    static const int kHardCapMs = 10000;

    explicit BoundedWaitOpenService(Context* context) : CloudService(context) {}
    CloudService* clone(Context* context) override { return new BoundedWaitOpenService(context); }
    QString id() const override { return QStringLiteral("BoundedWaitOpen"); }
    QString uiName() const override { return QStringLiteral("Bounded Wait Open"); }
    QImage logo() const override { return QImage(); }
    int capabilities() const override { return Download | Query; }

    bool open(QStringList&) override
    {
        openCalls.fetchAndAddOrdered(1);
        QElapsedTimer t;
        t.start();
        while (!cancelToken_.cancelled() && t.elapsed() < kHardCapMs)
            QTest::qSleep(CloudService::kCancelPollMs / 5); // QThread::msleep is protected; this isn't a QThread
        cancelledObserved.storeRelease(cancelToken_.cancelled() ? 1 : 0);
        return false; // never actually opens - only the bounded wait is under test
    }
    bool close() override { return true; }
    QString home() override { return QString(); }
    QList<CloudServiceEntry*> readdir(QString, QStringList&, QDateTime, QDateTime) override { return {}; }
};

QAtomicInt BoundedWaitOpenService::openCalls(0);
QAtomicInt BoundedWaitOpenService::cancelledObserved(0);

void TestCloudProviderWatchdog::athleteClose_cancelsAndJoinsAutoDownloadThenDeletesIt()
{
    CloudServiceFactory::instance().addService(new BoundedWaitOpenService(nullptr));
    BoundedWaitOpenService::openCalls.storeRelease(0);
    BoundedWaitOpenService::cancelledObserved.storeRelease(0);

    QTemporaryDir home;
    QWidget window;
    Context context(reinterpret_cast<MainWindow*>(&window));
    Athlete athlete(&context, QDir(home.path()));
    athlete.cyclist = QStringLiteral("BoundedWaitRider");
    context.athlete = &athlete;
    RideCache rideCache(&context);
    athlete.rideCache = &rideCache;

    const CloudService* registered = CloudServiceFactory::instance().service(QStringLiteral("BoundedWaitOpen"));
    QVERIFY(registered != nullptr);
    appsettings->setCValue(athlete.cyclist, registered->syncOnStartupSettingName(), QStringLiteral("true"));

    // The stub Athlete::Athlete() (ProviderSeamStubs.cpp) leaves this null;
    // production's real Athlete::Athlete() (Athlete.cpp:168) always
    // constructs one. Building it here for real is what makes this "drive
    // Athlete::close()" rather than "drive an empty no-op".
    athlete.cloudAutoDownload = new CloudServiceAutoDownload(&context);

    // Safety net: whatever this run proves or fails to prove, never leave a
    // real OS thread running past this test - force it down unconditionally.
    struct ForceStop
    {
        Athlete* a;
        ~ForceStop()
        {
            if (a->cloudAutoDownload) {
                a->cloudAutoDownload->requestStop();
                a->cloudAutoDownload->wait();
                delete a->cloudAutoDownload;
                a->cloudAutoDownload = nullptr;
            }
        }
    } forceStop{&athlete};

    athlete.cloudAutoDownload->start();

    // Bounded wait for run() to actually reach the blocked open() - not a
    // sleep, so this does not itself pad the bound assertion below.
    QElapsedTimer arming;
    arming.start();
    while (BoundedWaitOpenService::openCalls.loadAcquire() < 1 && arming.elapsed() < 5000)
        QTest::qSleep(2);
    QVERIFY2(BoundedWaitOpenService::openCalls.loadAcquire() >= 1,
             "run() never reached the blocked open() - nothing for close() to interrupt");
    QVERIFY2(athlete.cloudAutoDownload->isRunning(), "the download thread was not actually running before close()");

    QElapsedTimer closeTimer;
    closeTimer.start();
    athlete.close(); // DEC-043 - the ACTUAL teardown hook (MainWindow.cpp:2171)
    const qint64 elapsedMs = closeTimer.elapsed();

    qInfo("TEST-160 athlete.close() elapsed=%lldms openCalls=%d cancelledObserved=%d",
          static_cast<long long>(elapsedMs), int(BoundedWaitOpenService::openCalls.loadAcquire()),
          int(BoundedWaitOpenService::cancelledObserved.loadAcquire()));

    // (a) BOUNDED - a few multiples of kCancelPollMs PLUS run()'s own fixed,
    // pre-existing `sleep(3)` after the worklist loop (CloudService.cpp,
    // unconditional, unrelated to DEC-043 and out of scope to change here) -
    // not the old provider timeout (kOpenTimeoutMs=30000, or an outright hang
    // with no CancelToken at all) and nowhere near BoundedWaitOpenService's
    // own 10s backstop. Measured ~3.0s dominated entirely by that sleep(3);
    // 6s leaves generous headroom without hiding a regression back toward the
    // old unbounded wait.
    QVERIFY2(elapsedMs < 6000,
             qPrintable(
                 QStringLiteral("Athlete::close() took %1ms, not bounded by kCancelPollMs=%2 (+ run()'s own sleep(3))")
                     .arg(elapsedMs)
                     .arg(CloudService::kCancelPollMs)));
    // (b) the thread has actually stopped, and the leak fix deleted it.
    QVERIFY2(athlete.cloudAutoDownload == nullptr, "cloudAutoDownload was not deleted/nulled by close()");
    // (c) it stopped BECAUSE of cancellation, not because BoundedWaitOpenService's
    // own backstop happened to expire first (which would make (a) meaningless).
    QVERIFY2(BoundedWaitOpenService::cancelledObserved.loadAcquire() == 1,
             "open() gave up on its own backstop, not on the CancelToken - the bound is not proven");

    athlete.rideCache = nullptr;
}

// ---------------------------------------------------------------------------
// TEST-153 (b)-(e), over all THIRTEEN migrated providers.
// ---------------------------------------------------------------------------
namespace {

enum ProviderId {
    P_Dropbox = 0,
    P_Azum,
    P_SportTracks,
    P_Xert,
    P_Nolio,
    P_PolarFlow,
    P_Strava,
    P_TrainingsTageBuch,
    P_CyclingAnalytics,
    P_SixCycle,
    P_RideWithGPS,
    P_Selfloops,
    P_SportsPlusHealth,
    P_COUNT
};

const char* providerName(int p)
{
    static const char* names[] = {"Dropbox",
                                  "Azum",
                                  "SportTracks",
                                  "Xert",
                                  "Nolio",
                                  "PolarFlow",
                                  "Strava",
                                  "TrainingsTageBuch",
                                  "CyclingAnalytics",
                                  "SixCycle",
                                  "RideWithGPS",
                                  "Selfloops",
                                  "SportsPlusHealth"};
    return names[p];
}

CloudService* makeProvider(int p, Context* context, QNetworkAccessManager* nam)
{
    switch (p) {
    case P_Dropbox:
        return new Dropbox(context, nam);
    case P_Azum:
        return new Azum(context, nam);
    case P_SportTracks:
        return new SportTracks(context, nam);
    case P_Xert:
        return new Xert(context, nam);
    case P_Nolio:
        return new Nolio(context, nam);
    case P_PolarFlow:
        return new PolarFlow(context, nam);
    case P_Strava:
        return new Strava(context, nam);
    case P_TrainingsTageBuch:
        return new TrainingsTageBuch(context, nam);
    case P_CyclingAnalytics:
        return new CyclingAnalytics(context, nam);
    case P_SixCycle:
        return new SixCycle(context, nam);
    case P_RideWithGPS:
        return new RideWithGPS(context, nam);
    case P_Selfloops:
        return new Selfloops(context, nam);
    case P_SportsPlusHealth:
        return new SportsPlusHealth(context, nam);
    }
    return nullptr;
}

// Reach nam() through a PRODUCTION entry point, whichever one this provider has.
// The ten watchdog providers have a blocking site; the three manager-only
// siblings do not, and their manager is reached from writeFile.
void touchNetwork(int p, CloudService* service)
{
    QStringList errors;
    const QDateTime from = QDateTime::currentDateTime().addYears(-2);
    const QDateTime to = QDateTime::currentDateTime();
    QByteArray payload("<ride/>");

    switch (p) {
    case P_Dropbox:
        service->setSetting(GC_DROPBOX_TOKEN, "TOKEN");
        static_cast<Dropbox*>(service)->createFolder("/gc");
        break;
    case P_Azum:
        service->setSetting(GC_AZUM_ACCESS_TOKEN, "TOKEN");
        service->setSetting(GC_AZUM_ATHLETE_ID, "7");
        service->open(errors);
        break;
    case P_SportTracks:
        service->setSetting(GC_SPORTTRACKS_REFRESH_TOKEN, "REFRESH");
        service->open(errors);
        break;
    case P_Xert:
        service->setSetting(GC_XERT_REFRESH_TOKEN, "REFRESH");
        service->open(errors);
        break;
    case P_Nolio:
        appsettings->setValue(GC_NOLIO_REFRESH_TOKEN, "REFRESH");
        appsettings->setValue(GC_NOLIO_LAST_REFRESH, QDateTime::currentDateTime().addDays(-30).toString());
        service->open(errors);
        break;
    case P_PolarFlow:
        service->setSetting(GC_POLARFLOW_TOKEN, "TOKEN");
        service->open(errors);
        break;
    case P_Strava:
        service->setSetting(GC_STRAVA_REFRESH_TOKEN, "REFRESH");
        service->open(errors);
        break;
    case P_TrainingsTageBuch:
        service->setSetting(GC_TTBUSER, "user");
        service->setSetting(GC_TTBPASS, "pass");
        service->open(errors);
        break;
    case P_CyclingAnalytics:
        service->setSetting(GC_CYCLINGANALYTICS_TOKEN, "TOKEN");
        static_cast<CyclingAnalytics*>(service)->readdir("", errors, from, to);
        break;
    case P_SixCycle:
        service->setSetting(GC_SIXCYCLE_USER, "user@example.com");
        service->setSetting(GC_SIXCYCLE_PASS, "pass");
        service->open(errors);
        break;
    case P_RideWithGPS: {
        RideFile ride;
        static_cast<RideWithGPS*>(service)->writeFile(payload, "a.json", &ride, 1);
        break;
    }
    case P_Selfloops: {
        RideFile ride;
        static_cast<Selfloops*>(service)->writeFile(payload, "a.tcx.gz", &ride, 1);
        break;
    }
    case P_SportsPlusHealth: {
        RideFile ride;
        static_cast<SportsPlusHealth*>(service)->writeFile(payload, "a.tcx.gz", &ride, 1);
        break;
    }
    }
}

void addAllProviders()
{
    QTest::addColumn<int>("provider");
    for (int p = 0; p < P_COUNT; p++)
        QTest::newRow(providerName(p)) << p;
}

} // namespace

void TestCloudProviderWatchdog::manager_notBuiltUntilFirstUse_data()
{
    addAllProviders();
}

void TestCloudProviderWatchdog::manager_notBuiltUntilFirstUse()
{
    // (b), first half, and the per-provider half of (a): CONSTRUCTION ALONE
    // BUILDS NOTHING. This is what the seventeen pre-main factory registrations
    // do, and it kills both "construct eagerly in the base ctor" and "the
    // provider still builds its own in its own constructor".
    QFETCH(int, provider);
    gcStubClearSettings();

    fixture::Harness harness;
    CloudService* service = makeProvider(provider, harness.context(), nullptr);
    QVERIFY(service);

    QCOMPARE(service->namCreated(), false);
    QCOMPARE(service->findChildren<QNetworkAccessManager*>().count(), 0);

    delete service;
}

void TestCloudProviderWatchdog::manager_injectedUsedExactlyAsSupplied_data()
{
    addAllProviders();
}

void TestCloudProviderWatchdog::manager_injectedUsedExactlyAsSupplied()
{
    // (c). The injected manager is used EXACTLY as supplied and NO default is
    // built alongside it - checked AFTER the provider has actually gone to the
    // network, because "alongside" happens in nam(), not in the constructor.
    QFETCH(int, provider);
    gcStubClearSettings();
    net::reset();
    net::Plan silent;
    silent.behaviour = net::Silent;
    net::plans << silent;

    fixture::Harness harness;
    net::FakeNam* nam = new net::FakeNam();
    CloudService* service = makeProvider(provider, harness.context(), nam);
    service->setRequestTimeoutOverrideMs(60);

    net::Rescue rescue(3000);
    touchNetwork(provider, service);

    QCOMPARE(service->namCreated(), true);
    const QList<QNetworkAccessManager*> managers = service->findChildren<QNetworkAccessManager*>();
    QCOMPARE(managers.count(), 1);
    QCOMPARE(managers.first(), static_cast<QNetworkAccessManager*>(nam));

    // The request really did go through the injected manager, so the row is
    // about use and not merely about parentage.
    QVERIFY2(net::repliesCreated >= 1, "nothing was requested through the injected manager");

    delete service;
    net::drainDeferredDeletes();
}

void TestCloudProviderWatchdog::manager_destroyedExactlyOnce_data()
{
    addAllProviders();
}

void TestCloudProviderWatchdog::manager_destroyedExactlyOnce()
{
    // (e). Destroying the service destroys its manager, once. A provider that
    // still carried its own `if (context) delete nam;` would double-free here
    // and ASan would say so; a base that dropped its delete would leave the
    // pointer alive.
    QFETCH(int, provider);
    gcStubClearSettings();
    net::reset();
    net::Plan silent;
    silent.behaviour = net::Silent;
    net::plans << silent;

    fixture::Harness harness;
    net::FakeNam* nam = new net::FakeNam();
    QPointer<net::FakeNam> weak(nam);
    const int destroyedBefore = net::FakeNam::instancesDestroyed;

    CloudService* service = makeProvider(provider, harness.context(), nam);
    service->setRequestTimeoutOverrideMs(60);
    net::Rescue rescue(3000);
    touchNetwork(provider, service);

    QVERIFY(!weak.isNull());
    delete service;
    net::drainDeferredDeletes();

    QVERIFY2(weak.isNull(), "the manager outlived the service that owned it");
    QCOMPARE(net::FakeNam::instancesDestroyed, destroyedBefore + 1);
}

void TestCloudProviderWatchdog::manager_createdOnceOnFirstUse()
{
    // (b), second half: FIRST USE CREATES EXACTLY ONE, and every later use
    // returns that same one. The creation logic lives in exactly one place - the
    // base - so this is asserted once, against the real base implementation
    // through a real CloudService subclass, rather than thirteen times against
    // thirteen copies of nothing.
    fixture::Harness harness;
    ProbeService service(harness.context());

    QCOMPARE(service.namCreated(), false);

    QNetworkAccessManager* first = service.nam();
    QVERIFY(first != nullptr);
    QCOMPARE(service.findChildren<QNetworkAccessManager*>().count(), 1);

    for (int i = 0; i < 5; i++)
        QCOMPARE(service.nam(), first);
    QCOMPARE(service.findChildren<QNetworkAccessManager*>().count(), 1);
}

namespace {

// A3-F3 - UNCONDITIONAL TEARDOWN FOR A SLOT THAT OWNS A RUNNING QThread.
//
// Qt 6.8.2's ~QThread calls qFATAL - not qWarning - when it destroys a thread
// that is still running, and qFatal aborts the process. So ONE failed assertion
// between worker.start() and the teardown at the bottom used to end the whole
// run: QTest's QVERIFY/QCOMPARE `return` out of the void slot, ~QThread then
// runs on a still-running thread, and the process dies where it stands.
//
// MEASURED, NOT ASSUMED - AND THE NUMBERS ARE THE RUN'S, NOT A DEDUCTION FROM
// DECLARATION ORDER. The pre-guard control (this slot forced to fail, full
// suite, freshly built patch-only binary) reported
//     Totals: 150 passed, 1 failed, 0 skipped
// - 150 passing entries plus the intended failure - and then died:
//     QFATAL : QThread: Destroyed while thread is still running
//     Received signal 6 (SIGABRT)
// FOUR entries never executed at all (manager_wiredExactlyOnce,
// table_row7_spuriousExecReturn_isolated, nestedEventLoopStillWorksAfterRow7,
// cleanupTestCase - established by diffing the two runs' entry lists, not by
// reading a position off a sorted PASS list, which says nothing about where a
// slot is declared). The same forced failure WITH this guard in place completes
// the run: "154 passed, 1 failed" - every entry executes, only the intended one
// fails, no QFATAL, no SIGABRT.
//
// So teardown is RAII and runs on EVERY path out, including every early return.
//
// NO QTest MACRO APPEARS IN THE DESTRUCTOR. QVERIFY/QCOMPARE expand to a
// `return`, which inside ~WorkerGuard would skip the quit()/wait() this class
// exists to guarantee - trading one silent teardown bug for another.
//
// HOW A FAILURE STAYS VISIBLE WITHOUT ONE. finish() performs every cleanup
// attempt and records the COMPLETE result of each - deletion queued, destruction
// observed, worker stopped.
//   * NORMAL PATH: the slot calls finish() by name and asserts that record, so a
//     teardown failure is an ordinary QTest failure at the ordinary place.
//   * EARLY-RETURN PATH: a macro above has already returned and nobody is left
//     to assert, so the destructor reports for itself - SILENT when the cleanup
//     succeeded, and otherwise a named qCritical per failed item, emitted AFTER
//     every cleanup attempt has already run, so reporting can neither abort the
//     process nor truncate the teardown.
class WorkerGuard
{
  public:
    struct Result
    {
        bool deleteQueued = false;
        bool destructionObserved = false;
        bool workerStopped = false;
        bool hadService = false;
    };

    explicit WorkerGuard(QThread& worker) : worker_(worker) {}

    // ON THE EARLY-RETURN PATH THE DESTRUCTOR IS THE ONLY THING LEFT, SO IT HAS
    // TO SAY WHAT IT COULD NOT DO. A QTest macro above has already `return`ed
    // out of the slot; nobody is left to assert on the Result. A cleanup that
    // then FAILED would be completely invisible - the run would look like one
    // ordinary assertion failure while a worker thread, or a ProbeService and
    // its manager, was still alive and touching the net:: globals every later
    // row uses.
    //
    // STILL NO QTest MACRO HERE: a macro expands to `return` and would skip the
    // checks below it. Under THIS TARGET'S REGISTERED ENVIRONMENT qCritical
    // only hands the message to the message handler - it does not abort (that
    // is qFatal), does not unwind, and does not skip the remaining cleanup,
    // which finish() has in any case already completed in full before the first
    // check runs. Every condition is tested independently so one failure cannot
    // hide another.
    //
    // THAT IS A PROPERTY OF THE ENVIRONMENT, NOT OF qCritical. Qt can make
    // criticals FATAL through QT_FATAL_CRITICALS, and the variable is a
    // COUNTDOWN, not a flag: measured on Qt 6.8.2, `=1` aborts on the first
    // critical (SIGABRT, exit 134, straight through QTest::qRun - QTest's
    // message handler does not intercept it), `=2` survives the first and
    // aborts on the second, while `=0` and unset are both non-fatal. So a
    // destructor that reports through qCritical would, under an inherited
    // `QT_FATAL_CRITICALS=1`, abort the process DURING cleanup and destroy the
    // very evidence it exists to print.
    //
    // Both CTest registrations of this target therefore pin
    // QT_FATAL_CRITICALS=0 explicitly rather than relying on it being unset.
    // THAT PIN GOVERNS THE CTest-REGISTERED RUNS ONLY. Invoking this binary
    // directly inherits whatever the caller's environment holds, so a direct
    // run is only covered if that command pins QT_FATAL_CRITICALS=0 itself.
    //
    // IT IS SILENT ON EVERY GOOD PATH. The normal path calls finish() by name
    // and asserts the Result, so `finished_` is already true here and nothing
    // is emitted; a SUCCESSFUL early-return cleanup emits nothing either. Any
    // "WorkerGuard:" line in a log therefore means a real teardown failure.
    ~WorkerGuard()
    {
        const bool earlyReturn = !finished_;
        const Result& r = finish();
        if (!earlyReturn)
            return;

        if (r.hadService && !r.deleteQueued)
            qCritical("WorkerGuard: early-return cleanup could NOT queue deleteLater() "
                      "for the adopted service onto its affinity thread");

        if (r.hadService && r.deleteQueued && !r.destructionObserved)
            qCritical("WorkerGuard: early-return cleanup did NOT observe the adopted "
                      "service's destruction within %d ms",
                      kDestroyTimeoutMs);

        if (!r.workerStopped)
            qCritical("WorkerGuard: early-return cleanup did NOT stop the worker thread "
                      "within %d ms",
                      kWorkerStopTimeoutMs);
    }

    WorkerGuard(const WorkerGuard&) = delete;
    WorkerGuard& operator=(const WorkerGuard&) = delete;

    // Called once the service exists. BEFORE this the guard is still effective:
    // it simply has no object to destroy and only stops the thread.
    void adopt(QObject* service)
    {
        service_ = service;
        // DirectConnection, with the service as its own context. destroyed() is
        // emitted at the START of ~QObject, while the QObject subobject is still
        // valid memory, and this lambda never dereferences the service - it only
        // releases the semaphore, on whichever thread emits, i.e. the worker.
        QObject::connect(
            service_, &QObject::destroyed, service_, [this]() { destroyed_.release(); }, Qt::DirectConnection);
    }

    // Idempotent. Safe to call explicitly and then again from the destructor.
    const Result& finish()
    {
        if (finished_)
            return result_;
        finished_ = true;

        if (service_ != nullptr) {
            result_.hadService = true;
            QObject* s = service_;
            // `service_` is cleared BEFORE the wait, so no path below this line
            // can dereference an object that is about to be destroyed.
            service_ = nullptr;
            result_.deleteQueued = QMetaObject::invokeMethod(s, [s]() { s->deleteLater(); }, Qt::QueuedConnection);
            if (result_.deleteQueued)
                result_.destructionObserved = destroyed_.tryAcquire(1, kDestroyTimeoutMs);
        }

        // ALWAYS, on every path, including the one where the service was never
        // created or the deleteLater could not be queued.
        worker_.quit();
        result_.workerStopped = worker_.wait(kWorkerStopTimeoutMs);
        return result_;
    }

  private:
    static const int kDestroyTimeoutMs = 5000;
    static const int kWorkerStopTimeoutMs = 5000;

    QThread& worker_;
    QObject* service_ = nullptr;
    // A3-F4. WHAT THIS SEMAPHORE ACTUALLY GUARANTEES: it is a member of the
    // guard, so it stays alive until destruction has been OBSERVED and the
    // worker has been STOPPED - both of which happen inside finish(), while the
    // guard is still alive. It does NOT "outlive the worker", and nothing needs
    // it to: the connection that releases it dies with the service, which
    // finish() destroys before it returns.
    QSemaphore destroyed_;
    Result result_;
    bool finished_ = false;
};

} // namespace

void TestCloudProviderWatchdog::manager_createdInObjectAffinityThread()
{
    // (d). The contract says the manager is created in the OBJECT's affinity
    // thread. The previous form of this row proved the opposite of what it
    // claimed: it left the service on an UNSTARTED QThread and called nam() from
    // the main thread, so the only way to pass was for production to accept an
    // off-affinity call and move the manager across afterwards. That is the
    // accommodation the contract now forbids, and the row would have locked it
    // in - it could only ever have failed if the accommodation were REMOVED.
    //
    // So the call is made where it is legal: on a REAL, RUNNING thread that the
    // service actually lives on, through a blocking-queued invocation.
    //
    // AND EVERY OBSERVATION IS MADE THERE TOO. Once the service has been moved
    // to `worker` it is worker-owned, and reading its QObject state from this
    // thread - findChildren(), namCreated(), wiredWith - would be exactly the
    // cross-thread access this slice exists to eliminate, committed by the test
    // that certifies it. So the lambda captures PLAIN SCALARS AND POINTERS while
    // it runs on the worker, and this thread afterwards asserts only on those
    // captured values. Nothing below dereferences the service or the manager.
    fixture::Harness harness;

    QThread worker;
    worker.start();

    // A3-F3. THE GUARD IS INSTALLED BEFORE THE FIRST ASSERTION THAT CAN RETURN
    // FROM THIS SLOT - including the isRunning() check on the next line. It is
    // declared after `worker`, so it is destroyed BEFORE it: the thread is
    // always stopped before ~QThread can see it running.
    WorkerGuard guard(worker);

    QVERIFY(worker.isRunning());

    ProbeService* service = new ProbeService(nullptr);
    service->moveToThread(&worker);
    // From here the guard owns the teardown of `service` too.
    guard.adopt(service);

    // NOTHING IS READ OFF `service` FROM HERE ON. moveToThread() above is the
    // handover itself and has to be issued from this side, but it is the last
    // thing this thread does to the object: even service->thread() is QObject
    // state, and reading it here would be the same cross-thread access this row
    // exists to forbid. The affinity assertion it would have made is made
    // instead by `observedService`, sampled inside the worker below, where it is
    // actually meaningful.
    QThread* observedCurrent = nullptr;
    QThread* observedService = nullptr;
    QThread* observedManager = nullptr;
    QNetworkAccessManager* observedManagerPtr = nullptr;
    QNetworkAccessManager* observedWiredWith = nullptr;
    int observedManagerChildren = -1;
    int observedWireCalls = -1;
    bool observedNamCreatedBefore = true;
    bool observedNamCreatedAfter = false;

    // BlockingQueuedConnection: the lambda runs ON `worker`, and this thread
    // waits. Everything sampled inside is therefore sampled in the affinity
    // thread, which is the only place the question is meaningful.
    const bool observeDelivered = QMetaObject::invokeMethod(
        service,
        [&]() {
            observedCurrent = QThread::currentThread();
            observedService = service->thread();
            observedNamCreatedBefore = service->namCreated();

            observedManagerPtr = service->nam();

            observedManager = observedManagerPtr->thread();
            observedManagerChildren = service->findChildren<QNetworkAccessManager*>().count();
            observedNamCreatedAfter = service->namCreated();
            observedWireCalls = service->wireCalls;
            observedWiredWith = service->wiredWith;
        },
        Qt::BlockingQueuedConnection);

    // IF THE INVOCATION WAS NEVER DELIVERED, EVERY ASSERTION BELOW WOULD BE
    // ABOUT ITS OWN INITIALISERS. The return value is the only thing that
    // distinguishes "the worker ran this" from "nothing happened".
    QVERIFY2(observeDelivered, "the observation invokeMethod was not delivered to the worker");

    QVERIFY(observedManagerPtr != nullptr);

    // CURRENT == SERVICE == MANAGER, all sampled inside the invocation.
    QCOMPARE(observedCurrent, &worker);
    QCOMPARE(observedService, &worker);
    QCOMPARE(observedManager, &worker);

    // Lazy before, materialised after - both read in the affinity thread.
    QCOMPARE(observedNamCreatedBefore, false);
    QCOMPARE(observedNamCreatedAfter, true);

    // Parented in one operation, so exactly one manager exists and it is ours.
    QCOMPARE(observedManagerChildren, 1);

    // wireNam() ran THERE too, exactly once, against that same manager - not on
    // some other thread that merely happened to reach the object first.
    QCOMPARE(observedWireCalls, 1);
    QCOMPARE(observedWiredWith, observedManagerPtr);

    // DESTROYED IN ITS OWN AFFINITY THREAD - VIA deleteLater(), NOT delete.
    //
    // Deleting a QObject from a thread other than its own is precisely the
    // class of defect this slice exists to remove, so the teardown does not get
    // to cheat by moving the service back here first. But the obvious way to
    // honour that - a queued lambda that runs `delete service` - is itself
    // unsafe: that lambda is delivered AS a QMetaCallEvent to `service`, so it
    // destroys the object while the object is handling an event. Qt documents
    // that as a crash. deleteLater() is the supported form: it posts a
    // DeferredDelete that the worker's event loop runs once the current event
    // has been fully dispatched.
    //
    // Leak detection is OFF in this target, so a service that was never
    // destroyed would be invisible to ASan - and quit()/wait() returning is
    // NOT evidence that anything was deleted, only that the loop stopped. The
    // evidence is QObject::destroyed itself.
    //
    // A3-F3. The teardown itself now lives in WorkerGuard, which the destructor
    // would run anyway. It is invoked HERE, by name, so its outcome can be
    // asserted on the normal path - the destructor cannot assert anything
    // (a QTest macro there would `return` and skip the quit()/wait()).
    //
    // The semaphore's real guarantee is stated on the member itself (A3-F4): it
    // stays alive until destruction is observed AND the worker is stopped, both
    // of which happen inside finish(). It is released from a DirectConnection
    // slot, which runs on whichever thread emits - the worker, inside ~QObject.
    const WorkerGuard::Result& teardown = guard.finish();

    QVERIFY2(teardown.deleteQueued, "the deleteLater request was not queued onto the worker");

    // DESTRUCTION IS PROVEN, not assumed, and it is bounded so a failure is a
    // failure rather than a hung suite.
    QVERIFY2(teardown.destructionObserved, "QObject::destroyed was never emitted - the service was not destroyed "
                                           "in its affinity thread within the timeout");

    // `service` is a dangling pointer from here on and is never read or called
    // again; the guard cleared its own copy before waiting.
    QVERIFY2(teardown.workerStopped, "the worker thread did not stop within the timeout");
}

void TestCloudProviderWatchdog::manager_wiredExactlyOnce()
{
    // The subtlest half of the lazy ruling: provider-specific sslErrors wiring
    // must be installed EXACTLY ONCE when the manager becomes available -
    // neither dropped (SSL errors silently unhandled) nor duplicated (two
    // certificate prompts).
    fixture::Harness harness;

    // Default path.
    {
        ProbeService service(harness.context());
        QCOMPARE(service.wireCalls, 0);
        QNetworkAccessManager* manager = service.nam();
        QCOMPARE(service.wireCalls, 1);
        QCOMPARE(service.wiredWith, manager);
        for (int i = 0; i < 5; i++)
            service.nam();
        QCOMPARE(service.wireCalls, 1);
    }

    // Injected path - the manager exists from construction, but the wiring still
    // cannot happen there (a virtual call from a base constructor does not reach
    // the override), so it happens on first use, once.
    {
        net::FakeNam* nam = new net::FakeNam();
        ProbeService service(harness.context(), nam);
        QCOMPARE(service.wireCalls, 0);
        QCOMPARE(service.nam(), static_cast<QNetworkAccessManager*>(nam));
        QCOMPARE(service.wireCalls, 1);
        QCOMPARE(service.wiredWith, static_cast<QNetworkAccessManager*>(nam));
        for (int i = 0; i < 5; i++)
            service.nam();
        QCOMPARE(service.wireCalls, 1);
    }
}

void TestCloudProviderWatchdog::table_row7_spuriousExecReturn_isolated()
{
    // ROW 7 - "exec() returned without either slot having run". The only way to
    // produce that from outside the helper is QCoreApplication::exit(), which
    // sets this thread's quitNow flag PERMANENTLY: QEventLoop::exec() then
    // returns -1 at once, for ever, and only QCoreApplication::exec() clears it -
    // which this binary never enters.
    //
    // Run in-process, that made the slot a poison pill. It had to be declared
    // last, every later nested event loop was silently broken, and the suite
    // acquired an ordering constraint that no test framework enforces. It runs
    // in a CHILD now; this parent process never sets the flag.
    const childproc::Result r = childproc::run(childmode::Row7SpuriousExec);

    QVERIFY2(r.started, "row 7 child did not start");
    QVERIFY2(r.finishedInTime, "row 7 child did not finish inside its timeout");
    QVERIFY(r.entered(childmode::Row7SpuriousExec));
    QCOMPARE(r.exitCode, 0);
    QVERIFY2(!r.sawAsan(), qPrintable(r.err));
    QVERIFY2(r.out.contains(QStringLiteral("DEC040_ROW7_END")),
             qPrintable(QStringLiteral("row 7 child did not run to completion:\n%1").arg(r.out)));

    // Each claim is a separate parsed field, so a partial failure is legible
    // rather than collapsing into one boolean.
    QVERIFY2(r.out.contains(QStringLiteral("outcomeIsNetworkError=1")),
             qPrintable(QStringLiteral("row 7 did not report NetworkError:\n%1").arg(r.out)));
    QVERIFY2(r.out.contains(QStringLiteral("ok=0")),
             qPrintable(QStringLiteral("row 7 reported success:\n%1").arg(r.out)));
    QVERIFY2(r.out.contains(QStringLiteral("bodyEmpty=1")),
             qPrintable(QStringLiteral("row 7 returned a body:\n%1").arg(r.out)));
    QVERIFY2(r.out.contains(QStringLiteral("hasNoOutcomeText=1")),
             qPrintable(QStringLiteral("row 7 lost its error text:\n%1").arg(r.out)));
}

void TestCloudProviderWatchdog::nestedEventLoopStillWorksAfterRow7()
{
    // THE HEALTH CONTROL, AND ITS POSITION IS THE POINT. It is declared AFTER
    // row 7 and QTest runs slots in declaration order, so under the old
    // in-process arrangement this slot could not have passed: exec() would have
    // returned -1 immediately and the timer would never have fired.
    //
    // An ordinary nested event loop, of the kind every blockingRequest row
    // depends on, run right behind row 7.
    QEventLoop loop;
    bool fired = false;
    QTimer::singleShot(0, &loop, [&]() {
        fired = true;
        loop.quit();
    });

    const int rc = loop.exec();

    // One assertion, not two: `rc` and `fired` are the same fact seen twice. A
    // poisoned thread makes exec() return -1 AND leaves the timer undispatched,
    // so QCOMPARE(rc, 0) restated what this already proves.
    QVERIFY2(fired && rc == 0, "the nested event loop never dispatched - row 7 poisoned this process");

    // And the production helper itself still works here, which is the property
    // that actually matters for every later row.
    fixture::Harness harness;
    net::reset();
    net::FakeNam* nam = new net::FakeNam();
    ProbeService service(harness.context(), nam);

    net::Plan plan;
    plan.behaviour = net::Ok;
    plan.body = "{}";
    net::plans << plan;

    const RequestResult result = service.blockingRequest(nam->get(QNetworkRequest(QUrl("http://x/"))), 5000);
    QCOMPARE(result.outcome, RequestOutcome::Finished);
}

// Machine-readable, one fact per line, so the parent asserts on parsed values
// rather than on a human-readable blob. Printed BEFORE QApplication exists.
static int runPreMainCensus()
{
    childmode::announce(childmode::PreMainCensus);

    const CloudServiceFactory& factory = CloudServiceFactory::instance();
    const QStringList registered = factory.serviceNames();

    printf("DEC040_CENSUS_TOTAL=%d\n", int(registered.count()));

    // The id is bracketed because four of them contain a space; brackets keep
    // the line unambiguously parseable without inventing an escaping scheme.
    for (int i = 0; i < kExpectedRegisteredMigratedCount; i++) {
        const QString wanted = QString::fromLatin1(kExpectedRegisteredMigrated[i].id);
        const CloudService* service = factory.service(wanted);
        // present / absent, and - for the ones that are there - whether the
        // template is holding a manager. namCreated() is a const observer; it
        // builds nothing, so reading it cannot perturb what it measures.
        printf("DEC040_TEMPLATE id=[%s] present=%d namCreated=%d\n", kExpectedRegisteredMigrated[i].id,
               service != nullptr ? 1 : 0, (service != nullptr && service->namCreated()) ? 1 : 0);
    }

    for (int i = 0; i < kRegistrationExceptionCount; i++) {
        const QString name = QString::fromLatin1(kRegistrationExceptions[i].id);
        printf("DEC040_EXCEPTION id=[%s] present=%d\n", kRegistrationExceptions[i].id,
               factory.service(name) != nullptr ? 1 : 0);
    }

    printf("DEC040_CENSUS_END\n");
    fflush(stdout);
    return 0;
}

// The positive control for the capture path. It emits, on stderr and before
// QApplication, exactly the text the census asserts is ABSENT. If the parent
// cannot see it here, the parent's "no warning was emitted" reading is a
// statement about a broken pipe and not about the code under test.
static int runStderrControl()
{
    childmode::announce(childmode::StderrControl);
    fprintf(stderr, "QObject::connect(QObject, Unknown): invalid nullptr parameter\n");
    fflush(stderr);
    return 0;
}

int main(int argc, char* argv[])
{
    // === CHILD MODE IS DETECTED AT THE VERY TOP, before QApplication and before
    // === anything else in this function runs. PreMainCensus in particular MUST
    // === answer here: its whole subject is the state left by static
    // === initialisation, and constructing QApplication would contaminate it.
    const childmode::Mode mode = childmode::detect(argc, argv);
    childmode::stripFlags(argc, argv);

    switch (mode) {
    case childmode::PreMainCensus:
        return runPreMainCensus();
    case childmode::StderrControl:
        return runStderrControl();
    case childmode::None:
    default:
        break;
    }

    QApplication app(argc, argv);

    // The remaining child modes need an application object (threads, event
    // loops, deferred deletes), so they are dispatched after it exists but
    // before any test object is constructed.
    switch (mode) {
    case childmode::OffAffinityNam:
        return childmode::runOffAffinityNam();
    case childmode::InjectedNamAffinity:
        return childmode::runInjectedNamAffinity();
    case childmode::Row7SpuriousExec:
        return childmode::runRow7SpuriousExec();
    default:
        break;
    }

    TestCloudProviderWatchdog testObject;
    return QTest::qExec(&testObject, argc, argv);
}

#include "testCloudProviderWatchdog.moc"
