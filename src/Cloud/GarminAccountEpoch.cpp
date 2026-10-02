/*
 * Copyright (c) 2026 GoldenCheetah Contributor
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the Free
 * Software Foundation; either version 2 of the License, or (at your option)
 * any later version.
 */

#include "GarminAccountEpoch.h"

#include <QMutex>
#include <QMutexLocker>

QHash<QString, quint64> GarminAccountEpoch::s_epoch;

namespace {
// The map is process-global mutable state. Every production caller today lives on
// the GUI thread (readdir/readFile bridge async->sync on the caller thread, and
// Disconnect is a dialog button), so this lock is uncontended in practice — but
// the map outlives every individual GarminConnect and the class is a seam, so
// guarding it makes the container's own invariants hold regardless of who calls.
//
// It is a lock over the MAP, not over the check-then-act in GarminConnect: a bump
// that lands between a caller's current() and its use of the result is exactly the
// mid-flight race clause (c)'s post-download recheck exists to catch, and is
// always resolved fail-closed (the later recheck sees the new value).
//
// Function-local static so there is no static-initialisation-order dependency
// with GarminConnect.cpp's static service registration.
QMutex& epochMutex()
{
    static QMutex mutex;
    return mutex;
}
} // namespace

quint64 GarminAccountEpoch::current(const QString& configDir)
{
    QMutexLocker locker(&epochMutex());
    return s_epoch.value(configDir, 0);
}

quint64 GarminAccountEpoch::bump(const QString& configDir)
{
    QMutexLocker locker(&epochMutex());
    const quint64 next = s_epoch.value(configDir, 0) + 1;
    s_epoch.insert(configDir, next);
    return next;
}
