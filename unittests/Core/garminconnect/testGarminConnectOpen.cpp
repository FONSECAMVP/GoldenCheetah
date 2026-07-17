/*
 * Copyright (c) 2026 GoldenCheetah Contributor
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the Free
 * Software Foundation; either version 2 of the License, or (at your option)
 * any later version.
 */

// TEST garmin:T-019 — REQ-007 closure (Slice 2/3): GarminConnect::open()/close()
// worker-in-CloudService lifecycle.
//
//   open() resolves the athlete config dir, loads the stored tokens via
//   GarminTokenStore::loadChecked (REQ-006), and restores the session through
//   the host from the returned blob. On a permission-refused LoadResult it
//   returns false with a labelled error and does NOT attempt a download
//   (REQ-006 forces a fresh SSO — never a download with an unsafe session).
//   close() performs bounded teardown; tokens persist on disk.
//
// Python-free: a fake IGarminDownloadClient (DES-004 seam) records the restore
// blob + any download attempt, and stubs/ReadFileStubPreamble.h stubs
// CloudService + replaces PyEmbeddedAdapter with a Python-free fake →
// `garmin-fast` label. GarminTokenStore + AtomicFile are the real (pure-Qt)
// units, driven against a real temp config dir with controlled file modes.

#include "GarminConnect.h"
#include "GarminTokenStore.h"
#include "IGarminDownloadClient.h"

#include <QByteArray>
#include <QDir>
#include <QFile>
#include <QMetaObject>
#include <QString>
#include <QStringList>
#include <QTemporaryDir>
#include <QUuid>
#include <QtTest/QtTest>

#ifdef Q_OS_UNIX
#    include <sys/stat.h>
#endif

// ---------------------------------------------------------------------------
// FakeRestoreClient — records the restore blob it received and whether any
// download was attempted, then emits sessionRestored / restoreFailed via a
// queued invocation so it arrives while open()'s blocking QEventLoop runs.
// ---------------------------------------------------------------------------
class FakeRestoreClient : public IGarminDownloadClient
{
    Q_OBJECT
  public:
    bool restoreOk = true;
    GarminRestoreFailure::Kind restoreKind = GarminRestoreFailure::Unknown;
    QString lastRestoreBlob;
    int restoreCalls = 0;
    int downloadCalls = 0;

    void restoreSession(const QString& blob, QUuid id) override
    {
        lastRestoreBlob = blob;
        ++restoreCalls;
        const bool ok = restoreOk;
        const GarminRestoreFailure::Kind kind = restoreKind;
        QMetaObject::invokeMethod(
            this,
            [this, id, ok, kind]() {
                if (ok) {
                    emit sessionRestored(id);
                } else {
                    GarminRestoreFailure e;
                    e.kind = kind;
                    emit restoreFailed(id, e);
                }
            },
            Qt::QueuedConnection);
    }

    void downloadActivity(const QString&, const QString& fmt, QUuid id) override
    {
        ++downloadCalls; // open() must NEVER reach here on the failure paths
        QMetaObject::invokeMethod(
            this, [this, id]() { emit downloadFailed(id, GarminDownloadFailure{}); }, Qt::QueuedConnection);
        Q_UNUSED(fmt);
    }
};

namespace {
const QByteArray kBlob = QByteArray("{\"oauth1\":\"OA1\",\"oauth2\":\"OA2.refresh\"}");

// Write <dir>/garminconnect/tokens.json with `bytes` and set its POSIX mode.
QString writeTokenFile(const QString& configDir, const QByteArray& bytes, uint mode)
{
    QDir().mkpath(configDir + QStringLiteral("/garminconnect"));
    const QString path = configDir + QStringLiteral("/garminconnect/tokens.json");
    QFile f(path);
    f.open(QIODevice::WriteOnly | QIODevice::Truncate);
    f.write(bytes);
    f.close();
#ifdef Q_OS_UNIX
    ::chmod(path.toLocal8Bit().constData(), mode);
#else
    Q_UNUSED(mode);
#endif
    return path;
}
} // namespace

class TestGarminConnectOpen : public QObject
{
    Q_OBJECT

  private slots:

    // open() with a conforming 0600 token file: loadChecked returns Ok, and the
    // session is restored from THAT blob (verbatim) through the injected client.
    void openRestoresSessionFromLoadedBlob()
    {
        QTemporaryDir tmp;
        QVERIFY(tmp.isValid());
        writeTokenFile(tmp.path(), kBlob, 0600);

        FakeRestoreClient fake;
        fake.restoreOk = true;
        GarminConnect gc(nullptr, &fake, tmp.path());

        QStringList errors;
        const bool ok = gc.open(errors);

        QVERIFY2(ok, "open() must succeed when a conforming token file restores");
        QCOMPARE(errors.size(), 0);
        QCOMPARE(fake.restoreCalls, 1);
        // Restored from the blob loadChecked returned — VERBATIM.
        QCOMPARE(fake.lastRestoreBlob, QString::fromUtf8(kBlob));
    }

    // REQ-006: a token file WIDER than owner-only 0600 is refused. open() must
    // return false with a labelled error and must NOT attempt any restore or
    // download (forces a fresh SSO instead of using an unsafe session).
    void openRefusesOnPermissionRejectedAndDoesNotDownload()
    {
#ifndef Q_OS_UNIX
        QSKIP("POSIX permission-refusal path is UNIX-only");
#endif
        QTemporaryDir tmp;
        QVERIFY(tmp.isValid());
        // 0644 — group/other-readable → loadChecked must reject.
        writeTokenFile(tmp.path(), kBlob, 0644);

        FakeRestoreClient fake;
        GarminConnect gc(nullptr, &fake, tmp.path());

        QStringList errors;
        const bool ok = gc.open(errors);

        QVERIFY2(!ok, "open() must fail on a permission-rejected token file");
        QVERIFY2(!errors.isEmpty(), "a labelled error must be pushed on rejection");
        QVERIFY2(errors.first().contains(QStringLiteral("Garmin"), Qt::CaseInsensitive),
                 "the error must be labelled as a Garmin Connect problem");
        QCOMPARE(fake.restoreCalls, 0);  // did NOT attempt to restore an unsafe session
        QCOMPARE(fake.downloadCalls, 0); // and did NOT attempt any download
    }

    // No token file at all → NotFound: open() returns false with a labelled
    // error and attempts no download.
    void openFailsWhenNoStoredSession()
    {
        QTemporaryDir tmp;
        QVERIFY(tmp.isValid());
        // deliberately do not write any token file

        FakeRestoreClient fake;
        GarminConnect gc(nullptr, &fake, tmp.path());

        QStringList errors;
        const bool ok = gc.open(errors);

        QVERIFY2(!ok, "open() must fail when there is no stored session");
        QVERIFY2(!errors.isEmpty(), "a labelled error must be pushed when no session exists");
        QCOMPARE(fake.restoreCalls, 0);
        QCOMPARE(fake.downloadCalls, 0);
    }

    // A restore that fails (e.g. session expired) → open() returns false with a
    // labelled error.
    void openFailsWhenRestoreFails()
    {
        QTemporaryDir tmp;
        QVERIFY(tmp.isValid());
        writeTokenFile(tmp.path(), kBlob, 0600);

        FakeRestoreClient fake;
        fake.restoreOk = false;
        fake.restoreKind = GarminRestoreFailure::SessionExpired;
        GarminConnect gc(nullptr, &fake, tmp.path());

        QStringList errors;
        const bool ok = gc.open(errors);

        QVERIFY2(!ok, "open() must fail when the stored session cannot be restored");
        QVERIFY2(!errors.isEmpty(), "a labelled error must be pushed when restore fails");
        QCOMPARE(fake.restoreCalls, 1);
    }

    // close() performs bounded teardown and returns promptly. With an injected
    // client (not owned) there is no host thread to stop, so this must return
    // true well within any watchdog.
    void closeTearsDownBounded()
    {
        QTemporaryDir tmp;
        QVERIFY(tmp.isValid());
        writeTokenFile(tmp.path(), kBlob, 0600);

        FakeRestoreClient fake;
        GarminConnect gc(nullptr, &fake, tmp.path());
        QStringList errors;
        QVERIFY(gc.open(errors));

        QElapsedTimer t;
        t.start();
        const bool closed = gc.close();
        QVERIFY2(closed, "close() must return true");
        QVERIFY2(t.elapsed() < 2000, "close() teardown must be bounded (never hang)");
    }
};

QTEST_MAIN(TestGarminConnectOpen)
#include "testGarminConnectOpen.moc"
