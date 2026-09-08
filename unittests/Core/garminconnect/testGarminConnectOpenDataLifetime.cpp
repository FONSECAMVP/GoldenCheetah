/*
 * Copyright (c) 2026 GoldenCheetah Contributor
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the License as published by the Free Software Foundation.
 */

// REQ-022 / T-164 — OpenData is a QThread (OpenData.h:35) holding a
// Context* it dereferences throughout run() (:224/:233/:238/:243/:307/:312/
// :344-346) across three blocking QEventLoop suspensions (:130/:184/:322),
// while the GUI thread stays interactive. A tab close during an OpenData
// upload frees that Context on the GUI thread (MainWindow::removeAthleteTab,
// MainWindow.cpp:2183-2185) — a cross-thread UAF no dialog-layer guard can
// reach (finding S-R021-04).
//
// The REAL src/Cloud/OpenData.cpp is compiled into this executable via the
// force-included stubs/OpenDataLifetimeStubPreamble.h (sibling of the
// wizard targets' preambles; the Context stand-in carries the
// athleteClose(QString, Context*) signal the production code connects to,
// mirroring real Context.h:288).
//
// NETWORK CONTROL: run() creates its own QNetworkAccessManager, so the only
// seam is QNetworkProxy::setApplicationProxy — pointed at a FakeProxyServer
// in this file that speaks just enough forward-proxy HTTP. It serves a valid
// one-server list, answers the STEP TWO ping and the STEP FOUR post, and
// HOLDS the initial server-list request until the test releases it: that is
// what parks the worker inside the first loop.exec() (:130) deterministically,
// which is the "mid-flight" premise of the acceptance criterion.
//
// VERDICTS ARE ASAN VERDICTS: the RED half of the teardown slot is a
// heap-use-after-free (the freed Context read at :224's
// context->athlete->rideCache chain), not a QCOMPARE.

#if defined(__SANITIZE_ADDRESS__) || (defined(__has_feature) && __has_feature(address_sanitizer))
// instrumented — ok
#else
#    error "testGarminConnectOpenDataLifetime (T-164..T-166) is a lifetime test and requires AddressSanitizer"
#endif

#include "OpenData.h"

#include <QApplication>
#include <QHostAddress>
#include <QNetworkProxy>
#include <QTcpServer>
#include <QTcpSocket>
#include <QtTest/QtTest>

// A forward proxy just complete enough for OpenData's three blocking
// requests. Holds the server-list GET until releaseHeld() so a teardown can
// land while the worker is suspended inside loop.exec() (OpenData.cpp:130).
class FakeProxyServer : public QObject
{
    Q_OBJECT

  public:
    FakeProxyServer(QObject* parent = nullptr) : QObject(parent)
    {
        connect(&server_, &QTcpServer::newConnection, this, &FakeProxyServer::onNewConnection);
        server_.listen(QHostAddress::LocalHost);
    }

    quint16 port() const { return server_.serverPort(); }
    int connections() const { return requests_.size(); }
    QString methodAt(int i) const { return requests_[i]->method; }
    QString uriAt(int i) const { return requests_[i]->uri; }

    // Writes the held server-list response. Only the server-list URI is
    // ever held; the ping and the post are answered as they arrive.
    void releaseHeld()
    {
        foreach (Request* r, requests_) {
            if (r->held && headersComplete(r)) {
                respond(r);
            }
        }
    }

  private slots:

    void onNewConnection()
    {
        while (server_.hasPendingConnections()) {
            QTcpSocket* socket = server_.nextPendingConnection();
            Request* r = new Request;
            r->socket = socket;
            requests_.append(r);
            connect(socket, &QTcpSocket::readyRead, this, [this, r]() { this->onReadyRead(r); });
        }
    }

  private:
    struct Request
    {
        QTcpSocket* socket = nullptr;
        QByteArray buffer;
        QString method;
        QString uri;
        int contentLength = 0;
        bool headersParsed = false;
        bool responded = false;
        bool held = false;
    };

    static bool headersComplete(Request* r) { return r->headersParsed; }

    bool bodyComplete(Request* r) const
    {
        if (r->method != QStringLiteral("POST"))
            return true;
        int headerEnd = r->buffer.indexOf("\r\n\r\n");
        return headerEnd >= 0 && r->buffer.size() >= headerEnd + 4 + r->contentLength;
    }

    void onReadyRead(Request* r)
    {
        r->buffer += r->socket->readAll();
        if (!r->headersParsed) {
            int headerEnd = r->buffer.indexOf("\r\n\r\n");
            if (headerEnd < 0)
                return;
            QByteArray headerBlock = r->buffer.left(headerEnd);
            QList<QByteArray> lines = headerBlock.split('\n');
            QList<QByteArray> parts = lines[0].simplified().split(' ');
            if (parts.size() >= 2) {
                r->method = QString::fromLatin1(parts[0]);
                r->uri = QString::fromLatin1(parts[1]);
            }
            foreach (QByteArray line, lines) {
                if (QString::fromLatin1(line).startsWith(QStringLiteral("Content-Length:"), Qt::CaseInsensitive))
                    r->contentLength = line.split(':').value(1).simplified().toInt();
            }
            r->headersParsed = true;
            // The server-list fetch is the suspension the tests need to
            // control; everything else is answered immediately.
            r->held = r->uri.contains(QStringLiteral("opendata.json"));
        }
        if (headersComplete(r) && bodyComplete(r) && !r->responded && !r->held)
            respond(r);
    }

    void respond(Request* r)
    {
        r->responded = true;
        QByteArray body;
        if (r->uri.contains(QStringLiteral("opendata.json"))) {
            // one server that answers pings: run()'s STEP TWO will pick it
            body = QByteArray("{\"SERVERS\":[{\"url\":\"http://opendata-test.invalid/\"}]}");
        } else {
            body = QByteArray("ok");
        }
        QByteArray response = QByteArray("HTTP/1.1 200 OK\r\n") + "Content-Type: application/json\r\n" +
                              "Content-Length: " + QByteArray::number(body.size()) + "\r\n" +
                              "Connection: close\r\n\r\n" + body;
        r->socket->write(response);
        r->socket->flush();
        r->socket->disconnectFromHost();
    }

    QTcpServer server_;
    QList<Request*> requests_;
};

// Records the cross-thread progress emissions (queued from the worker) so
// "the upload finished" is an observation, not an inference.
class ProgressRecorder : public QObject
{
    Q_OBJECT

  public slots:

    void onProgress(int n, int, QString)
    {
        if (n == 0)
            sawDone = true;
    }

  public:
    bool sawDone = false;
};

class TestGarminConnectOpenDataLifetime : public QObject
{
    Q_OBJECT

  private slots:

    void initTestCase()
    {
        proxy_ = new FakeProxyServer(this);
        QNetworkProxy::setApplicationProxy(QNetworkProxy(QNetworkProxy::HttpProxy, "127.0.0.1", proxy_->port()));
    }

    void cleanupTestCase() { QNetworkProxy::setApplicationProxy(QNetworkProxy::NoProxy); }

    // T-164 — acceptance criterion, executed: the Context is freed on the
    // GUI thread while the worker is suspended inside run()'s first
    // loop.exec(); when the held reply releases it, the worker must bail
    // without dereferencing the freed Context. RED = the ASan
    // heap-use-after-free on the :224 chain; GREEN = resume-point bail.
    void athleteCloseWhileWorkerSuspendedMustNotDereferenceContext()
    {
        int savesBefore = g_ridesSaveCalls;

        Context* context = new Context();
        context->athlete = new Athlete;
        context->athlete->rideCache = new RideCache;

        OpenData* od = new OpenData(context);
        od->postData();

        // PREMISE: the suspension really landed. The proxy has exactly
        // one connection (the STEP ONE server-list GET issued one line
        // above the first loop.exec()) and nothing has been answered, so
        // the worker's only possible location is parked inside that
        // loop.exec() waiting for the reply only we can send.
        QTRY_COMPARE(proxy_->connections(), 1);
        QVERIFY(od->isRunning());
        QTest::qWait(100); // let the worker pass get()->connect()->exec()

        // Teardown on the GUI thread, mirroring the real sequence:
        // Athlete::close() opens with notifyAthleteClose, then
        // MainWindow::removeAthleteTab frees athlete and context.
        context->notifyAthleteClose(QStringLiteral("tester"), context);
        delete context->athlete->rideCache;
        delete context->athlete;
        delete context;

        // Release the suspended worker.
        proxy_->releaseHeld();

        // GREEN shape: the worker bails at the resume point — no deref,
        // no STEP TWO ping, thread finishes promptly. (In RED the
        // process has already died of the freed-Context read at :224.)
        QTRY_VERIFY(od->isFinished());
        QCOMPARE(g_ridesSaveCalls, savesBefore); // the :224 chain never ran
        QCOMPARE(proxy_->connections(), 1);      // STEP TWO never started

        delete od;
    }

    // T-165 — stop-flag semantics: requestStop() ahead of the thread
    // start (the DEC-043 "safe before start()" property) must make run()
    // return at its entry guard without issuing any request at all.
    void stopRequestedBeforeThreadStartSkipsNetworkEntirely()
    {
        int baseline = proxy_->connections();
        int savesBefore = g_ridesSaveCalls;

        Context* context = new Context();
        context->athlete = new Athlete;
        context->athlete->rideCache = new RideCache;

        OpenData* od = new OpenData(context);
        od->requestStop(); // teardown raced ahead of the thread start
        od->postData();

        QTRY_VERIFY(od->isFinished());
        QCOMPARE(proxy_->connections(), baseline); // network never touched
        QCOMPARE(g_ridesSaveCalls, savesBefore);   // no context deref either

        delete od;
        delete context->athlete->rideCache;
        delete context->athlete;
        delete context;
    }

    // T-166 — positive control + selectivity: with the Context alive and
    // the user's grant in force, the upload must still run all five steps
    // and POST — INCLUDING while a DIFFERENT athlete's tab closes
    // mid-flight (multi-tab selectivity of the stop flag).
    void grantedUploadCompletesAndPostsDespiteUnrelatedAthleteClose()
    {
        int baseline = proxy_->connections();
        int setCValueBefore = g_setCValueCalls;

        Context* context = new Context();
        context->athlete = new Athlete;
        context->athlete->rideCache = new RideCache;
        Context* unrelated = new Context();

        OpenData* od = new OpenData(context);
        ProgressRecorder rec;
        connect(od, SIGNAL(progress(int, int, QString)), &rec, SLOT(onProgress(int, int, QString)));
        od->postData();

        // STEP ONE in flight (held by the proxy), then another tab closes.
        QTRY_COMPARE(proxy_->connections(), baseline + 1);
        unrelated->notifyAthleteClose(QStringLiteral("other"), unrelated);
        proxy_->releaseHeld(); // the real server would answer now

        // STEP TWO ping and STEP FOUR post both went out and were served.
        QTRY_COMPARE(proxy_->connections(), baseline + 3);
        QTRY_VERIFY(rec.sawDone); // progress(0,...) = finished
        QTRY_VERIFY(od->isFinished());
        QCOMPARE(proxy_->methodAt(baseline + 2), QStringLiteral("POST"));

        // the STEP FIVE tail (:344-346) wrote its three lastpost keys
        // through the still-live context.
        QCOMPARE(g_setCValueCalls - setCValueBefore, 3);

        delete unrelated;
        delete od;
        delete context->athlete->rideCache;
        delete context->athlete;
        delete context;
    }

  private:
    FakeProxyServer* proxy_ = nullptr;
};

QTEST_MAIN(TestGarminConnectOpenDataLifetime)
#include "testGarminConnectOpenDataLifetime.moc"
