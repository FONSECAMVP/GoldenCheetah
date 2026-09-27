/*
 * Copyright (c) 2026 GoldenCheetah Contributor
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the Free
 * Software Foundation; either version 2 of the License, or (at your option)
 * any later version.
 */

// T-258/T-259 — DEC-084: garminInstantFromString() is the ONE way any
// startTimeGMT comparison site turns a string into an instant. This is the
// primitive's own positive-case table (B-STAGE9-144: a refusal-only suite
// cannot tell a working parser from a dead one) plus the naive-spelling
// regression guard run under a non-UTC host TZ.

#include "GarminTime.h"

#include <QDateTime>
#include <QString>
#include <QTimeZone>
#include <QtTest/QtTest>

#include <cstdlib>

class TestGarminTime : public QObject
{
    Q_OBJECT

  private slots:

    // DEC-084's measured Qt 6.8.2 table: a zone-less spelling is DECLARED
    // UTC; a `Z`/offset spelling is CONVERTED. Each case asserted against its
    // absolute UTC instant (not just "isValid"), per B-STAGE9-144.
    void eachSpellingResolvesToItsMeasuredInstant()
    {
        const QDateTime naive = garminInstantFromString(QStringLiteral("2026-09-26 09:54:01"));
        QVERIFY(naive.isValid());
        QCOMPARE(naive, QDateTime(QDate(2026, 9, 26), QTime(9, 54, 1), QTimeZone::UTC));

        const QDateTime zulu = garminInstantFromString(QStringLiteral("2026-09-01T00:00:00Z"));
        QVERIFY(zulu.isValid());
        QCOMPARE(zulu, QDateTime(QDate(2026, 9, 1), QTime(0, 0, 0), QTimeZone::UTC));

        const QDateTime offset = garminInstantFromString(QStringLiteral("2026-09-01T02:00:00+02:00"));
        QVERIFY(offset.isValid());
        QCOMPARE(offset, QDateTime(QDate(2026, 9, 1), QTime(0, 0, 0), QTimeZone::UTC));

        const QDateTime isoNaive = garminInstantFromString(QStringLiteral("2026-09-01T02:00:00"));
        QVERIFY(isoNaive.isValid());
        QCOMPARE(isoNaive, QDateTime(QDate(2026, 9, 1), QTime(2, 0, 0), QTimeZone::UTC));
    }

    void unparseableInputReturnsInvalid()
    {
        QVERIFY(!garminInstantFromString(QStringLiteral("not-a-timestamp")).isValid());
        QVERIFY(!garminInstantFromString(QString()).isValid());
    }

    // T-259 — the naive spelling (the ONLY one live sidecars carry) must
    // resolve to the same absolute instant regardless of the host's TZ. Runs
    // under Australia/Hobart (UTC+10, DEC-084's own measurement zone): a
    // regression to unconditional toUTC() would shift this naive spelling by
    // -10h and fail the QCOMPARE below, whereas under a host TZ of UTC that
    // same mutant would pass by coincidence.
    void naiveSpellingIsTZIndependent()
    {
        qputenv("TZ", "Australia/Hobart");
        tzset();

        const QDateTime dt = garminInstantFromString(QStringLiteral("2026-09-26 09:54:01"));
        QVERIFY(dt.isValid());
        QCOMPARE(dt, QDateTime(QDate(2026, 9, 26), QTime(9, 54, 1), QTimeZone::UTC));

        // A cursor/entry pair one second apart must still order the same way
        // under this TZ as it would under UTC.
        const QDateTime earlier = garminInstantFromString(QStringLiteral("2026-09-26 09:54:00"));
        QVERIFY(earlier.isValid());
        QVERIFY(earlier < dt);

        qunsetenv("TZ");
        tzset();
    }
};

QTEST_APPLESS_MAIN(TestGarminTime)
#include "testGarminTime.moc"
