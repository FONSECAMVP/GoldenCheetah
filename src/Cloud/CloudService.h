/*
 * Copyright (c) 2015 Mark Liversedge (liversedge@gmail.com)
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the Free
 * Software Foundation; either version 2 of the License, or (at your option)
 * any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc., 51
 * Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 */

#ifndef GC_CloudService_h
#define GC_CloudService_h
#include <QList>
#include <QMap>
#include <QString>
#include <QStringList>
#include <QDateTime>
#include <QObject>
#include <QPointer>
#include <QNetworkAccessManager>
#include <QNetworkReply>

#include <QDialog>
#include <QCheckBox>
#include <QTreeWidget>
#include <QSplitter>
#include <QLineEdit>
#include <QLabel>
#include <QPushButton>
#include <QProgressBar>
#include <QPropertyAnimation>

#include "Context.h"
#include "Athlete.h"
#include "Settings.h"

// A CloudService is a base class for working with cloud services
// and for historic reasons local file stores too
// we want to sync or backup to. The initial version is to support
// working with Dropbox but could be extended to support other
// stores including Google, Microsoft "cloud" storage or even
// to sync / backup to a pen drive or similar

class RideItem;
class CloudServiceEntry;

// DEC-040 Stage 1 (W2) - THE BOUNDED-REQUEST OUTCOME CONTRACT.
//
// Every blocking provider wait in this tree used to be the same three lines:
//
//     QEventLoop loop;
//     connect(reply, SIGNAL(finished()), &loop, SLOT(quit()));
//     loop.exec();
//
// which is unbounded: a server that accepts the connection and then never
// answers parks the calling thread forever, and for the ten providers migrated
// here that thread is either the GUI thread or CloudServiceAutoDownload's
// worker. CloudService::blockingRequest replaces all of them with one wait that
// cannot outlive its timeout and that reports WHY it stopped waiting, so a
// caller can no longer mistake "we gave up" for "the server said yes".
//
// Cancelled is DECLARED here but is UNREACHABLE IN STAGE 1: nothing in this
// version constructs a cancellation, and blockingRequest never assigns it. It
// is present so that Stage 2 (generic cooperative cancellation) adds a new
// arm rather than churning this enum and every switch over it.
enum class RequestOutcome {
    Finished,       // the reply emitted finished() naturally, with no error
    TimedOut,       // the watchdog fired before any natural finish was observed
    Cancelled,      // STAGE 2 ONLY - unreachable in Stage 1, see above
    NetworkError    // a natural finish carrying an error, or no outcome at all
};

struct RequestResult {

    RequestOutcome outcome = RequestOutcome::NetworkError;
    int httpStatus = -1;
    QNetworkReply::NetworkError error = QNetworkReply::NoError;
    QString errorString;

    // Populated ONLY when outcome == Finished. Every other outcome leaves this
    // default-constructed, so a caller that forgets to test ok() and parses the
    // body anyway gets an empty document rather than a stale or partial one.
    QByteArray body;

    bool ok() const { return outcome == RequestOutcome::Finished; }
};

// Representing an Athlete when the service allows for
// a coach or manager relationship -- i.e. it lists athletes
// so you can choose which one you want to sync with
class CloudServiceAthlete
{
    public:
        CloudServiceAthlete() : local(NULL) {}

        QString id;
        QString name;
        QString desc;
        void *local;        // available for the service to use
};

class CloudService : public QObject {

    Q_OBJECT

    public:

        // REIMPLEMENT EACH FROM BELOW FOR A NEW
        // TYPE OF FILESTORE. SEE Dropbox.{h,cpp}
        // FOR A REFERENCE IMPLEMENTATION THE

        // DEC-040 Stage 1 (S-1) - THE BASE OWNS THE NETWORK ACCESS MANAGER.
        //
        // Thirteen services in this tree each declared their own `nam` member and
        // each built it as `if (context) nam = new QNetworkAccessManager(this);`
        // with `nam` absent from the initialiser list. On the NULL-context path -
        // which every one of them takes pre-main, from the `static bool add =
        // addXxx();` factory registration at the bottom of its .cpp - the member
        // was therefore left INDETERMINATE, and the matching destructor's
        // `if (context) delete nam;` skipped it. Nothing read it, so nothing
        // noticed. blockingRequest changes that: the migrated call sites read the
        // manager on paths the old code guarded by hand, so the member's
        // initialisation became load-bearing and had to stop being conditional.
        //
        // THE INVARIANT IS "DETERMINISTICALLY NULL-OR-VALID, NEVER INDETERMINATE".
        // Eager construction in this constructor was one way to reach that, and it
        // was the first thing tried; it was withdrawn because it is not free. Those
        // seventeen `static bool add = addXxx();` registrations run BEFORE main(),
        // and therefore before any QCoreApplication exists. Building a
        // QNetworkAccessManager there measurably works - correct affinity, no
        // crash - but each one emits two
        //   QObject::connect(QObject, Unknown): invalid nullptr parameter
        // warnings as it wires itself to an application object that is not there
        // yet, and leaves a partially wired manager behind. Roughly thirty-four
        // lines of startup noise, for objects that in the common case are never
        // used: the factory's registered instances exist to answer id(), uiName()
        // and clone(), and clone() makes a fresh service for the real work.
        //
        // So the manager is created LAZILY, on first real use, by nam() below.
        // Pre-main construction is inert: no manager, no warnings. The member is
        // NULL until nam() runs and valid afterwards - never indeterminate, which
        // is the property that mattered.
        //
        // `injectedNam` is the test seam and the ONLY way a different manager can
        // get in. It is adopted at construction, parented to `this`, and used
        // EXACTLY as supplied - nam() never builds a default alongside it.
        //
        // An injected manager whose affinity does not match this service's is
        // REJECTED, not repaired: adopting it would either reparent across
        // threads or silently move a manager the caller may already be using.
        // The diagnostic carries the stable sentinel DEC040_INJECTED_NAM_AFFINITY.
        // Ownership
        // is therefore the same on both paths; there is no "the caller still owns
        // it" special case to get wrong. Production callers pass nothing and keep
        // the historic CloudService(Context*) / Provider(Context*) signature
        // unchanged; this is ordinary defaulted-argument construction, present in
        // the SAME compiled code that ships, not a test-only preprocessor branch.
        CloudService(Context *context, QNetworkAccessManager *injectedNam = NULL);
        virtual ~CloudService();

        // DEC-040 Stage 1 (S-1) - "has the manager been materialised yet?"
        //
        // The lazy contract above is only a contract if something can check it, and
        // the thing that most needs checking cannot be checked any other way: that
        // pre-main factory registration builds NO manager. That construction has
        // already happened by the time any code can run, so the only trace it can
        // leave is the state of the object it built. This is a const observer over
        // that state; it starts no request and creates nothing.
        bool namCreated() const { return nam_ != NULL; }

        // DEC-040 Stage 1 (S-1) - the generic bounds, calibrated on GarminConnect's
        // shipped kListTimeoutMs/kDownloadTimeoutMs (GarminConnect.cpp).
        //
        // SixCycle does NOT use these: it shipped with its own, tighter 5s open /
        // 10s readdir and keeps them, because nothing here is evidence about
        // SixCycle's server and loosening a bound that already works is a
        // regression dressed up as consistency.
        static const int kOpenTimeoutMs = 30000;    // open / auth / one-shot calls
        static const int kListTimeoutMs = 60000;    // directory listings

        // DEC-040 Stage 1 (S-1) - test-facing timeout override.
        //
        // A watchdog test cannot afford to spend the real 30s or 60s, and a suite
        // that shortens the bound with #ifdef would no longer be testing the code
        // that ships. So the override is an ordinary member of the production
        // class, consumed at ONE point inside blockingRequest; no call site passes
        // it, mentions it, or changes shape because of it. Negative means "use the
        // bound the call site asked for", which is what every production caller
        // gets.
        void setRequestTimeoutOverrideMs(int ms) { requestTimeoutOverrideMs_ = ms; }

        // The following must be reimplemented
        virtual bool initialize() { return true; }

        // factory only has services for a NULL context, so we always
        // clone for the context its used in before doing anything - including config
        virtual CloudService *clone(Context *) = 0;

        // id of service, which MUST NOT be translated - it is the symbol
        // that represents the website, so likely to just be the URL simplified
        // e.g. https://www.strava.com => "Strava"
        virtual QString id() const { return "NONE"; }
        virtual QString uiName() const { return tr("None"); }
        virtual QString description() const { return ""; }

        // need a logo, we may resize but will keep aspect ratio
        virtual QImage logo() const = 0;

        // an icon to put on the authorize button (mandated by strava guidelines)
        virtual QString authiconpath() const { return QString(""); }

        // register with capabilities of the service - emerging standard
        // is a service that allows oauth, query and upload as well as download
        enum { OAuth=0x01, UserPass=0x02, Upload=0x04, Download=0x08, Query=0x10} capa_;
        virtual int capabilities() const { return OAuth | Upload | Download | Query; }

        // register with type of service
        enum { Activities=0x01, Measures=0x02, Calendar=0x04 } type_;
        virtual int type() const { return Activities; }

        // open/connect and close/disconnect
        virtual bool open(QStringList &errors) { Q_UNUSED(errors); return false; }
        virtual bool close() { return false; }

        // REQ-008 (DEC-garmin-019 Option C) — connect/disconnect persistence hooks.
        // Default no-ops so the ~15 other services are unaffected (non-breaking).
        // GarminConnect overrides these to persist the connect-success blob +
        // active-account pointer, and to delete them on disconnect. The wizard
        // drives persistConnectSuccess() on connect; CredentialsPage::deleteClicked()
        // drives disconnectService() generically (no per-service special-case).
        // NOTE: named disconnectService() (not disconnect()) to avoid name-hiding
        // QObject::disconnect() overloads in the ~15 CloudService subclasses.
        virtual void persistConnectSuccess(const QString &garminUserId, const QString &tokenBlob)
            { Q_UNUSED(garminUserId); Q_UNUSED(tokenBlob); }
        virtual void disconnectService() {}

        // what is the path to the home directory on this store
        virtual QString home() { return "/"; }

        // create a folder
        virtual bool createFolder(QString path) { Q_UNUSED(path); return false; }

        // set any local settings on folder selection (used by google drive)
        virtual void folderSelected(QString path) { Q_UNUSED(path); return; }

        // The operation id is local correlation metadata. It must be returned
        // unchanged and must never alter the remote name or payload.
        virtual bool writeFile(QByteArray &data, QString remotename, RideFile *ride, quint64 operationId) {
            Q_UNUSED(data); Q_UNUSED(remotename); Q_UNUSED(ride); Q_UNUSED(operationId); return false;
        }
        quint64 newWriteOperationId() { if (++nextWriteOperationId_ == 0) ++nextWriteOperationId_; return nextWriteOperationId_; }
        void notifyWriteComplete(quint64 operationId, QString name, QString message) { emit writeComplete(operationId,name,message); }

        // read a file  and notify when done
        virtual bool readFile(QByteArray *data, QString remotename, QString remoteid) {
            Q_UNUSED(data); Q_UNUSED(remotename); Q_UNUSED(remoteid); return false;
        }
        // DEC-garmin-036 (REQ-028 (c)) - THE BUFFER-IDENTITY CONTRACT, WRITTEN
        // DOWN. It was always relied on and never stated: notifyReadFailed below
        // says "pass back the SAME `data` pointer the caller handed to readFile"
        // (:160-161), and this, its success twin, said nothing at all - even
        // though the consumer frees whichever pointer arrives on either channel.
        //
        // CONTRACT, for every service:
        //   - emit readComplete OR readFailed for a given readFile() call, never
        //     both and never neither.
        //   - pass back the SAME `data` pointer the caller handed to readFile.
        //     Not a copy, not a replacement, not a re-`new`ed buffer with the same
        //     bytes: THE POINTER. The sync dialog now uses that pointer as the
        //     IDENTITY of the transfer - it is how a completion belonging to an
        //     abandoned batch is told from the live batch's own (the in-flight
        //     ticket, CloudServiceSyncDialog::inflight) - so a service that
        //     returns a different buffer has its completions silently swallowed
        //     rather than merely mislabelled.
        //   - the caller is the sole owner and the sole deleter of that buffer.
        //
        // AUDITED, not assumed, 2026-08-18: all eleven readFile implementations
        // in this tree satisfy it. Nine (Azum, CyclingAnalytics, Dropbox,
        // PolarFlow, SixCycle, SportTracks, Strava, Xert, Nolio) stash the
        // caller's pointer in `buffers.insert(reply,data)` inside readFile and
        // hand back `buffers.value(reply)` - Strava and Nolio via
        // prepareResponse(), whose only return statement is `return data;`, and
        // SportTracks/CyclingAnalytics/Xert via a `returning` local that is that
        // same lookup. GarminConnect captures the pointer by value in
        // postReadComplete. LocalFileStore emits with its own parameter.
        void notifyReadComplete(QByteArray *data, QString name, QString message) { emit readComplete(data,name,message); }

        // DEC-garmin-023 - the EXPLICIT read-FAILURE channel.
        //
        // readComplete cannot carry failure. Every service in this tree passes
        // tr("Completed.") as `message` on SUCCESS (Strava, Dropbox, SportTracks,
        // Xert, Azum, PolarFlow, CyclingAnalytics, SixCycle, Nolio,
        // LocalFileStore), so "message is non-empty" cannot discriminate, and
        // "payload is empty" is a guess about the parser. A service that KNOWS
        // the read did not happen therefore says so here instead.
        //
        // CONTRACT for a service that chooses to use it:
        //   - emit readFailed OR readComplete for a given readFile() call, never
        //     both and never neither. The consumers free the caller-preallocated
        //     buffer on whichever arrives, so a second delivery is a double free.
        //   - pass back the SAME `data` pointer the caller handed to readFile,
        //     with nothing staged into it.
        //   - `reason` is user-facing text and is rendered verbatim by the sync
        //     dialog, so it must say what went wrong and which service said so.
        //
        // Not emitting it at all is entirely legal and is what the ~15 existing
        // services do: they are unchanged, and behave exactly as before.
        void notifyReadFailed(QByteArray *data, QString name, QString reason) { emit readFailed(data,name,reason); }

        // list and select an athlete - list will need to block rather than notify asynchronously
        virtual QList<CloudServiceAthlete> listAthletes() { return QList<CloudServiceAthlete>(); }
        virtual bool selectAthlete(CloudServiceAthlete) { return false; }

        // The service must define what settings it needs in the "settings" map.
        // Each entry maps a setting type to the appsetting symbol name
        // Local - setting maintained internally by the cloud service which require no interation
        // Combo - setttings selected from a predefined list with the setting name encoded
        //         to include the values e.g:
        //         "<athlete-private>google_drive/drive_scope::Scope::drive::drive.appdata::drive.file::drive"
        //         would offer a combo called "Scope" with 3 values drive, drive.appdata and
        //         drive.file that are selected in order to set a combo to allow the user to
        //         select the setting for "<athlete-private>google_drive/drive_scope"
        //         Only 1 is supported at present, we can add more if needed later
        //
        // AthleteID can only be provided if the service implements the listAthletes and selectAthlete
        // entry points -- to list and accept the choice of athlete by the user
        enum CloudServiceSetting { Username, Password, OAuthToken, Key, URL, DefaultURL, Folder, AthleteID,
                                   Local1, Local2, Local3, Local4, Local5, Local6,
                                   Combo1, Metadata1, Consent } setting_;
        QHash<CloudServiceSetting, QString> settings;

        // When a service is instantiated by the cloud service factory, the configuration
        // is injected into the map below. The cloud service should read this to get user
        // configuration rather than directly from appsettings. This is because the settings
        // may be different for different accounts, and the settings might not yet be saved
        // in appsettings (e.g. during configuration dialogs when getting an OAuth token
        // or browsing for a folder.
        QHash<QString, QVariant> configuration;
        QVariant getSetting(QString name, QVariant def=QString("")) { return configuration.value(name, def); }
        void setSetting(QString name, QVariant value) { configuration.insert(name, value); }

        // we use a dirent style API for traversing
        // root - get me the root of the store
        // readdir - get me the contents of a path
        virtual CloudServiceEntry *root() { return NULL; }
        virtual QList<CloudServiceEntry*> readdir(QString path, QStringList &errors) {
            Q_UNUSED(path); errors << "not implemented."; return QList<CloudServiceEntry*>();
        }
        virtual QList<CloudServiceEntry*> readdir(QString path, QStringList &errors, QDateTime from, QDateTime to) {
            Q_UNUSED(from);
            Q_UNUSED(to);
            return readdir(path, errors);
        }

        // UTILITY
        void mapReply(QNetworkReply *reply, QString name) { replymap_.insert(reply,name); }
        void mapReply(QNetworkReply *reply, QString name, quint64 operationId) {
            replymap_.insert(reply,name); writeOperationMap_.insert(reply,operationId);
        }
        QString replyName(QNetworkReply *reply) { return replymap_.value(reply,""); }
        quint64 replyWriteOperationId(QNetworkReply *reply) { return writeOperationMap_.take(reply); }
        void compressRide(RideFile*ride, QByteArray &data, QString id);
        RideFile *uncompressRide(QByteArray *data, QString id, QStringList &errors);
        QString uploadExtension();
        static void sslErrors(QWidget *parent, QNetworkReply* reply ,QList<QSslError> errors);

        // APPSETTINGS SYMBOLS - SERVICE SPECIFIC
        QString syncOnImportSettingName() const { return QString("%1/%2/syncimport").arg(GC_QSETTINGS_ATHLETE_PRIVATE).arg(id()); }
        QString syncOnStartupSettingName() const { return QString("%1/%2/syncstartup").arg(GC_QSETTINGS_ATHLETE_PRIVATE).arg(id()); }
        QString activeSettingName() const { return QString("%1/%2/active").arg(GC_QSETTINGS_ATHLETE_PRIVATE).arg(id()); }

        // PUBLIC INTERFACES. DO NOT REIMPLEMENT
        static bool upload(QWidget *parent, Context *context, CloudService *store, RideItem*);

        enum compression { none, zip, gzip };
        typedef enum compression CompressionType;

        CompressionType uploadCompression;
        CompressionType downloadCompression;
        enum uploadType { JSON, TCX, PWX, FIT, CSV } filetype;

        bool useMetric; // CloudService know distance or duration metadata (eg Today's Plan)
        bool useEndDate; // Dates for file entries use end date time not start (weird, I know, but thats how SixCycle work)

        QString message;

    signals:
        void writeComplete(quint64 operationId, QString id, QString message);
        void readComplete(QByteArray *data, QString id, QString message);

        // DEC-garmin-023 - the alternative to readComplete for a read that did
        // not happen. See notifyReadFailed above for the emitter's contract.
        void readFailed(QByteArray *data, QString id, QString reason);

    protected:

        // if you want a new filestoreentry struct
        // we manage them in the file store so you
        // don't have to. When the filestore is deleted
        // these entries are deleted too
        CloudServiceEntry *newCloudServiceEntry();

        // DEC-040 Stage 1 (W2) - RUN ONE REQUEST TO A BOUNDED, REPORTED OUTCOME.
        //
        // OWNERSHIP OF `reply` IS TRANSFERRED. blockingRequest disposes of it on
        // every path out, and the caller must not touch it afterwards - which is
        // why everything a caller could still want is snapshotted into the
        // returned RequestResult BEFORE disposal happens.
        //
        // ORDER, and it is the whole design: reconcile the outcome, THEN snapshot
        // the fields, THEN dispose, THEN verify that disposal changed nothing,
        // and only then return. The fourth step is not decoration: disposal
        // aborts the reply and the abort synthesises a finished() which is
        // genuinely delivered, so the guard that stops it rewriting the outcome
        // has to be observable. If it is ever removed, the verify step stops the
        // process with DEC040_POST_DISPOSAL_STATE_CHANGED rather than returning a
        // result derived from state that moved. abort() is never issued until the outcome is
        // already fixed, so the finished() that an abort provokes arrives after
        // reconciliation is over and cannot rewrite it. Correspondingly,
        // reply->isFinished() is NEVER consulted to decide an outcome - only the
        // naturalFinish_ flag, which the finished() slot sets solely while no
        // abort has been issued. Asking the reply "are you finished?" after an
        // abort gets the answer "yes", and that answer is precisely the false
        // success this contract exists to prevent.
        //
        // A natural finish carrying an error is NetworkError, never Finished, and
        // only Finished populates body.
        RequestResult blockingRequest(QNetworkReply *reply, int timeoutMs);

        QMap<QNetworkReply*,QString> replymap_;
        QMap<QNetworkReply*,quint64> writeOperationMap_;
        quint64 nextWriteOperationId_ = 0;
        QList<CloudServiceEntry*> list_;

        // DEC-040 Stage 1 (S-1) - THE ONLY WAY TO REACH THE NETWORK MANAGER.
        //
        // Every `nam->get(...)` / `nam->post(...)` in the thirteen migrated
        // services is now `nam()->get(...)`, because the manager may not exist
        // yet and this is where it comes into being. Making the accessor the only
        // route is what makes "lazy" safe: there is no member left to dereference
        // by mistake before it has been built.
        //
        // WHAT IT GUARANTEES
        //   - an injected manager is returned EXACTLY as supplied; no default is
        //     ever built alongside one.
        //   - otherwise exactly ONE default manager is built, on the first call,
        //     and every later call returns that same one.
        //   - the manager's thread affinity is THIS SERVICE'S affinity thread,
        //     and that is DETECTED rather than accommodated. Calling nam() from
        //     any thread other than thread() is a programming error and is
        //     rejected deterministically - see the affinity note below.
        //     Because the check runs first, the manager can then be built and
        //     parented in ONE operation, `new QNetworkAccessManager(this)`,
        //     which is the only construction that is correct by itself.
        //   - wireNam() is called EXACTLY ONCE, the first time a manager becomes
        //     available - see wireNam below.
        //
        // THE AFFINITY RULE, AND WHY IT IS NOT AN ASSERT.
        //
        // An earlier revision created the manager parentless on the CALLER's
        // thread and pushed it across with moveToThread(). That is
        // accommodation: it makes an off-affinity first use silently work, and
        // a QNetworkAccessManager driven from the wrong thread is undefined
        // behaviour whatever its affinity says. The contract is now the
        // opposite - off-affinity use is DETECTED and stops the process.
        //
        // It is not Q_ASSERT. This tree builds its providers, and the test
        // target that gates them, with -DNDEBUG -DQT_NO_DEBUG, so Q_ASSERT is
        // compiled out of both the shipping binary AND the binary the gate
        // runs. An invariant that disappears from every build that matters is
        // not an invariant. qFatal() is present in all builds; the diagnostic
        // carries the stable sentinel DEC040_NAM_OFF_AFFINITY so a death test
        // can match on it rather than on "exited nonzero".
        QNetworkAccessManager *nam();

        // DEC-040 Stage 1 (S-1) - PROVIDER-SPECIFIC WIRING, INSTALLED EXACTLY ONCE.
        //
        // Nine of the ten migrated services connected the manager's sslErrors
        // signal to their own onSslErrors slot in their CONSTRUCTOR. With lazy
        // creation there is no manager in the constructor, so that connect has to
        // move to the moment the manager appears - and it has to happen exactly
        // once, because dropping it silently disables SSL error handling and
        // repeating it pops the certificate dialog twice.
        //
        // This is a virtual, and that is safe HERE where it would not have been in
        // the constructor: nam() is only ever reached from a fully constructed
        // object, so the call dispatches to the subclass override as written. The
        // namWired_ latch is set BEFORE the call, so a wireNam implementation that
        // itself reaches nam() re-enters onto the already-built manager and does
        // not recurse.
        //
        // Dropbox has NO onSslErrors slot at all and never had one, so it does not
        // override this. That asymmetry is preserved deliberately: giving Dropbox
        // SSL error handling it never had would change how it behaves on a bad
        // certificate, which is not what this slice is for.
        virtual void wireNam(QNetworkAccessManager *) {}

        Context *context;

        // DEC-040 Stage 1 (S-1) - NULL until nam() materialises it, valid
        // afterwards, never indeterminate. Owned by this service (parented to it)
        // on both the default and the injected path, so it is destroyed exactly
        // once, with the service.
        QNetworkAccessManager *nam_;
        bool namWired_;

        // DEC-040 Stage 1 (S-1) - see setRequestTimeoutOverrideMs. Consumed at
        // exactly one point, inside blockingRequest.
        int requestTimeoutOverrideMs_ = -1;
};

// REQ-017 (b)/(e) - teardown of a store that an owner opened.
//
// Whoever creates a CloudService owns it, and on teardown must close() it and
// only THEN destroy it. close() is what performs the bounded session teardown -
// for GarminConnect that is stopping the download worker thread and releasing
// the embedded interpreter session - so a store
// that is merely dropped, or deleted without being closed, leaves that worker and
// that session alive until process exit. This is the same close()-then-delete
// idiom CloudServiceAutoDownload already uses (CloudService.cpp).
//
// This paragraph used to describe that worker stop as "quit()+wait(), never
// terminate()". That was FALSE of the shipped code and is corrected here:
// GarminDownloadChain and GarminAuthChain both run
// `quit(); if (!wait(kQuitWaitMs)) { terminate(); wait(kTerminateWaitMs); }`,
// each citing DES-001 invariant 3, which defines terminate()-as-last-resort as
// the DESIGNED escape hatch for a worker that will not come back - a bounded
// teardown has to terminate rather than block its owner forever. The flat "never"
// read as a project-wide prohibition and contradicted two shipped files. The
// narrower "never terminate() *from here*" at GarminConnect.cpp is accurate
// about its own call site and is deliberately left alone.
//
// The caller's pointer is cleared BEFORE close()/delete, so a second teardown of
// the same owner is a no-op that cannot double-delete, and no re-entrant path can
// observe a dangling store.
//
// Templated on the store type so the contract is exercisable in a unit test
// without linking the whole CloudService translation unit (see TEST-064).
template <typename Store>
void closeAndDeleteStore(Store *&store)
{
    if (store == NULL) return;

    Store *doomed = store;
    store = NULL;

    doomed->close();
    delete doomed;
}

// UPLOADER dialog to upload a single rideitem to the file
//          store. Typically as a quick ^U type operation or
//          via a MainWindow menu option
class CloudServiceUploadDialog : public QDialog
{

    Q_OBJECT

    public:
        // DEC-garmin-029 (A3-R027-F1) - TWO-PHASE INIT. The constructor builds
        // the widget SHELL ONLY and runs no nested event loop; start() does every
        // blocking thing (open the store, prompt about unsaved changes, compress
        // and kick off the write) on a fully-constructed object.
        CloudServiceUploadDialog(QWidget *parent, Context *context, CloudService *store, RideItem *item);

        // PHASE TWO. Returns false if the upload could not be started - or if a
        // parent teardown destroyed `this` inside one of those nested loops.
        // Callers MUST call it before exec(), and MUST NOT touch the dialog again
        // once it has returned false: on the teardown route the object is already
        // gone (only the returned bool survives).
        bool start();

        QLabel *info;               // how much being uploaded / status
        QProgressBar *progress;     // whilst we wait
        QPushButton *okcancel;      // cancel whilst occurring, ok once done

    public slots:
        int exec();
        void completed(quint64 operationId, QString name, QString message);

    private:
        Context *context;
        CloudService *store;
        RideItem *item;
        QByteArray data;            // compressed data to upload
        bool status;                // did upload get kicked off ok?
        quint64 operationId = 0;
};

// XXX a better approach might be to reimplement QFileSystemModel on 
// a CloudService and use the standard file dialogs instead. XXX
class CloudServiceDialog : public QDialog
{
    Q_OBJECT

    public:
        CloudServiceDialog(QWidget *parent, CloudService *store, QString title, QString pathname, bool dironly=false);
        QString pathnameSelected() { return pathname; }

    public slots:

        // trap enter pressed as we don't want to close on it
        bool eventFilter(QObject *obj, QEvent *evt);

        // user hit return on the path edit
        void returnPressed();
        void folderSelectionChanged();
        void folderClicked(QTreeWidgetItem*, int);

        // user double clicked on a file
        void fileDoubleClicked(QTreeWidgetItem*, int);

        // user typed or selected a path
        void setPath(QString path, bool refresh=false);

        // set the folder or files list
        void setFolders(CloudServiceEntry *fse);
        void setFiles(CloudServiceEntry *fse);

        // create a folder
        void createFolderClicked();

    protected:
        QLineEdit   *pathEdit; // full path shown
        QSplitter   *splitter; // left and right had side
        QTreeWidget *folders;  // left side folder list
        QTreeWidget *files;    // right side "files" list

        QPushButton *cancel;   // ugh. did the wrong thing
        QPushButton *open;     // open the selected "file"
        QPushButton *create;   // create a folder

        CloudService *store;
        QString title;
        QString pathname;
        bool dironly;
};

class FolderNameDialog : public QDialog
{
    Q_DECLARE_TR_FUNCTIONS(FolderNameDialog)
    public:
        FolderNameDialog(QWidget *parent);
        QString name() { return nameEdit->text(); }
        
        QLineEdit   *nameEdit; // full path shown
        QPushButton *cancel;   // ugh. did the wrong thing
        QPushButton *create;     // use name we just provided

};

//
// The Sync Dialog
//
class CloudServiceSyncDialog : public QDialog
{
    Q_OBJECT
    G_OBJECT


    public:
        CloudServiceSyncDialog(Context *context, CloudService *store);

        // REQ-017 (e) - this dialog OWNS the store it was handed: it opens it in
        // the constructor, so it closes and destroys it here. Callers must not
        // delete the store themselves.
        ~CloudServiceSyncDialog();
        int outstandingTransferCount() const { return readOperations.count() + writeOperations.count(); }

    public slots:

        // DEC-garmin-026 (A3-R025-F1) - PHASE TWO of two-phase init. The
        // constructor now builds only the widget SHELL and runs NO nested event
        // loop, so it can never be caught half-built by a parent teardown. This
        // slot carries everything the constructor's body used to do from
        // store->open() onward - the one and only nested-loop work - and it runs
        // on a COMPLETE object, so DEC-024's done()/closeEvent() gate and
        // DEC-025's destructor depth-check + BlockingCall QPointer already
        // protect it. Returns true when the store opened and the dialog is ready
        // to be shown/exec()'d; false on open-failure (the dialog then tears
        // itself down exactly as the old constructor's failure branch did) or if
        // `this` was destroyed inside one of its blocking calls.
        bool start();

        void cancelClicked();
        void refreshClicked();
        void tabChanged(int);
        void downloadClicked();
        void refreshCount();
        void refreshUpCount();
        void refreshSyncCount();
        void selectAllChanged(int);
        void selectAllUpChanged(int);
        void selectAllSyncChanged(int);

        void completedRead(QByteArray *data, QString name, QString message);

        // DEC-garmin-023 - the counterpart of completedRead on CloudService's
        // explicit failure channel. Does the same three things completedRead
        // does for a read that produced no ride: show the reason, free the
        // caller-preallocated buffer, advance the loop. Without it a service
        // that refuses leaves this dialog parked on "Downloading n of N".
        void failedRead(QByteArray *data, QString name, QString reason);

        void completedWrite(quint64 operationId, QString name, QString message);

        // DEC-garmin-024 (A3-R017-F1) - the LAST gate before this dialog is
        // allowed to destroy itself. QDialog::done() is what reject()/accept()
        // and the Escape key all funnel through, and on Qt it destroys a
        // WA_DeleteOnClose dialog EVEN IF closeEvent() ignored the close - so
        // overriding closeEvent() alone leaves Escape and reject() lethal
        // (measured, not assumed: see TEST-070/071). Deferring here covers every
        // one of them with a single decision.
        void done(int result) override;

    protected:

        // DEC-garmin-024 - the window X. Refuses the close while a store call
        // that can run a nested QEventLoop is on the stack; see deferCloseIfBusy.
        void closeEvent(QCloseEvent *e) override;

    private:
        Context *context;
        CloudService *store;
        QList<CloudServiceEntry*> workouts;

        bool downloading;
        bool sync;
        bool aborted;

        // DEC-garmin-024 (A3-R017-F1) - REENTRANCY GUARD.
        //
        // GarminConnect::readFile/readdir run NESTED QEventLoops (blockingDownload
        // / blockingList / blockingRestore), so GUI events - including the user
        // closing this dialog - ARE delivered in the middle of a call this dialog
        // made ON THE STORE. Since REQ-017 (e) this dialog deletes itself on close
        // and its destructor deletes the store, that close used to free the store
        // while readFile was still running on it, and free this dialog while
        // syncNext was still running on it: a use-after-free, reproduced under
        // AddressSanitizer by TEST-070.
        //
        // So a close arriving while such a call is on the stack is DEFERRED: the
        // sync is aborted (so the user's click visibly does something) and the
        // close is replayed once the last of those frames has unwound.
        //
        // A DEPTH COUNT rather than a bool because these frames can nest - the
        // QApplication::processEvents() inside a blocking call can dispatch a
        // click on Refresh, putting a readdir frame inside a readFile frame - and
        // a bool would be cleared by the inner frame while the outer one was
        // still live, reopening exactly the window this closes.
        int blockingCallDepth;
        bool closeDeferred;

        // Sets up the deferral and returns true when the caller must NOT proceed
        // with destroying/closing the dialog.
        bool deferCloseIfBusy();

        // DEC-garmin-031 (REQ-022) - THE ORPHAN RECORD, and the end of DEC-025's
        // deliberate leak.
        //
        // DEC-025 declined to delete the store when the destructor was reached
        // with a blocking call still suspended, and LEAKED it, on the stated
        // ground that "at parent-teardown time the application is already tearing
        // down, so there is no live event loop left for a reaper to run on".
        // REQ-021 falsified that: this dialog is now a child of context->tab, and
        // MainWindow::removeAthleteTab deletes that tab on routes after which the
        // application keeps running (two or more athlete tabs, or two or more
        // MainWindows). A once-per-exit leak became a per-close ACCUMULATING one -
        // and for GarminConnect each occurrence is a live OS thread plus a
        // resident interpreter session, because close() is what quit()+wait()s the
        // download worker and releases the adapter.
        //
        // WHY NOT store->deleteLater(). Deferred deletion is delivered by the
        // event loop the call was made FROM, and on this path that loop is the
        // store's OWN suspended nested loop - so deleteLater() frees the store
        // under the frame executing on it, which is the use-after-free DEC-025
        // exists to prevent. Not assumed: MEASURED by TEST-089, which posts a
        // deleteLater() and a Qt::QueuedConnection invokeMethod from inside a
        // nested QEventLoop through production's exact geometry and finds BOTH
        // delivered before that loop returns (with a plain-delete control proving
        // the apparatus sees deaths, and a late-delete control proving it can also
        // report life).
        //
        // So the trigger is not the event loop but the FRAME COUNT: the last
        // BlockingCall to unwind is, by construction, the point at which no store
        // call this dialog made is on the stack any more. That release performs
        // the close+delete the destructor could not.
        //
        // PER-DIALOG AND REFCOUNTED, not a static counter: two sync dialogs can
        // have suspended frames on the same stack, and one shared counter would
        // let either dialog's unwinding reap the other's store early. No
        // production route to two concurrent sync dialogs was found, but the bug
        // that shape would cause is silent, and the correct shape costs one
        // pointer. The record is created by the FIRST BlockingCall and held by
        // every one of them, so it OUTLIVES the dialog exactly as the frames do.
        //
        // See TEST-090 (the mechanism, including the two-dialog case), TEST-072
        // and TEST-084 (the store is now closed and deleted exactly once, and only
        // after the nested loop returned) and TEST-073 (the idle control: an
        // ordinary teardown still closes and deletes promptly, so this cannot have
        // merely relocated the leak).
        class StoreReaper
        {
            public:
                StoreReaper() : refs(0), orphan(NULL) {}

                // One more BlockingCall frame is holding this record.
                void retain() { refs++; }

                // Take ownership of the store the destructor declined to delete.
                // Called only by ~CloudServiceSyncDialog, at most once - the
                // dialog is being destroyed as it calls this.
                void adopt(CloudService *store) { orphan = store; }

                // Release ONE frame. The last release closes+deletes whatever was
                // adopted and destroys the record; a record nobody adopted a store
                // into simply goes away.
                void release();

            private:
                // Destroyed only by its own last release().
                ~StoreReaper() {}

                StoreReaper(const StoreReaper &);            // not copyable
                StoreReaper &operator=(const StoreReaper &); // not assignable

                int refs;             // live BlockingCall frames holding this
                CloudService *orphan; // the store the destructor could not delete
        };

        // NULL whenever no blocking call is on the stack. Created by the first
        // BlockingCall and dropped by the last, so this pointer is only ever read
        // while at least one frame holds the record alive.
        StoreReaper *reaper;

        // RAII, deliberately: an early return, a `continue`, or an exception out
        // of a store call must not leave the depth count raised, or this dialog
        // could never be closed again.
        //
        // DEC-garmin-025 (A3-R017b-F1) - the back-pointer is a QPointer because
        // this object OUTLIVES the dialog on the parent-teardown path: Qt
        // destroys child widgets directly from ~QObject, with a store call - and
        // therefore one of these - still on the stack. A raw pointer here means
        // ~BlockingCall writes blockingCallDepth into freed memory as the very
        // first thing that runs when the nested loop unwinds.
        class BlockingCall
        {
            public:
                explicit BlockingCall(CloudServiceSyncDialog *dialog);
                ~BlockingCall();

            private:
                BlockingCall(const BlockingCall &);            // not copyable
                BlockingCall &operator=(const BlockingCall &); // not assignable
                QPointer<CloudServiceSyncDialog> dialog;

                // DEC-garmin-031 - a RAW pointer on purpose, and the reason the
                // record is refcounted: it must stay readable after the QPointer
                // above has gone null, because the whole job of this frame's
                // unwinding is to hand one release to a record the DIALOG no
                // longer exists to own.
                StoreReaper *reaper;
        };

        // Quick lists for checking if file exists
        // locally (rideFiles) or remotely (uploadFiles)
        QStringList rideFiles;
        QStringList uploadFiles;

        // keeping track of progress...
        int downloadcounter,    // *x* of n downloading
            downloadtotal,      // x of *n* downloading
            successful,         // how many downloaded ok?
            listindex;          // where in rideList we've got to

        // DEC-garmin-032 (REQ-027) - WHICH BATCH A SUSPENDED FRAME BELONGS TO.
        //
        // syncNext/uploadNext/downloadNext keep their position in `listindex`, a
        // MEMBER, but the loop they are executing lives in a stack frame. That was
        // survivable while every branch that suspended also RETURNED: one frame,
        // one batch. DEC-032 made the parse-failure branch `continue` instead, so
        // there is now a frame that SUSPENDS AND THEN KEEPS ITERATING - and the
        // event delivered inside that suspension can be a second click on the
        // download button.
        //
        // Two clicks in one processEvents() burst is all it takes: the first is an
        // ABORT (downloadClicked :1910-1918), the second a fresh START
        // (:1919-1926), which resets `aborted` to false, `listindex` to 0 and the
        // counters, and re-drives the loop from inside the first batch's own
        // suspended frame. When that returns, the suspended frame resumes with
        // `aborted == false` - so the abort re-read cannot see this - and carries
        // on transferring rows that now belong to somebody else, addressing them
        // by a `listindex` the live batch is also moving (:2193/:2242/:2249/:2311/
        // :2318/:2457/:2464).
        //
        // So each batch is NUMBERED. The number is bumped by the start branch and
        // snapshotted into a LOCAL at loop entry; a frame whose snapshot no longer
        // matches is not the current batch and stands down. A counter rather than
        // a bool because the question is "is this still MY batch", which two
        // successive restarts would answer wrongly with any flag that only toggles.
        //
        // Wraparound is not guarded: it needs 2^31 clicks on one dialog, and the
        // failure it would cause is one stale frame surviving one iteration.
        // See TEST-100.
        int batchGeneration;

        // DEC-garmin-034 (REQ-028) - WHICH LIST A SUSPENDED FRAME BELONGS TO.
        //
        // batchGeneration above answers "is this still MY batch". It cannot answer
        // the other half, because a Refresh starts no batch: refreshClicked
        // (:1516) DELETES every QTreeWidgetItem in all three lists and rebuilds
        // them, and it is reachable from inside any of this dialog's nested event
        // loops - the Refresh button is never disabled (four references in this
        // file, not one of them a setEnabled) and :2126-2134 already says so.
        //
        // Three of this project's decisions - DEC-025, DEC-030, DEC-032 - each
        // enumerated what must survive a suspension, and all three enumerated
        // `this`, `store` and `context`. NONE ever enumerated the ROW. Both
        // transfer loops capture `QTreeWidgetItem *curr` BEFORE openRideFile's
        // nested loop and use it after (syncNext, uploadNext), and the three
        // completion slots address rows POSITIONALLY as `child(listindex-1)` in a
        // list that a Refresh may have replaced under them. The first is a live
        // heap-use-after-free (PROBE-A, re-measured by TEST-108); REQ-027 widened
        // it from a read to a WRITE.
        //
        // So each generation of the LISTS is numbered too. The counter is bumped
        // by refreshClicked before it deletes anything; the two drivers snapshot
        // it into a LOCAL at loop entry, exactly as they do batchGeneration, and
        // stand down if it has moved while they were suspended. The completion
        // slots have no "before" of their own - they ARE the resumption - so the
        // batch's value is snapshotted into batchListGeneration when the batch
        // starts (downloadClicked) and they compare against that: any Refresh
        // between the start of a batch and one of its completions invalidates the
        // positional addressing that completion is about to do, whichever row it
        // was for.
        //
        // WHAT THIS DOES NOT DO, so nobody reads more into it than the mechanism
        // delivers: it validates the CONTAINER, not the ELEMENT. It is sound only
        // while a whole-list rebuild is the only way a row dies - true when this
        // was written and re-verified since (CloudService.cpp:1570/:1577/:1584 are
        // the only tree-item deletes in that file), but that is an invariant, not
        // a guarantee, and a future mutator that deletes ONE row defeats it.
        //
        // AND IT IS ONLY WORTH THE PLACES IT IS COMPARED AT (A3-R028b-F8). The
        // counter cannot fire by itself: each frame has to re-compare it after
        // each of ITS OWN suspensions, and NOTHING IN THIS CODEBASE HOLDS A
        // COMPLETE LIST OF THOSE. Two were missed by the wave that introduced this
        // and found afterwards - saveRide, which suspends inside
        // DataProcessorFactory::autoProcess (A3-R028b-F1), and the two drivers'
        // parse-failure processEvents(), which suspend and then keep iterating
        // (A3-R028b-F2). Any enumeration of
        //
        // CITED BY SYMBOL, NOT BY LINE, DELIBERATELY (A3-R038-F3, [[LSN-034]]
        // refinement 8). This paragraph carried three line numbers and by
        // 2026-08-23 all three pointed at unrelated statements - the file had
        // grown under them. A comment is the one channel nothing re-reads: a
        // briefing gets checked by the agent it is sent to, but a stale citation
        // in source is found only if an adversary happens to be aimed at this
        // file. Grep the symbol instead; it survives every edit above it.
        // suspension points in these files, including DEC-029's four store calls,
        // is a list of the ones someone has looked at. Adding a call that can
        // suspend means adding a compare beside it.
        //
        // It also does NOT see the SORT route (S-R028-01), and nothing here ever
        // will: a sort REORDERS rows without freeing any, so it bumps no counter
        // and passes every guard here BY CONSTRUCTION. That route is closed
        // somewhere else entirely - DEC-garmin-038 takes the mutator away for the
        // batch's duration (suspendListSorting/restoreListSorting below) rather
        // than trying to detect it, because there is nothing here for it to
        // detect. See TEST-132..TEST-139. And it cannot tell a LATE completion of
        // an ABORTED batch from a live one (REQ-028 (c)): two transfers are
        // outstanding at once there, and a single shared snapshot is overwritten
        // by the restarted batch's own dispatch before the stale completion
        // arrives - which is what DEC-garmin-036's per-transfer ticket below was
        // added for, and it is the ticket, not this counter, that answers it.
        // Wraparound is not guarded, for batchGeneration's reason.
        // See TEST-108, TEST-109, TEST-110.
        int listGeneration;      // bumped by refreshClicked
        int batchListGeneration; // ...and what it read when this batch started

        // DEC-garmin-038 (= DEC-garmin-034 AMENDMENT, S-R028-01 / S-R028-02) -
        // THE THIRD LIST MUTATOR: SORTING, AND WHY IT IS TAKEN AWAY RATHER THAN
        // TRACKED.
        //
        // The two counters above answer "is this still my batch" and "is this
        // still my list". A SORT makes both answer YES and still breaks the
        // drivers: it frees nothing, deletes nothing and starts no batch - it
        // PERMUTES the rows - so it bumps no counter and passes every guard in
        // this class by construction, while the three drivers walk their lists
        // POSITIONALLY (`for (int i=listindex; ...) child(i)`).
        //
        // AND IT NEEDS NO CLICK. MEASURED by TEST-131 on Qt 6.8.2, both QPA
        // backends: with sortingEnabled == true a setText on the SORT COLUMN
        // reorders the list INSIDE the write (layoutChanged is emitted by setText;
        // there is no lazy, not-yet-applied window and QTreeWidgetItem::child()'s
        // executePendingSort() is NOT what delivers it). All three drivers write
        // the Status column on every dispatch, and all three lists are sortable by
        // Status out of the constructor - so a user who sorted by Status at any
        // earlier time makes the batch reorder its own list underneath its own
        // loop. End to end, with no interaction during the batch at all: the same
        // row uploaded twice and the other never transferred.
        //
        // SO THE MUTATOR IS REMOVED FOR THE BATCH'S DURATION. suspendListSorting()
        // is called where the batch STARTS - downloadClicked, the one place a
        // batch can start - and BEFORE its first status write, which is the whole
        // of what makes this guard load-bearing: a disable arriving after a write
        // cannot undo the move that write already made (TEST-131 Q3). It closes
        // BOTH vectors, not only the data one: QTreeView::setSortingEnabled(false)
        // clears sectionsClickable and sortIndicatorShown too, and a
        // setSortIndicator made while it is off moves nothing (TEST-131 Q4).
        //
        // A BATCH IS NOT ONE STACK FRAME, which is why this is a MEMBER pair and
        // not an RAII scope guard. The drivers process one row, return, and let a
        // completion slot re-drive them; a guard scoped to a driver CALL would
        // re-enable sorting BETWEEN ROWS and reinstate the defect while looking
        // correct. See TEST-139, which samples inside the first completion.
        //
        // WHERE IT IS RESTORED, and the enumeration is the decision's, not the
        // implementation's afterthought: every path that ENDS a batch while this
        // dialog is still alive.
        //   * the three completion tails (the batch finished)
        //   * downloadClicked's abort branch (the user stopped it)
        //   * refreshClicked (the lists are being rebuilt; every driver and every
        //     completion of the running batch then stands down at its
        //     listGeneration compare and NOTHING re-drives, so no tail can be left
        //     to do it - and the restore is at the TOP of that function so the
        //     early return on a freed Context is covered too)
        // A path that exits WITHOUT restoring leaves the user's lists permanently
        // unsortable until the dialog is reopened: a silent UX regression no
        // memory-safety test would catch. See TEST-136 for one run per class.
        //
        // WHAT IS DELIBERATELY *NOT* A RESTORE SITE:
        //   * the stale-frame exits (`batchGeneration != generation`). A frame
        //     standing down there is NOT the live batch - restoring would turn
        //     sorting back on underneath the batch that is still running.
        //   * the self.isNull() exits. The dialog is already destroyed and these
        //     are its members; the restore is only ever called from paths on which
        //     `this` is known alive, which is why no bail below one of those
        //     guards calls it.
        //   * the driver's own `aborted` exits, which are downstream of the abort
        //     branch that has already restored.
        //   * deferCloseIfBusy(), which DOES end a batch and does NOT restore -
        //     the fourth terminating class, missing from this list until the
        //     DEC-038 A3 found it (A3-R038-F1). It is safe because every
        //     deferred close terminates in destruction, so no live dialog
        //     survives to have unsortable lists; the full argument, and the
        //     condition under which it would BECOME a restore site, is written
        //     out at deferCloseIfBusy in CloudService.cpp.
        //
        // THE USER'S COLUMN AND ORDER SURVIVE. setSortingEnabled(true) re-applies
        // whatever indicator the header carries, and on a header that never had
        // one that is section 0 / DESCENDING (measured, TEST-131 Q1a), so the
        // pre-batch (section, order) is recorded per list and re-applied BEFORE
        // sorting is switched back on - one sort, on the user's own column.
        // See TEST-137.
        //
        // ACCEPTED RESIDUAL, recorded rather than hidden: this is PREVENTION, not
        // TOLERANCE. A future caller that re-enables sorting mid-batch
        // reintroduces the defect and no architectural guard would catch it; the
        // drivers become intrinsically reorder-tolerant only if they ever dispatch
        // from a snapshot of row POINTERS instead of an index (DEC-038's rejected
        // Option B, which stays additive on top of this). Second residual: the
        // header really is inert during a batch - a deliberate UX cost.
        struct ListSortState
        {
            bool enabled = true;
            int section = -1;
            Qt::SortOrder order = Qt::AscendingOrder;
        };

        void suspendListSorting(); // at the batch's start, before its first write
        void restoreListSorting(); // at every termination path, and only those

        bool sortingSuspended;   // is a batch's suspension in force?
        ListSortState downSortState, upSortState, syncSortState;

        // DEC-garmin-036 (REQ-028 (c)) - WHICH TRANSFER A COMPLETION BELONGS TO.
        //
        // The two counters above answer "is this still my batch" and "is this
        // still my list". Neither can answer the third question, and the comment
        // above already admits it: on the abort-and-restart route TWO transfers
        // are outstanding at once, and a single shared snapshot is overwritten by
        // the restarted batch's own dispatch before the abandoned batch's
        // completion arrives, so it reads EQUAL and passes every guard. The
        // completion then labels whichever row the LIVE batch happens to be
        // transferring, advances that batch's progress bar for work it never did,
        // and re-drives the loop - a second driver over the same list (measured
        // by TEST-107 before this, asserted by TEST-113/115 after it).
        //
        // So the completion is made to carry an identity, and the ticket below is
        // where the dispatch leaves it. It is ARMED immediately before each of the
        // four store calls that can produce a completion - syncNext's read and its
        // write, downloadNext's read, uploadNext's write - and CONSUMED at the top
        // of each of the three completion slots. What it records is exactly what
        // the slot cannot work out for itself:
        //
        //   token   the buffer the read was issued with, freshly `new`ed one line
        //           earlier. readComplete/readFailed hand back that same pointer -
        //           a contract that was unwritten until DEC-036 and is now stated
        //           on notifyReadComplete (:145) and notifyReadFailed - so pointer
        //           identity IS transfer identity on the read paths. Compared,
        //           never dereferenced.
        //   write id the opaque value allocated at dispatch and returned by
        //           writeComplete. Remote names remain data, never identity.
        //   row     the QTreeWidgetItem the transfer was issued FOR, already in
        //           hand at the dispatch. This replaces the seven positional
        //           `child(listindex-1)` dereferences in the three slots: a
        //           completion now labels the row it was about instead of the row
        //           that currently sits at the batch's bookmark.
        //   col     ...and the status column that row is labelled in (7 on the
        //           sync and upload lists, 5 on the download list), so the arm
        //           site is the single source of it.
        //
        // Records are one-shot: admission removes the exact identity from its
        // map, so a second completion cannot count or drive again.
        //
        // An abort does not cancel a remote transfer. Its operation record stays
        // in the map until that exact completion arrives or the dialog is
        // destroyed. A restart can therefore have its own record concurrently;
        // equal remote names do not collide because write ids are distinct.
        // Refresh keeps the operation identity but nulls its row, so a late
        // completion remains consumable without touching a deleted item.
        //
        // WHAT THIS DOES NOT DO. `row` is a raw pointer and this struct does not
        // prove it alive; the DEC-034 compare immediately above every consumption
        // site is what does that, which is why that compare stays and why the
        // ticket is consumed BELOW it and never above it.
        //
        // WHAT IT DOES FOR THE SORT ROUTE, and where the other half went. This
        // ticket is what makes the LABELLING half of S-R028-01 sort-proof: the
        // completion labels the row it was issued FOR, and a permutation does not
        // move a pointer. The DRIVER half - `for (int i=listindex; ...)` walking a
        // list that reordered under it - is NOT closed here and cannot be, because
        // this ticket says nothing about positions. It is closed by
        // DEC-garmin-038, which suspends sorting for the batch's duration so the
        // permutation never happens (suspendListSorting above; TEST-132..TEST-139).
        // The two are adjacent and independent: the ticket still carries the row
        // through a sort that happens BETWEEN batches.
        struct TransferOperation {
            QTreeWidgetItem *row = nullptr;
            int col = 0;
            int generation = 0;
            int listGeneration = 0;
        };
        QMap<QByteArray*, TransferOperation> readOperations;
        QMap<quint64, TransferOperation> writeOperations;

        bool saveRide(RideFile *, QStringList &);
        bool syncNext();        // kick off another download/upload
                                // returns false if none left
        bool downloadNext();    // kick off another download
                                // returns false if none left
        bool uploadNext();     // kick off another upload
                                // returns false if none left

        // tabs - Upload/Download
        QTabWidget *tabs;

        // athlete selection
        //QMap<QString, QString> athlete;
        QComboBox *athleteCombo;

        QPushButton *refreshButton;
        QPushButton *cancelButton;
        QPushButton *downloadButton;

        QDateEdit *from, *to;

        // Download
        QCheckBox *selectAll;
        QTreeWidget *rideListDown;

        // Upload
        QCheckBox *selectAllUp;
        QTreeWidget *rideListUp;

        // Sync
        QCheckBox *selectAllSync;
        QTreeWidget *rideListSync;
        QComboBox *syncMode;

        // show progress
        QProgressBar *progressBar;
        QLabel *progressLabel;

        QCheckBox *overwrite;
};

// Representing a File or Folder
class CloudServiceEntry
{
    public:

        // THESE MEMBERS NEED TO BE MAINTAINED BY
        // THE FILESTORE IMPLEMENTATION (Dropbox, Google etc)
        QString name;                       // file name
        QString label;                      // alternate name
        QString id;                         // file id
        bool isDir;                         // is a directory
        unsigned long size;                 // my size
        QDateTime modified;                 // last modification date
        double distance;                    // distance (km)
        long duration;                      // duration (secs)

        // This is just file metadata written by the implementation.
        //QMap<QString, QString> metadata;
        // THESE MEMBERS ARE MAINTAINED BY THE 
        // FILESTORE BASE IMPLEMENTATION
        CloudServiceEntry *parent;             // parent directory, NULL for root.
        QList<CloudServiceEntry *> children;   // parent directory, NULL for root.
        bool initial;                       // haven't scanned for children yet.

        // find the index of a child, return -1 if not found
        int child(QString directory) {
            bool found = false;
            int i = 0;
            for(; i<children.count(); i++) {
                if (children[i]->name == directory) {
                    found = true;
                    break;
                }
            }
            if (found) return i;
            else return -1;
        }
};

struct CloudServiceDownloadEntry {

    CloudServiceEntry *entry;
    QByteArray *data;
    CloudService *provider;
    enum { Pending, InProgress, Failed, Complete } state;

};

class CloudServiceAutoDownload : public QThread {

    Q_OBJECT

    public:

        // automatically downloads from cloud services
        CloudServiceAutoDownload(Context *context) : context(context), initial(true) {}

        // re-run after inital
        void checkDownload();

        // DEC-040 Stage 1 - WHY AN EMPTY LISTING IS NOT ONE THING.
        //
        // run() asked each service for a listing and then branched on
        // found.count() alone, so it had exactly two states where there are
        // three: it could tell "entries" from "no entries", but it could not tell
        // "no entries because there is nothing new" from "no entries because the
        // listing FAILED". The `errors` QStringList that readdir fills was
        // constructed, passed in by reference, populated on failure, and then
        // dropped on the floor unread.
        //
        // That was survivable while an unbounded listing simply never returned:
        // the thread parked and there was no empty list to misread. Bounding the
        // wait (W2) turns that hang into a prompt, silent, EMPTY listing - so the
        // failure mode the bound removes would have come straight back as "auto
        // download finished, nothing to do", which is worse than a hang because
        // it is quiet and looks like success.
        //
        // So the errors are kept. This is the observable the refusal needs: a
        // caller can distinguish the two empty cases, which is the only thing
        // that makes readdir's failure report a report at all rather than a
        // local variable. Reset at the top of every run().
        QStringList autoDownloadErrors() const { return autoDownloadErrors_; }

    public slots:

        // external entry point to trigger auto download
        void autoDownload();

        // thread worker to generate download requests
        void run();

        // receiver for downloaded files to add to the ridecache
        void readComplete(QByteArray*,QString,QString);

        // DEC-garmin-023 - receiver for reads that did NOT happen. Frees the
        // preallocated buffer and reports the reason; run()'s blocking
        // per-activity QEventLoop is released by this signal too, so a refusal
        // costs the next activity nothing instead of burning the 30s watchdog.
        void readFailed(QByteArray*,QString,QString);

    private:

        Context *context;
        bool initial;

        // DEC-040 Stage 1 - see autoDownloadErrors() above.
        QStringList autoDownloadErrors_;

        // list of files to download
        QList <CloudServiceDownloadEntry> downloadlist;

        // list of providers - so we can clean up
        QList<CloudService*> providers;
};

// all cloud services register at startup and can be accessed by name
// which is typically the website name e.g. "Todays Plan"
class CloudServiceFactory {

    static CloudServiceFactory *instance_;
    QHash<QString,CloudService*> services_;
    QStringList names_;

    public:

    // update settings to new scheme (try and guess which services have
    // been configured and set them active so they are processed etc
    static void upgrade(QString name);

    // get the instance
    static CloudServiceFactory &instance() {
        if (!instance_) instance_ = new CloudServiceFactory();
        return *instance_;
    }

    // how many services
    int serviceCount() const { return names_.size(); }
    QHash<QString,CloudService*> serviceHash() const { return services_; }

    void initialize() {
        foreach(const QString &service, services_.keys())
            services_[service]->initialize();
    }

    // sorted list of service names
    const QStringList serviceNames() const { QStringList returning = names_;
                                              std::sort(returning.begin(), returning.end(), Utils::qstringascend);
                                              return returning; }

    const CloudService *service(QString name) const { return services_.value(name, NULL); }
    const QList<CloudService*> services() {
        QList<CloudService*>returning;
        QHashIterator<QString,CloudService*> i(services_);
        i.toFront();
        while(i.hasNext()) {
            i.next();
            returning << i.value();
        }
        return returning;
    }

    void saveSettings(CloudService *service, Context *context) {

        QHashIterator<CloudService::CloudServiceSetting,QString> want(service->settings);
        want.toFront();
        while(want.hasNext()) {
            want.next();

            // key might need parsing
            QString key;
            if (want.value().contains("::")) key = want.value().split("::").at(0);
            else key = want.value();

            // get value
            QString value = service->getSetting(key, "").toString();

            if (value == "") continue;

            // ok, we have a setting
            appsettings->setCValue(context->athlete->cyclist, key, value);

            #ifdef GC_WANT_ALLDEBUG
            qDebug()<<"factory save setting:" <<key<< value;
            #endif
        }

        // generic settings
        QString syncstartup = service->getSetting(service->syncOnStartupSettingName(), "").toString();
        if (syncstartup != "")  appsettings->setCValue(context->athlete->cyclist, service->syncOnStartupSettingName(), syncstartup);

        QString syncimport = service->getSetting(service->syncOnImportSettingName(), "").toString();
        if (syncimport != "")  appsettings->setCValue(context->athlete->cyclist, service->syncOnImportSettingName(), syncimport);

    }

    CloudService *newService(const QString &name, Context *context) const {

        // INSTANTIATE FOR THIS CONTEXT
        #ifdef GC_WANT_ALLDEBUG
        qDebug()<<"factory instantiate:" << name;
        #endif
        CloudService *returning = services_.value(name)->clone(context);


        // INJECT CONFIGURATION
        QHashIterator<CloudService::CloudServiceSetting, QString> i(returning->settings);
        i.toFront();
        while (i.hasNext()) {
            i.next();

            // ignore default URL
            if (i.key() == CloudService::DefaultURL) continue;

            // the setting name
            QString sname=i.value();

            // Combos and Metadata are tricky
            if (i.key() == CloudService::Combo1) { sname = i.value().split("::").at(0); }
            if (i.key() == CloudService::Metadata1) { sname = i.value().split("::").at(0); }

            // populate from appsetting configuration
            QVariant value = appsettings->cvalue(context->athlete->cyclist, sname, "");

            // apply default url
            if (i.key() == CloudService::URL && value == "") {
                // get the default value for the service
                value = returning->settings.value(CloudService::CloudServiceSetting::DefaultURL, "");
            }
            returning->configuration.insert(sname, value);

            #ifdef GC_WANT_ALLDEBUG
            qDebug()<<"set:"<<sname<<"="<<value;
            #endif
        }

        // add sync on import, syncstartup and active
        QVariant value = appsettings->cvalue(context->athlete->cyclist, returning->syncOnImportSettingName(), "false").toString();
        returning->configuration.insert(returning->syncOnImportSettingName(), value);
        value = appsettings->cvalue(context->athlete->cyclist, returning->syncOnStartupSettingName(), "false").toString();
        returning->configuration.insert(returning->syncOnStartupSettingName(), value);
        value = appsettings->cvalue(context->athlete->cyclist, returning->activeSettingName(), "false").toString();
        returning->configuration.insert(returning->activeSettingName(), value);

        // DONE
        return returning;
    }

    bool addService(CloudService *service) {

        // duplicates not welcome
        if(names_.contains(service->id())) return false;

        // register - but must never use, since it has a NULL context
        services_.insert(service->id(), service);
        names_.append(service->id());

        return true;
    }

};

class CloudServiceAutoDownloadWidget : public QWidget
{

    Q_OBJECT

    Q_PROPERTY(int transition READ getTransition WRITE setTransition)

    public:
        CloudServiceAutoDownloadWidget(Context *context,QWidget *parent);

        // transition animation 0-255
        int getTransition() const {return transition;}
        void setTransition(int x) { if (transition !=x) {transition=x; update();}}

    protected:
        void paintEvent(QPaintEvent*);

    public slots:

        void downloadStart();
        void downloadFinish();
        void downloadProgress(QString s, double x, int i, int n);

    private:

        Context *context;
        enum { Checking, Downloading, Dormant } state;
        double progress;
        int oneof, total;
        QString servicename;

        // animating checking
        QPropertyAnimation *animator;
        int transition;
};

#endif
