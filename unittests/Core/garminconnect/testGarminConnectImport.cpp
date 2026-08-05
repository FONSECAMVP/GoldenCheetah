/*
 * Copyright (c) 2026 GoldenCheetah Contributor
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

// TEST-067 — REQ-018: a downloaded Garmin activity must actually IMPORT.
//
// Acceptance criterion (verbatim):
//   "A successfully downloaded Garmin activity is accepted by
//    CloudService::uncompressRide and yields a parsed RideFile, in BOTH
//    consumers (CloudServiceSyncDialog::completedRead and
//    CloudServiceAutoDownload::readComplete). The acceptance test must CROSS
//    the uncompressRide boundary (bytes -> completion -> uncompressRide ->
//    non-NULL RideFile), not stop at readFile's staging: the defect survived a
//    22/22-green suite precisely because no test crossed it, so a test that only
//    asserts staging does not satisfy this REQ. Covers both the FIT and the TCX
//    fallback names."
//
// WHY THIS TARGET IS DIFFERENT FROM EVERY OTHER garmin test
// ---------------------------------------------------------
// Every other GarminConnect test force-includes stubs/ReadFileStubPreamble.h,
// which REPLACES CloudService with a lightweight stand-in that has no
// uncompressRide at all. That is exactly why the REQ-018 defect shipped behind a
// green suite: the boundary that rejects the download was never compiled into a
// test. So this target compiles the REAL src/Cloud/CloudService.cpp (hence the
// REAL uncompressRide, its REAL downloadCompression guard and the REAL
// RideFileFactory dispatch) against the REAL GC headers, together with the REAL
// src/FileIO/RideFile.cpp + FitRideFile.cpp + TcxRideFile.cpp + TcxParser.cpp so
// a REAL ride parser produces the RideFile.
//
// The application layers that CloudService.cpp merely references but that this
// contract does not exercise (RideCache/RideItem/RideMetric/MainWindow/
// DataProcessor/Colors/GlobalContext/...) are satisfied by stubs/ImportSeamStubs.cpp
// — link-level stand-ins behind the REAL headers. Nothing on the path under test
// (readFile -> readComplete -> uncompressRide -> RideFileFactory -> Fit/Tcx
// reader) is stubbed.
//
// Python-free (the injected IGarminDownloadClient replaces the worker host and
// PyEmbeddedAdapter is a link stub that is never constructed), so this stays on
// the `garmin-fast` label.
//
// The payloads are REAL activity files from test/rides — a synthetic 12-byte
// ".FIT" header would satisfy readFile's sniff but could never parse, and
// "yields a parsed RideFile" is the whole point of this REQ.

#include "Athlete.h"
#include "CloudService.h"
#include "Context.h"
#include "GarminConnect.h"
#include "GarminTokenStore.h"
#include "IGarminDownloadClient.h"
#include "RideFile.h"
#include "zipwriter.h"

#include <QByteArray>
#include <QDir>
#include <QFile>
#include <QHash>
#include <QMetaObject>
#include <QString>
#include <QStringList>
#include <QTemporaryDir>
#include <QTemporaryFile>
#include <QUuid>
#include <QtTest/QtTest>

// ---------------------------------------------------------------------------
// FakeDownloadClient — Python-free IGarminDownloadClient (DES-004 seam), same
// shape as testGarminConnectReadFile's: scripts a response per requested fmt and
// posts it queued so it arrives while GarminConnect's blocking QEventLoop runs.
// ---------------------------------------------------------------------------
class FakeDownloadClient : public IGarminDownloadClient
{
    Q_OBJECT
  public:
    struct Resp
    {
        bool ok = false;
        QByteArray bytes;
        GarminDownloadFailure::Kind kind = GarminDownloadFailure::Unknown;
    };

    QHash<QString, Resp> responses; // fmt -> scripted response
    QStringList downloadFmts;       // recorded fmt-call sequence

    void restoreSession(const QString&, QUuid id) override
    {
        QMetaObject::invokeMethod(this, [this, id]() { emit sessionRestored(id); }, Qt::QueuedConnection);
    }

    void listActivities(const QString&, QUuid) override {}

    void downloadActivity(const QString&, const QString& fmt, QUuid id) override
    {
        downloadFmts << fmt;
        const Resp r = responses.value(fmt);
        QMetaObject::invokeMethod(
            this,
            [this, id, r]() {
                if (r.ok) {
                    emit downloaded(id, r.bytes);
                } else {
                    GarminDownloadFailure e;
                    e.kind = r.kind;
                    e.rawMessage = QStringLiteral("scripted failure");
                    emit downloadFailed(id, e);
                }
            },
            Qt::QueuedConnection);
    }
};

namespace {

const QString kUid = QStringLiteral("123456789");
const QByteArray kBlob = QByteArray("{\"oauth1\":\"OA1-secret\",\"oauth2\":\"OA2.refresh\"}");

QByteArray readFixture(const QString& relative)
{
    QFile f(QStringLiteral(GC_TEST_RIDES_DIR) + QStringLiteral("/") + relative);
    if (!f.open(QIODevice::ReadOnly))
        return QByteArray();
    const QByteArray b = f.readAll();
    f.close();
    return b;
}

// Garmin's ORIGINAL download arrives ZIP-wrapped (DEC-016); build a real ZIP with
// the same writer production unwraps with.
QByteArray makeZip(const QString& entryName, const QByteArray& content)
{
    QTemporaryFile tf;
    tf.open();
    const QString path = tf.fileName();
    tf.close();
    {
        ZipWriter w(path);
        w.addFile(entryName, content);
        w.close();
    }
    QFile f(path);
    f.open(QIODevice::ReadOnly);
    const QByteArray z = f.readAll();
    f.close();
    return z;
}

} // namespace

class TestGarminConnectImport : public QObject
{
    Q_OBJECT

  private:
    // A minimal athlete the way uncompressRide needs it: it writes the staged
    // bytes to context->athlete->home->temp() before handing the file to the
    // ride-file reader.
    QTemporaryDir athleteRoot;
    Context* context = nullptr;
    Athlete* athlete = nullptr;

    // Drives ONE download all the way across the boundary and returns the
    // RideFile (or NULL). `errors` and `stagedName` are reported out so a failing
    // run says WHY it was refused rather than just "was null".
    RideFile* downloadAndImport(const QString& activityId, const FakeDownloadClient::Resp& original,
                                const FakeDownloadClient::Resp& tcx, QString* stagedName, QStringList* errors)
    {
        FakeDownloadClient fake;
        fake.responses[QStringLiteral("ORIGINAL")] = original;
        fake.responses[QStringLiteral("TCX")] = tcx;

        QTemporaryDir cfg;
        if (!cfg.isValid()) {
            *errors << QStringLiteral("harness: could not create a config dir");
            return nullptr;
        }
        // DEC-garmin-020: readFile fails closed unless the account is connected.
        if (!GarminTokenStore::persistConnectSuccess(cfg.path(), kUid, kBlob)) {
            *errors << QStringLiteral("harness: could not connect the fixture account");
            return nullptr;
        }

        GarminConnect gc(context, &fake, cfg.path());

        // The REAL completion both consumers receive. CloudServiceSyncDialog::
        // completedRead and CloudServiceAutoDownload::readComplete are both
        // connected to this signal and both do the same first thing with its
        // payload: store->uncompressRide(data, name, errors).
        QByteArray* completionData = nullptr;
        QString completionName;
        connect(&gc, &CloudService::readComplete, this, [&](QByteArray* d, QString n, QString) {
            completionData = d;
            completionName = n;
        });

        QByteArray data;
        if (!gc.readFile(&data, QStringLiteral("ignored-name"), activityId)) {
            *errors << QStringLiteral("harness: readFile reported the download failed");
            return nullptr;
        }

        // The completion is a QUEUED self-post (B-R007-01) — pump until it lands.
        for (int i = 0; i < 500 && completionData == nullptr; ++i)
            QCoreApplication::processEvents(QEventLoop::AllEvents, 10);
        if (completionData == nullptr) {
            *errors << QStringLiteral("harness: no readComplete was ever posted");
            return nullptr;
        }
        if (stagedName)
            *stagedName = completionName;

        // ***** THE BOUNDARY THIS REQ IS ABOUT *****
        return static_cast<CloudService&>(gc).uncompressRide(completionData, completionName, *errors);
    }

  private slots:

    void initTestCase()
    {
        QVERIFY(athleteRoot.isValid());
        athlete = new Athlete(nullptr, QDir(athleteRoot.path()));
        context = new Context(nullptr);
        context->athlete = athlete;
        QVERIFY2(athlete->home->temp().exists(), "fixture: the athlete temp dir must exist");
    }

    void cleanupTestCase()
    {
        delete context;
        context = nullptr;
        delete athlete;
        athlete = nullptr;
    }

    // TEST-067a — the FIT path. A real Garmin ORIGINAL download (ZIP-wrapped FIT)
    // is staged by readFile as garmin-<id>.fit and must be ACCEPTED by
    // uncompressRide and parsed into a RideFile.
    void fitDownloadIsAcceptedByUncompressRideAndYieldsARideFile()
    {
        const QByteArray fit = readFixture(QStringLiteral("2013-04-11-17-32-50.fit"));
        QVERIFY2(!fit.isEmpty(), "fixture: the FIT sample must be readable");

        QString staged;
        QStringList errors;
        RideFile* ride = downloadAndImport(QStringLiteral("123"), {true, makeZip(QStringLiteral("123.fit"), fit), {}},
                                           {false, {}, GarminDownloadFailure::Unknown}, &staged, &errors);

        QCOMPARE(staged, QStringLiteral("garmin-123.fit"));
        QVERIFY2(!errors.contains(QStringLiteral("expected compressed activity file.")),
                 qPrintable(QStringLiteral("uncompressRide REFUSED the download: ") + errors.join(" | ")));
        QVERIFY2(ride != nullptr, qPrintable(QStringLiteral("no RideFile was produced: ") + errors.join(" | ")));
        QVERIFY2(ride->dataPoints().count() > 0, "the parsed RideFile must carry samples");
        delete ride;
    }

    // TEST-067b — the DEC-016 TCX fallback path. ORIGINAL fails, readFile retries
    // as TCX and stages garmin-<id>.tcx; that name must be accepted too.
    void tcxFallbackDownloadIsAcceptedByUncompressRideAndYieldsARideFile()
    {
        const QByteArray tcx = readFixture(QStringLiteral("2010-04-05-21-45-04.tcx"));
        QVERIFY2(!tcx.isEmpty(), "fixture: the TCX sample must be readable");

        QString staged;
        QStringList errors;
        RideFile* ride = downloadAndImport(QStringLiteral("77"), {false, {}, GarminDownloadFailure::Network},
                                           {true, tcx, {}}, &staged, &errors);

        QCOMPARE(staged, QStringLiteral("garmin-77.tcx"));
        QVERIFY2(!errors.contains(QStringLiteral("expected compressed activity file.")),
                 qPrintable(QStringLiteral("uncompressRide REFUSED the download: ") + errors.join(" | ")));
        QVERIFY2(ride != nullptr, qPrintable(QStringLiteral("no RideFile was produced: ") + errors.join(" | ")));
        QVERIFY2(ride->dataPoints().count() > 0, "the parsed RideFile must carry samples");
        delete ride;
    }
};

QTEST_MAIN(TestGarminConnectImport)
#include "testGarminConnectImport.moc"
