/*
 * Copyright (c) 2026 GoldenCheetah Contributor
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the Free
 * Software Foundation; either version 2 of the License, or (at your option)
 * any later version.
 */

// TEST garmin: T-167/T-168/T-169 — REQ-023 (finding S-R021-05), the STORE-layer
// collaborator use-after-free on the download path.
//
// Finding text (verbatim):
//   "CloudService itself holds Context *context (CloudService.h:260, plus a
//    shadowing copy in all 17 subclass headers) and dereferences
//    context->athlete->home->temp() inside CloudService::uncompressRide
//    (CloudService.cpp:301), on the download path (completedRead /
//    CloudServiceAutoDownload::readComplete)."
//
// ACCEPTANCE (verbatim): "a Context freed while a frame that will reach the
// :301 deref (context->athlete->home->temp() in CloudService::uncompressRide)
// is still live must no longer fault — proven by an EXECUTED ASan test, with
// the guard at the STORE layer (inside CloudService's own frame), not the
// dialog."
//
// CENSUS THAT SHAPED THESE SLOTS (re-derived 2026-09-07; the 2026-08-10 line
// numbers had shifted — the deref is now the `context->athlete->home->temp()`
// chain immediately before openRideFile inside CloudService::uncompressRide).
// Exactly TWO frames reach it, and nothing between either caller's entry and
// the deref SUSPENDS (the suffix gate, the QTemporaryFile write, the
// ZipReader/gUncompress work are all synchronous):
//
//   ROUTE 1  CloudServiceSyncDialog::completedRead -> store->uncompressRide
//            GUI thread (store and dialog are same-thread, so the
//            store->readComplete connection is DIRECT). The slot's guards
//            cover the dialog (QPointer self), the ticket, the batch/list
//            generations and `aborted` — NONE of them is a Context-liveness
//            check, and none can be: the deref is in the STORE's frame, which
//            is exactly why DEC-030 constraint 2 ("the guard sits on the layer
//            performing the unsafe operation") routed this finding here.
//   ROUTE 2  CloudServiceAutoDownload::readComplete -> provider->uncompressRide
//            QUEUED metacall delivered on the GUI thread; its head carries
//            DEC-043's stopRequested_ bail, which closes this route to the
//            deref for every teardown that flips the flag before freeing
//            (Athlete::close() does requestStop() -> wait() -> delete, and
//            MainWindow::removeAthleteTab deletes the Context only after
//            athlete->close()). TEST-159 is that guard's executed proof. The
//            store-layer guard below is what covers the state TEST-159's flag
//            cannot express: an owner that frees the Context WITHOUT flipping
//            the flag — which is every owner on Route 1, the dialog path,
//            where no stop flag exists at all.
//
// uncompressRide DOES suspend — at RideFileFactory::openRideFile's FIT-read
// nested QEventLoop — but AFTER the deref under test, and no statement after
// that loop dereferences context again (file.remove(); return). So the guard
// belongs before the deref, which is also function entry.
//
// WHAT IS REAL HERE: the REAL src/Cloud/CloudService.cpp is compiled in (same
// seam as testGarminConnectReadFailedConsumer — stubs/ImportSeamStubs.cpp plus
// stubs/SyncDialogSeamStubs.cpp for the RideCache the dialog walk touches),
// so the uncompressRide under test is the shipping one, and the dialog driven
// in T-168 is the real CloudServiceSyncDialog driven only through public
// slots and its real widget tree.
//
// NOT COVERED HERE — and honestly so:
//   * Route 2 is not re-driven here. Its head guard is TEST-159's executed
//     territory, and reaching readComplete requires CloudServiceAutoDownload's
//     private downloadlist, which run() only populates via a worker-thread
//     program (the same "that is a program, not a seam" verdict
//     testGarminConnectReadFailedConsumer recorded). The census maps it.
//   * The DIALOG layer's own unguarded Context dereferences found while
//     censusing — downloadNext/syncNext's completion tail
//     (context->athlete->rideCache->save()) and readComplete's post-suspension
//     context->athlete->home->activities() past uncompressRide's nested loop —
//     are outside REQ-023's letter (the finding names the STORE's deref) and
//     outside the briefing's two guard patterns for this REQ. They are why
//     T-168 keeps a SECOND row in the batch: after the store bails, the
//     completion tail re-drives syncNext onto row 2's dispatch and returns
//     BEFORE that tail, so this suite never faults in a frame it has no
//     mandate to fix. Reported, not hidden.

#include "Athlete.h"
#include "CloudService.h"
#include "Context.h"
#include "RideCache.h"
#include "RideFile.h"
#include "zipwriter.h"

#include <QApplication>
#include <QByteArray>
#include <QCheckBox>
#include <QDate>
#include <QDateTime>
#include <QDir>
#include <QFile>
#include <QList>
#include <QString>
#include <QStringList>
#include <QTemporaryDir>
#include <QTemporaryFile>
#include <QTreeWidget>
#include <QTreeWidgetItem>
#include <QtTest/QtTest>

#if !defined(__SANITIZE_ADDRESS__) && !defined(GC_TEST_WITH_ASAN)
#    error \
        "T-167/T-168 are lifetime tests and are only trustworthy under AddressSanitizer; build this target with -fsanitize=address."
#endif

// ---------------------------------------------------------------------------
// The store under (indirect) test: a CloudService whose every collaborator the
// dialog path needs is scripted, and whose readFile RECORDS the preallocated
// buffer and returns true ("a completion is coming") WITHOUT emitting — the
// test decides when the completion is delivered, which is the whole point: the
// Context must die BETWEEN the dispatch and the delivery.
// ---------------------------------------------------------------------------
class SpyStore : public CloudService
{
    Q_OBJECT

  public:
    explicit SpyStore(Context* context) : CloudService(context)
    {
        // uncompressRide's first gate rejects an uncompressed name when the
        // store claims a compression it was not given — with `none` the
        // .fit/.json names the dialog builds reach the context deref (same
        // fixture note as TEST-159's TearDownDuringReadFileService).
        downloadCompression = none;
    }

    CloudService* clone(Context* context) override { return new SpyStore(context); }
    QString id() const override { return QStringLiteral("UncompressLifetimeSpy"); }
    QString uiName() const override { return QStringLiteral("Uncompress Lifetime Spy"); }
    QImage logo() const override { return QImage(); }
    // The production capability set: the dialog only adds its Upload tab for a
    // service that advertises Upload, and its default tab is index 2 (sync) —
    // a Download-only service would put index 2 out of range and
    // downloadClicked would drive the wrong path.
    int capabilities() const override { return OAuth | Upload | Download | Query; }
    QString home() override { return QString(); }

    bool open(QStringList&) override { return true; }
    bool close() override { return true; }

    QList<CloudServiceEntry*> readdir(QString, QStringList&, QDateTime, QDateTime) override
    {
        QList<CloudServiceEntry*> out;
        for (const QString& name : entryNames) {
            CloudServiceEntry* e = newCloudServiceEntry();
            e->name = name;
            e->id = name;
            e->isDir = false;
            e->size = 0;
            e->modified = QDateTime::currentDateTime();
            out << e;
        }
        return out;
    }

    bool readFile(QByteArray* data, QString, QString, CloudService::ReadFileArmed* = nullptr) override
    {
        readBuffers << data; // the ticket downloadNext/syncNext armed was keyed on THIS pointer
        return true;         // a completion is coming — the test delivers it
    }

    QStringList entryNames;
    QList<QByteArray*> readBuffers;
};

// The dialog's lists are private; observed only through the real widget tree,
// exactly as testGarminConnectReadFailedConsumer harvests them. The three
// lists are told apart by their column-1 headers: "Workout Name" (download),
// "File" (upload), "Source" (sync).
static QTreeWidget* rideListWithHeader(QWidget* dialog, const QString& column1)
{
    for (QTreeWidget* tree : dialog->findChildren<QTreeWidget*>()) {
        if (tree->headerItem()->text(1) == column1)
            return tree;
    }
    return nullptr;
}

// The reason string the store-layer guard reports. Matched by substring, not
// equality, so the human-readable phrasing can be tuned without breaking the
// suite's ability to see that the BAIL is what ran.
static const char kGuardReasonFragment[] = "context destroyed";

class TestGarminConnectUncompressLifetime : public QObject
{
    Q_OBJECT

  private slots:

    // -----------------------------------------------------------------------
    // T-167 — REQ-023 core: a Context freed while a frame that will reach the
    // temp() deref is still live must not fault, bailing in the STORE's frame.
    //
    // RED (guard absent): the very first link of the chain, the `context`
    // field read at `context->athlete`, is a heap-use-after-free compiled into
    // CloudService.cpp — this target halts on the first ASan report, so the
    // abrupt end of the binary IS the red verdict.
    //
    // The premise assert matters as much as the outcome assert: a bail from
    // uncompressRide's FIRST gate (the compression-suffix check) would pass
    // trivially and prove nothing, so the slot asserts the errors do NOT
    // contain that gate's text — the frame really reached the deref's guard.
    // -----------------------------------------------------------------------
    void uncompressRide_afterContextTeardownMustBailInTheStore()
    {
        QTemporaryDir athleteRoot;
        Context* context = new Context(nullptr);
        Athlete* athlete = new Athlete(context, QDir(athleteRoot.path()));
        context->athlete = athlete;

        SpyStore store(context);

        // Content is irrelevant on the bail path: the deref happens before
        // any byte of it is looked at.
        QByteArray bytes("some bytes");
        QStringList errors;

        // The teardown under test, in the production order
        // (MainWindow::removeAthleteTab: `delete athlete; delete context;`).
        // The STORE survives both — which is the finding's whole premise:
        // CloudService holds `context` with nothing that re-validates it.
        delete athlete;
        delete context;

        RideFile* ride = store.uncompressRide(&bytes, QStringLiteral("2026_09_07_07_00_00.json"), errors);

        QVERIFY2(ride == nullptr, "a dead-Context decompress must return no ride");
        QVERIFY2(
            !errors.isEmpty(),
            "a dead-Context decompress must say why (B-R018-02(iii): errors are the only channel auto-download has)");
        bool sawGuardReason = false;
        bool sawSuffixGate = false;
        for (const QString& e : errors) {
            if (e.contains(QLatin1String(kGuardReasonFragment)))
                sawGuardReason = true;
            if (e.contains(QLatin1String("expected compressed")))
                sawSuffixGate = true;
        }
        QVERIFY2(sawGuardReason, qPrintable(QStringLiteral("the store's own guard must report the teardown, got: %1")
                                                .arg(errors.join(QStringLiteral("; ")))));
        QVERIFY2(!sawSuffixGate, "premise broken: the name was rejected by the compression gate, so the frame never "
                                 "reached the temp() deref's guard");
    }

    // -----------------------------------------------------------------------
    // T-168 — route selectivity: the SAME store-layer guard covers the census's
    // Route 1. The real CloudServiceSyncDialog is driven end to end through its
    // public slots (start, downloadClicked) and the completion is delivered
    // through the store's real readComplete signal — with the Context freed
    // between the dispatch and the delivery, a state none of the dialog's own
    // guards can even express (they check the dialog, the ticket, the
    // generations; not one of them asks whether the Context is alive).
    //
    // GREEN observables: no fault, row 0's status cell carries the STORE
    // guard's reason (the bail surfaced through the dialog route), and the
    // loop re-drove onto row 1 — the batch carried on with a dead Context
    // because the failure was reported like any other "could not process",
    // which is the auto-download discipline readComplete already follows.
    //
    // Why row 1 exists: without it, syncNext's completion tail dereferences
    // context->athlete->rideCache->save() — a DIALOG-layer context deref
    // outside this REQ's letter (see the file header). A second checked row
    // makes the re-drive dispatch row 1's read and return BEFORE that tail.
    // -----------------------------------------------------------------------
    void completedReadRouteWithFreedContextMustBailInTheStore()
    {
        QTemporaryDir athleteRoot;
        Context* context = new Context(nullptr);
        Athlete* athlete = new Athlete(context, QDir(athleteRoot.path()));
        athlete->cyclist = QStringLiteral("UncompressLifetimeRider");
        context->athlete = athlete;
        RideCache rideCache(context);
        athlete->rideCache = &rideCache;

        SpyStore* store = new SpyStore(context); // the dialog owns and deletes it
        const QString day = QDate::currentDate().toString(QStringLiteral("yyyy_MM_dd"));
        store->entryNames = QStringList()
                            << day + QStringLiteral("_10_00_00.fit") << day + QStringLiteral("_11_00_00.fit");

        CloudServiceSyncDialog dialog(context, store);
        QVERIFY(dialog.start());

        QTreeWidget* syncList = rideListWithHeader(&dialog, QStringLiteral("Source"));
        QVERIFY2(syncList != nullptr, "the dialog's sync list was not found in its widget tree");
        QCOMPARE(syncList->invisibleRootItem()->childCount(), 2);
        for (int i = 0; i < 2; ++i) {
            QCheckBox* check =
                qobject_cast<QCheckBox*>(syncList->itemWidget(syncList->invisibleRootItem()->child(i), 0));
            QVERIFY(check != nullptr);
            check->setChecked(true);
        }

        // Default tab is 2 (Synchronize): downloadClicked sets sync=true and
        // syncNext dispatches row 0's READ, arming the ticket keyed on the
        // preallocated buffer the spy recorded.
        dialog.downloadClicked();
        QCOMPARE(store->readBuffers.count(), 1);

        // The census's uncovered state: dialog and store alive, Context freed.
        // No dialog-layer guard fires between here and the store call — that
        // absence is precisely finding S-R021-05.
        delete athlete;
        delete context;

        // Deliver the completion through the REAL signal path (the dialog
        // connected store.readComplete to its completedRead in start()).
        store->notifyReadComplete(store->readBuffers.at(0), syncList->invisibleRootItem()->child(0)->text(1),
                                  QStringLiteral("Completed."));

        const QString status = syncList->invisibleRootItem()->child(0)->text(7);
        QVERIFY2(status.contains(QLatin1String(kGuardReasonFragment)),
                 qPrintable(QStringLiteral("the store guard's bail must surface on the row, got: %1").arg(status)));
        QVERIFY2(store->readBuffers.count() == 2,
                 "the batch must have re-driven onto row 1 after the bail, not stalled or faulted");

        // Row 1's buffer has no completion left to free it (the store is
        // deleted with the dialog); free it here so the slot ends as clean as
        // it started. The armed ticket holds only its address, never read.
        delete store->readBuffers.at(1);
    }

    // -----------------------------------------------------------------------
    // T-169 — positive control: with a LIVE Context the decompress path is
    // unchanged — the guard is transparent, the temp file is written, and the
    // REAL TcxRideFile parses the REAL sample ride out of a REAL qzip archive
    // built with the same ZipWriter compressRide uses.
    // -----------------------------------------------------------------------
    void uncompressRideLiveContextStillDecompressesAndParses()
    {
        QTemporaryDir athleteRoot;
        Context context(nullptr);
        Athlete athlete(&context, QDir(athleteRoot.path()));
        context.athlete = &athlete;

        SpyStore store(&context);
        store.downloadCompression = CloudService::zip;

        QFile sample(QStringLiteral(GC_TEST_RIDES_DIR) + QStringLiteral("/2008_12_28_08_13_27.tcx"));
        QVERIFY2(sample.open(QFile::ReadOnly), "sample TCX ride missing from test/rides");
        const QByteArray tcx = sample.readAll();
        sample.close();

        QTemporaryFile zipHolder;
        QVERIFY(zipHolder.open());
        const QString zipPath = zipHolder.fileName();
        zipHolder.close();
        {
            ZipWriter writer(zipPath);
            writer.addFile(QStringLiteral("2008_12_28_08_13_27.tcx"), tcx);
            writer.close();
        }
        QFile zf(zipPath);
        QVERIFY(zf.open(QFile::ReadOnly));
        QByteArray payload = zf.readAll();
        zf.close();

        QStringList errors;
        RideFile* ride = store.uncompressRide(&payload, QStringLiteral("2008_12_28_08_13_27.tcx.zip"), errors);

        QVERIFY2(ride != nullptr,
                 qPrintable(QStringLiteral("the live-Context path must still parse the ride, errors: %1")
                                .arg(errors.join(QStringLiteral("; ")))));
        QVERIFY2(ride->dataPoints().count() > 0, "the parsed ride must have samples");
        delete ride;
    }
};

QTEST_MAIN(TestGarminConnectUncompressLifetime)
#include "testGarminConnectUncompressLifetime.moc"
