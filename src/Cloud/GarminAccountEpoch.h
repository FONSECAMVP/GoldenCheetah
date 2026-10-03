/*
 * Copyright (c) 2026 GoldenCheetah Contributor
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the Free
 * Software Foundation; either version 2 of the License, or (at your option)
 * any later version.
 */

#ifndef GC_GarminAccountEpoch_h
#define GC_GarminAccountEpoch_h

#include <QHash>
#include <QString>
#include <QtGlobal>

// The per-athlete ACCOUNT EPOCH.
//
// A live GarminConnect that was open()ed while an account was connected keeps
// working after the user disconnects: disconnectService() deletes the token files
// and clears NO in-memory state, and nothing shuts down instances that are already
// open (the wizard's finish-with-sync dialog holds one open
// while Options -> Athlete -> Accounts -> Delete disconnects through a SECOND,
// freshly-minted instance). The obvious half of that is closed
// by re-reading the token file on every consuming call — but that
// predicate answers "is SOME account connected right now?", which is a different
// question from "is the account this session was opened against still the
// connected one?". Disconnect-then-reconnect answers the first YES while the
// second is NO.
//
// The epoch answers the second question with an int. Each athlete config dir
// carries a monotonically increasing counter; GarminConnect latches the current
// value at session open and compares on every readdir()/readFile(); Disconnect
// bumps it. A session whose latched value no longer matches is SUPERSEDED and
// refuses — with ZERO disk I/O, and without the disconnecting instance ever
// touching another instance's state (blockingDownload() runs a nested QEventLoop,
// so a disconnect CAN land inside a live download frame; reaching into other
// objects from there would be a use-after-free waiting to happen — see the
// Rationale in GarminConnect.cpp).
//
// This layers ON TOP of the token-file re-read guard; it does not replace it.
//
// Deliberately pure Qt — NO Python.h — the same seam discipline as
// GarminDownloadChain.h, so every Python-free `garmin-fast` test target
// can link it.
class GarminAccountEpoch
{
  public:
    // The epoch currently in force for `configDir`. Unknown dirs read 0, so a
    // session latched before anything was ever bumped compares equal (an athlete
    // who never disconnects never sees a change). Cheap: one hash lookup.
    static quint64 current(const QString& configDir);

    // Invalidate every session latched against `configDir`'s previous epoch, and
    // return the new value. Called by GarminConnect::disconnectService()
    // alongside GarminTokenStore::clearAccount(). Touches NOTHING but this map —
    // in particular it never dereferences another GarminConnect.
    static quint64 bump(const QString& configDir);

  private:
    // Keyed per config dir so athletes are isolated: disconnecting athlete A must
    // not invalidate a live session on athlete B. An empty configDir
    // (no athlete context) is a legitimate key of its own — such a service cannot
    // pass the token check anyway, so it is never able to act on the
    // result.
    static QHash<QString, quint64> s_epoch;
};

#endif // GC_GarminAccountEpoch_h
