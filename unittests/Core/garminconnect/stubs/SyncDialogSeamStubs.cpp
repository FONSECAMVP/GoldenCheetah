/*
 * Copyright (c) 2026 GoldenCheetah Contributor
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the Free
 * Software Foundation; either version 2 of the License, or (at your option)
 * any later version.
 */

// Link-level stand-ins for testGarminConnectReadFailedConsumer (TEST-069 /
// DEC-garmin-023) ONLY. STRICTLY ADDITIVE to stubs/ImportSeamStubs.cpp, which
// that target also links: this file defines the handful of symbols the SYNC
// DIALOG touches but the REQ-018 import boundary did not, and NOTHING that
// ImportSeamStubs already defines (a duplicate would fail to link, loudly).
//
// CloudServiceSyncDialog::refreshClicked walks context->athlete->rideCache to
// work out which remote activities the athlete already has. TEST-067 never
// needed a RideCache and ImportSeamStubs leaves Athlete::rideCache null; the
// dialog dereferences it, so this test needs a real, EMPTY one. Only the
// ctor/dtor are stubbed — rides() is inline in RideCache.h and returns the
// (empty) member vector for real, which is exactly the behaviour under test:
// an athlete with nothing local, so every remote activity is offered.
//
// Nothing here sits on the path under test (readFile -> readFailed ->
// failedRead -> syncNext).

#include "Context.h"
#include "LTMSettings.h"
#include "RideCache.h"
#include "RideCacheModel.h"

#include <QtGlobal>

// Same device as ImportSeamStubs: a QObject-derived class whose Q_OBJECT is not
// moc'd here still needs its three virtuals + staticMetaObject to link. The
// object never emits, receives or is qobject_cast in this test, so borrowing
// QObject's metaobject is sufficient — and fails loudly (a cast returns nullptr)
// rather than silently if that ever stops being true.
const QMetaObject RideCache::staticMetaObject = QObject::staticMetaObject;
const QMetaObject* RideCache::metaObject() const
{
    return &staticMetaObject;
}
void* RideCache::qt_metacast(const char* n)
{
    return QObject::qt_metacast(n);
}
int RideCache::qt_metacall(QMetaObject::Call c, int id, void** a)
{
    return QObject::qt_metacall(c, id, a);
}

// The REAL ctor loads RideDB.json, builds the metric estimator, starts a refresh
// thread and stands up a model. The dialog needs exactly one thing from the
// cache: an (empty) rides() vector. So this builds an inert one.
RideCache::RideCache(Context* context) : context(context)
{
    model_ = nullptr;
    exiting = false;
    progress_ = 100;
}

RideCache::~RideCache() {}
