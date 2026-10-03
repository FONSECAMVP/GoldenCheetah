/*
 * Copyright (c) 2026 GoldenCheetah Contributor
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the Free
 * Software Foundation; either version 2 of the License, or (at your option)
 * any later version.
 */

// Link-level stand-ins for testGarminConnectImport (TEST-067 / REQ-018) ONLY.
//
// That target compiles the REAL src/Cloud/CloudService.cpp so the REAL
// CloudService::uncompressRide (the boundary REQ-018 is about) is under test,
// plus the REAL RideFile/FitRideFile/TcxRideFile/TcxParser so a REAL parser
// produces the RideFile. Those translation units reference the rest of the
// application (RideCache, RideItem, RideMetric, MainWindow, DataProcessor,
// Colors, GlobalContext, ...) from code paths this contract never executes;
// linking them for real pulls in 355 of the app's 439 objects, i.e. the whole
// program (which is exactly why DEC-garmin-021 rejected that option for
// TEST-064).
//
// So the REAL headers are used everywhere — only the DEFINITIONS below are
// stand-ins, and NONE of them sits on the path under test:
//     readFile -> readComplete -> uncompressRide -> RideFileFactory -> reader
// Everything here is either inert or an honest minimum (the athlete directory
// structure IS created for real, because uncompressRide writes the staged bytes
// into home->temp() before handing the file to the reader).

// LTMSettings.h first: Athlete.h only forward-declares LTMSettings but holds it
// by value in a QList, so the container instantiation needs the complete type
// (the app gets it transitively; this TU must ask for it).
#include "Athlete.h"
#include "Colors.h"
#include "CompareDateRange.h"
#include "CompareInterval.h"
#include "Context.h"
#include "CsvRideFile.h"
#include "DataProcessor.h"
#include "JsonRideFile.h"
#include "LTMSettings.h"
#include "MainWindow.h"
#include "PyEmbeddedAdapter.h"
#include "RideCache.h"
#include "RideFileCommand.h"
#include "RideItem.h"
#include "RideMetadata.h"
#include "RideMetric.h"
#include "Settings.h"
#include "Specification.h"
#include "SplineLookup.h"
#include "TimeUtils.h"
#include "Units.h"
#include "Utils.h"
#include "WPrime.h"
#include "Zones.h"

#include <QDir>
#include <QtGlobal>

#include <functional>

// A QObject-derived class whose Q_OBJECT is not moc'd here still needs its three
// virtuals + staticMetaObject or its vtable will not link. None of these objects
// ever emits, receives or is qobject_cast in this test, so borrowing QObject's
// metaobject is sufficient — and it fails loudly (a cast returns nullptr) rather
// than silently if that ever stops being true.
#define GC_STUB_METAOBJECT(Klass)                                          \
    const QMetaObject Klass::staticMetaObject = QObject::staticMetaObject; \
    const QMetaObject* Klass::metaObject() const                           \
    {                                                                      \
        return &staticMetaObject;                                          \
    }                                                                      \
    void* Klass::qt_metacast(const char* n)                                \
    {                                                                      \
        return QObject::qt_metacast(n);                                    \
    }                                                                      \
    int Klass::qt_metacall(QMetaObject::Call c, int id, void** a)          \
    {                                                                      \
        return QObject::qt_metacall(c, id, a);                             \
    }

// --- globals ---------------------------------------------------------------
GSettings* appsettings = nullptr;
QString gcroot;
double dpiXFactor = 1.0;
double dpiYFactor = 1.0;

// --- TEST-082 (REQ-021) — the member-touch observation channel --------------
// Three of the stand-ins below are reached with a COLLABORATOR that a suspended
// dialog frame may have had freed underneath it, and in the real program each of
// them dereferences that object:
//   * Context::metadataFlush and RideItem::notifyRideMetadataChanged are SIGNALS
//     (Context.h:341, RideItem.h:89), so calling one runs
//     QMetaObject::activate(this, ...) — a load of `this->d_ptr`;
//   * MainWindow::saveSilent (SaveDialogs.cpp:125) dereferences both of its
//     arguments (rideItem->path, context->athlete->rideCache).
// An empty `{}` body does NONE of that, so a lifetime test asserting "this call
// would have used freed memory" could pass on a corpse and prove nothing — which
// is exactly what findings A3-R019-F3 and B-R019-05 recorded.
//
// So each of those bodies now performs one real load through its object and
// publishes what it loaded here. A store to a `volatile` at namespace scope
// cannot be elided and cannot be hoisted above the load that feeds it, so the
// dereference survives -O2 (this target is built Release). TEST-082 reads these
// to prove the touches are load-bearing rather than decorative.
namespace gcstub {
volatile quintptr contextMemberTouch = 0;
volatile quintptr rideItemMemberTouch = 0;
volatile quintptr saveSilentThisTouch = 0;
volatile quintptr saveSilentArgTouch = 0;

// --- TEST-121 (A3-R028b-F1) — THE SUSPENSION saveRide REALLY HAS ------------
//
// CloudServiceSyncDialog::saveRide (CloudService.cpp:3267) calls
// DataProcessorFactory::autoProcess(ride, "Auto", "Import"), which runs every
// processor whose configKeyAutomation is set to "Auto"
// (DataProcessor.cpp:220-221). Two of the shipped processors SUSPEND from
// inside postProcess: FixElevation builds a QNetworkAccessManager, posts to
// api.open-elevation.com and waits in a local QEventLoop with NO TIMEOUT
// (FixElevation.cpp:288-300), and FixPyDataProcessor goes through FixPyRunner
// (FixPyDataProcessor.cpp:39 -> FixPyRunner.cpp:40-48). So `saveRide` is a
// suspension point in production, and completedRead calls it while holding a
// raw QTreeWidgetItem* that a Refresh delivered into that loop has FREED.
//
// The stand-in below cannot import a real processor - the whole point of this
// file is that the processors are not linked - so it exposes the SEAM instead:
// a test may arm one action, which the stand-in calls SYNCHRONOUSLY from
// inside autoProcess, exactly where postProcess would have run. The action
// supplies its own nested QEventLoop.
//
// LSN-056 - THIS FILE IS COMPILED INTO THREE TARGETS
// (testGarminConnectImport, testGarminConnectReadFailedConsumer,
// testGarminConnectSyncDialogClose). The seam is therefore INERT by
// construction: an empty std::function is never called, and no target that
// does not arm it can behave differently. The counter is write-only.
std::function<void()> autoProcessAction; // null unless a test arms it
int autoProcessCalls = 0;                // was the seam reached at all?

// TEST-158 (B-R028-17, DEC-042) — did saveRide reach past its own suspension
// to the member reads DEC-042 guards? JsonFileReader::writeRideFile and
// Athlete::addRide are both link stubs already (do nothing, touch no disk);
// these just publish whether either was called, so a test can assert the
// accepted trade-off (a parent-teardown mid-autoProcess skips the save
// entirely) rather than merely "no crash".
int writeRideFileCalls = 0;
int addRideCalls = 0;
} // namespace gcstub

namespace {
// appsettings is dereferenced by the ride readers (e.g. FitRideFile reads the
// "fix garmin smart recording" preference), so it must be a live object, not a
// null pointer. It answers every query with the caller's own default.
struct AppSettingsInstaller
{
    AppSettingsInstaller() { appsettings = new GSettings(QStringLiteral("gc-test"), QStringLiteral("gc-test")); }
} appSettingsInstaller;
} // namespace

// --- GSettings -------------------------------------------------------------
GSettings::GSettings(QString org, QString app)
{
    Q_UNUSED(org);
    Q_UNUSED(app);
}

GSettings::~GSettings() {}

QVariant GSettings::value(const QObject* me, const QString key, const QVariant def)
{
    Q_UNUSED(me);
    Q_UNUSED(key);
    return def;
}

QVariant GSettings::cvalue(QString athleteName, QString key, QVariant def)
{
    Q_UNUSED(athleteName);
    Q_UNUSED(key);
    return def;
}

void GSettings::setCValue(QString athleteName, QString key, QVariant value)
{
    Q_UNUSED(athleteName);
    Q_UNUSED(key);
    Q_UNUSED(value);
}

// --- AthleteDirectoryStructure ---------------------------------------------
// Real behaviour, not a stand-in: uncompressRide writes into temp().
AthleteDirectoryStructure::AthleteDirectoryStructure(const QDir home) : myhome(home)
{
    athlete_activities = QStringLiteral("activities");
    athlete_tmp_activities = QStringLiteral("tmpactivities");
    athlete_imports = QStringLiteral("imports");
    athlete_records = QStringLiteral("records");
    athlete_downloads = QStringLiteral("downloads");
    athlete_fileBackup = QStringLiteral("bak");
    athlete_config = QStringLiteral("config");
    athlete_cache = QStringLiteral("cache");
    athlete_calendar = QStringLiteral("calendar");
    athlete_workouts = QStringLiteral("workouts");
    athlete_logs = QStringLiteral("logs");
    athlete_temp = QStringLiteral("temp");
    athlete_quarantine = QStringLiteral("quarantine");
    athlete_planned = QStringLiteral("planned");
    athlete_snippets = QStringLiteral("snippets");
    athlete_media = QStringLiteral("media");
}

AthleteDirectoryStructure::~AthleteDirectoryStructure() {}

void AthleteDirectoryStructure::createAllSubdirs()
{
    myhome.mkpath(activities().absolutePath());
    myhome.mkpath(tmpActivities().absolutePath());
    myhome.mkpath(imports().absolutePath());
    myhome.mkpath(config().absolutePath());
    myhome.mkpath(cache().absolutePath());
    myhome.mkpath(temp().absolutePath());
    myhome.mkpath(downloads().absolutePath());
}

GC_STUB_METAOBJECT(AthleteDirectoryStructure)

// --- Athlete ---------------------------------------------------------------
// The REAL Athlete ctor stands up the whole athlete (ride cache, zones, seasons,
// intervals, ...). uncompressRide needs exactly one thing from it: home->temp().
Athlete::Athlete(Context* context, const QDir& homeDir) : QObject(nullptr)
{
    Q_UNUSED(context);
    home = new AthleteDirectoryStructure(homeDir);
    home->createAllSubdirs();
    rideCache = nullptr;
    cloudAutoDownload = nullptr;
}

Athlete::~Athlete()
{
    delete home;
}

void Athlete::addRide(QString name, bool dosignal, bool select, bool useTempActivities, bool planned)
{
    Q_UNUSED(name);
    Q_UNUSED(dosignal);
    Q_UNUSED(select);
    Q_UNUSED(useTempActivities);
    Q_UNUSED(planned);
    ++gcstub::addRideCalls; // TEST-158 (DEC-042)
}

double Athlete::getWeight(QDate date, RideFile* ride)
{
    Q_UNUSED(date);
    Q_UNUSED(ride);
    return 75.0;
}

double Athlete::getHeight(RideFile* ride)
{
    Q_UNUSED(ride);
    return 1.75;
}

GC_STUB_METAOBJECT(Athlete)

// --- Context / GlobalContext -----------------------------------------------
Context::Context(MainWindow* mainWindow) : mainWindow(mainWindow)
{
    athlete = nullptr;
    // REQ-021 / DEC-garmin-030 - both cloud dialogs now take context->tab as
    // their PARENT (CloudService.cpp:119 and :884). The REAL Context::Context
    // (Context.cpp:141-162) does NOT initialise `tab` either - AthleteTab's
    // constructor sets it (AthleteTab.cpp:35) - so a test Context that never
    // gets a tab would hand QDialog whatever the heap happened to hold. Nulled
    // here so the shape is DEFINED: no tab means a parentless dialog, which is
    // what every fixture that does not model an athlete tab expects.
    tab = nullptr;
    ride = nullptr;
}

Context::~Context() {}

// A SIGNAL in the real program (Context.h:341), so the real body is
// QMetaObject::activate(this, ...) — an unconditional load of `this`. See the
// gcstub note above: this stand-in performs one, on a member the constructor
// above actually initialises, so a freed Context faults here as it would in
// production instead of sailing through an inert `{}`.
void Context::metadataFlush()
{
    gcstub::contextMemberTouch = reinterpret_cast<quintptr>(athlete);
}

void Context::autoDownloadStart() {}
void Context::autoDownloadEnd() {}
void Context::autoDownloadProgress(QString s, double x, int i, int n)
{
    Q_UNUSED(s);
    Q_UNUSED(x);
    Q_UNUSED(i);
    Q_UNUSED(n);
}

GC_STUB_METAOBJECT(Context)

GlobalContext::GlobalContext()
{
    rideMetadata = nullptr;
    colorEngine = nullptr;
    useMetricUnits = true;
}

GlobalContext* GlobalContext::context()
{
    static GlobalContext* gc = new GlobalContext();
    return gc;
}

GC_STUB_METAOBJECT(GlobalContext)

// --- TimeUtils / Units -----------------------------------------------------
DateRange::DateRange(QDate from, QDate to, QString name, QColor color)
    : from(from), to(to), name(name), color(color), valid(true)
{
}

DateRange& DateRange::operator=(const DateRange& other)
{
    from = other.from;
    to = other.to;
    name = other.name;
    color = other.color;
    id = other.id;
    valid = other.valid;
    return *this;
}

QDateTime convertToLocalTime(QString timestamp)
{
    return QDateTime::fromString(timestamp, Qt::ISODate).toLocalTime();
}

QString kphToPace(double kph, bool metric, bool swim)
{
    Q_UNUSED(kph);
    Q_UNUSED(metric);
    Q_UNUSED(swim);
    return QString();
}

// --- Specification ---------------------------------------------------------
Specification::Specification() : it(nullptr), ri(nullptr), recintsecs(0) {}
PlanFilter::PlanFilter(PlanFilterType type) : type(type) {}
bool Context::isValid(Context* p)
{
    return p != nullptr;
}

bool Specification::pass(RideItem* item) const
{
    Q_UNUSED(item);
    return true;
}

void Specification::setDateRange(DateRange dr)
{
    this->dr = dr;
}

double Specification::secsStart() const
{
    return -1;
}

double Specification::secsEnd() const
{
    return -1;
}

// --- RideItem / RideCache / RideMetadata / RideMetric ----------------------
// `isdirty` is initialised DELIBERATELY. The real RideItem is built by the
// RideCache and is never dirty on arrival; leaving the flag as whatever the heap
// held made the upload dialog take the unsaved-changes branch at random
// (CloudService.cpp:436), which surfaced as a HANG in an unasked-for modal
// prompt rather than as a failure. Finding B-R019-04.
RideItem::RideItem(RideFile* ride, Context* context) : context(context), ride_(ride)
{
    isdirty = false;
}

// REQ-025 / TEST-092 — THE LAZY OPEN, MODELLED RATHER THAN STUBBED AWAY.
//
// The real RideItem::ride(bool open = true) (RideItem.cpp:175-181) OPENS the
// ride file through RideFileFactory when `ride_` is null, and openRideFile runs
// a nested QEventLoop on the FIT read path (FitRideFile.cpp:172-184). That makes
// every `item->ride()` a SUSPENSION POINT — which is exactly what the comment at
// CloudService.cpp:556 claimed it was not ("compressRide() runs no nested event
// loop ... so a separate self-bail between it and writeFile would be unreachable
// and untestable"). CloudServiceUploadDialog::start() calls item->ride() twice,
// at :558 and :562.
//
// A stand-in that just returns ride_ cannot express that, so this one does what
// production does when the ride is not already in memory. The metric, override
// and interval bookkeeping the real function performs AFTERWARDS is omitted:
// none of it is on the path under test and none of it can suspend.
RideFile* RideItem::ride(bool open)
{
    if (!open || ride_)
        return ride_;

    QFile file(path + "/" + fileName);
    ride_ = RideFileFactory::instance().openRideFile(context, file, errors_);
    return ride_;
}

double RideItem::getForSymbol(QString name, bool useMetricUnits)
{
    Q_UNUSED(name);
    Q_UNUSED(useMetricUnits);
    return 0;
}

// Also a SIGNAL (RideItem.h:89), reached at CloudService.cpp:444 as
// `context->ride->notifyRideMetadataChanged()` — i.e. on an object the athlete
// tab may have freed. Same treatment, same reason.
void RideItem::notifyRideMetadataChanged()
{
    gcstub::rideItemMemberTouch = reinterpret_cast<quintptr>(context);
}

RideItem::~RideItem() {}

GC_STUB_METAOBJECT(RideItem)

void RideCache::save(bool opendata, QString filename)
{
    Q_UNUSED(opendata);
    Q_UNUSED(filename);
}

void RideMetadata::setLinkedDefaults(RideFile* ride)
{
    Q_UNUSED(ride);
}

QHash<QString, RideMetricPtr> RideMetric::computeMetrics(RideItem* item, Specification spec, const QStringList& metrics)
{
    Q_UNUSED(item);
    Q_UNUSED(spec);
    Q_UNUSED(metrics);
    return QHash<QString, RideMetricPtr>();
}

// --- DataProcessorFactory --------------------------------------------------
DataProcessorFactory* DataProcessorFactory::instance_ = nullptr;

DataProcessorFactory& DataProcessorFactory::instance()
{
    if (!instance_)
        instance_ = new DataProcessorFactory();
    return *instance_;
}

bool DataProcessorFactory::autoProcess(RideFile* ride, QString mode, QString op)
{
    Q_UNUSED(ride);
    Q_UNUSED(mode);
    Q_UNUSED(op);

    // TEST-121 (A3-R028b-F1) — the armed action stands in for a processor that
    // suspends. See the gcstub block comment above. Fires ONCE and disarms
    // itself first, so a re-entrant autoProcess (saveRide makes a second call
    // with mode "Save") cannot run it twice.
    ++gcstub::autoProcessCalls;
    if (gcstub::autoProcessAction) {
        std::function<void()> action = gcstub::autoProcessAction;
        gcstub::autoProcessAction = nullptr;
        action();
    }

    return false;
}

QMap<QString, DataProcessor*> DataProcessorFactory::getProcessors(bool coreProcessorsOnly) const
{
    Q_UNUSED(coreProcessorsOnly);
    return QMap<QString, DataProcessor*>();
}

// --- Colors ----------------------------------------------------------------
QColor GCColor::getColor(int c)
{
    Q_UNUSED(c);
    return QColor(Qt::black);
}

QColor GCColor::invertColor(QColor c)
{
    Q_UNUSED(c);
    return QColor(Qt::white);
}

// --- Utils -----------------------------------------------------------------
namespace Utils {
bool qstringascend(const QString& s1, const QString& s2)
{
    return s1 < s2;
}
} // namespace Utils

// --- MainWindow ------------------------------------------------------------
// Reached at CloudService.cpp:445 as `context->mainWindow->saveSilent(context,
// item)`, with all three of those objects owned by an athlete tab that may have
// been closed while the dialog was suspended.
//
// The load through `this` is QObject-level ON PURPOSE, and is a PRAGMATIC close
// rather than a strict one: no target in this tree can afford a real MainWindow,
// so the "MainWindow" here is a reinterpret_cast of a plain QWidget (see the
// upload harness in testGarminConnectSyncDialogClose.cpp) and reading a
// MainWindow-specific member would be undefined behaviour. isWidgetType() reads
// d_ptr in the QObject subobject, which is at offset 0 of both types and does
// fault on freed storage. NOTE that the real saveSilent (SaveDialogs.cpp:125)
// touches no MainWindow member at all, so this stand-in is deliberately STRICTER
// than production on `this` — it can over-report a freed MainWindow, never
// under-report one.
//
// The two ARGUMENT loads are exact, though: the real function dereferences
// rideItem->path, rideItem->ride() and context->athlete->rideCache.
void MainWindow::saveSilent(Context* context, RideItem* item)
{
    gcstub::saveSilentThisTouch = isWidgetType() ? 1 : 2;
    gcstub::saveSilentArgTouch =
        reinterpret_cast<quintptr>(context->athlete) ^ reinterpret_cast<quintptr>(item->context);
}

// --- ride writers (upload/save paths, never executed here) -----------------
RideFile* JsonFileReader::openRideFile(QFile& file, QStringList& errors, QList<RideFile*>* list) const
{
    Q_UNUSED(file);
    Q_UNUSED(list);
    errors << QStringLiteral("json reader not linked into this test target");
    return nullptr;
}

bool JsonFileReader::writeRideFile(Context* context, const RideFile* ride, QFile& file) const
{
    Q_UNUSED(context);
    Q_UNUSED(ride);
    Q_UNUSED(file);
    ++gcstub::writeRideFileCalls; // TEST-158 (DEC-042)
    return false;
}

RideFile* CsvFileReader::openRideFile(QFile& file, QStringList& errors, QList<RideFile*>* list) const
{
    Q_UNUSED(file);
    Q_UNUSED(list);
    errors << QStringLiteral("csv reader not linked into this test target");
    return nullptr;
}

bool CsvFileReader::writeRideFile(Context* context, const RideFile* ride, QFile& file, CsvType format) const
{
    Q_UNUSED(context);
    Q_UNUSED(ride);
    Q_UNUSED(file);
    Q_UNUSED(format);
    return false;
}

// --- misc RideFile.cpp / FitRideFile.cpp collaborators ---------------------
RideFileCommand::RideFileCommand(RideFile* ride) : ride(ride), stackptr(0), inLUW(false), luw(nullptr) {}

RideFileCommand::~RideFileCommand() {}

GC_STUB_METAOBJECT(RideFileCommand)

CompareDateRange::~CompareDateRange() {}
CompareInterval::~CompareInterval() {}

int Zones::getCP(int rnum) const
{
    Q_UNUSED(rnum);
    return 0;
}

int Zones::whichRange(const QDate& date) const
{
    Q_UNUSED(date);
    return -1;
}

void FilterHrv(XDataSeries* rr, double rr_min, double rr_max, double filt, int hwin)
{
    Q_UNUSED(rr);
    Q_UNUSED(rr_min);
    Q_UNUSED(rr_max);
    Q_UNUSED(filt);
    Q_UNUSED(hwin);
}

WPrime::WPrime() {}

void WPrime::setRide(RideFile* ride)
{
    Q_UNUSED(ride);
}

void SplineLookup::update(const QwtSplineBasis& spline, const QPolygonF& polygon, double tolerance)
{
    Q_UNUSED(spline);
    Q_UNUSED(polygon);
    Q_UNUSED(tolerance);
}

double SplineLookup::valueY(double x) const
{
    Q_UNUSED(x);
    return 0;
}

// --- Garmin embedded-Python adapter ----------------------------------------
// GarminConnect's lazy production path does `new PyEmbeddedAdapter(modulePath)`.
// The test injects an IGarminDownloadClient, so this is never constructed — it
// only keeps the target Python-free (`garmin-fast`), exactly as the
// ReadFileStubPreamble fake does for the other targets.
PyEmbeddedAdapter::PyEmbeddedAdapter(const GarminPyModulePath& modulePath)
{
    Q_UNUSED(modulePath);
    qFatal("PyEmbeddedAdapter must never be constructed in testGarminConnectImport");
}

PyEmbeddedAdapter::~PyEmbeddedAdapter() {}

PyAuthOutcome PyEmbeddedAdapter::authenticate(const QString&, const QString&)
{
    return {};
}

PyAuthOutcome PyEmbeddedAdapter::submitMfa(const QString&)
{
    return {};
}

PyDownloadOutcome PyEmbeddedAdapter::downloadActivity(const QString&, const QString&)
{
    return {};
}

PyLoadTokensOutcome PyEmbeddedAdapter::loadTokens(const QString&)
{
    return {};
}

PyListOutcome PyEmbeddedAdapter::listActivitiesSince(const QString&)
{
    return {};
}

PyProfileOutcome PyEmbeddedAdapter::fetchProfile()
{
    return {};
}
