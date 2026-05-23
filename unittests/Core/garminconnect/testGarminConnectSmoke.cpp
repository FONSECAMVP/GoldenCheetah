/*
 * Copyright (c) 2026 GoldenCheetah Contributor
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the Free
 * Software Foundation; either version 2 of the License, or (at your option)
 * any later version.
 */

// Phase 2.1 bootstrap smoke test for DEC-008 (testing toolchain).
// Proves QTest + CTest builds and runs under GC_WANT_GARMINCONNECT.
// Phase 2.2 replaces this with real tests for REQ-001..015.

#include <QString>
#include <QTest>

class TestGarminConnectSmoke : public QObject
{
    Q_OBJECT

  private slots:
    void toolchainAlive() { QCOMPARE(QString("garmin").length(), 6); }
};

QTEST_APPLESS_MAIN(TestGarminConnectSmoke)
#include "testGarminConnectSmoke.moc"
