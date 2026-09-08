/*
 * Copyright (c) 2026 GoldenCheetah Contributor
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the License as published by the Free Software Foundation.
 */

// REQ-022 / T-164..T-166 — the Context stand-in for the
// testGarminConnectOpenDataLifetime target. Mirrors exactly the real
// Context.h surface OpenData touches (Context.h:106 QObject base, the
// athleteClose(QString, Context*) signal declared at Context.h:288,
// athlete/mainWindow members) and nothing else.
//
// It lives in its own header, listed as a TARGET SOURCE so AUTOMOC scans
// its Q_OBJECT, because the production connect is string-based
// (SIGNAL(athleteClose(QString,Context*))) and a string-based connect
// needs a real sender metaobject at runtime. The force-included preamble
// (stubs/OpenDataLifetimeStubPreamble.h) must itself stay moc-free — see
// the equivalent note in stubs/WizardLifetimeStubPreamble.h.
//
// DRIFT DISCLOSURE (ORCH-059): the real Context ctor takes a MainWindow*
// and creates a QWebEngineProfile; this stand-in takes a QObject parent
// and nothing else. Neither difference is on the hazard path — the UAF is
// a read of the freed Context allocation itself, which ASan flags
// regardless of member layout, and OpenData only ever reads
// athlete/mainWindow off it.

#ifndef _GC_OPENDATA_LIFETIME_STUB_CONTEXT_H
#define _GC_OPENDATA_LIFETIME_STUB_CONTEXT_H

#include <QObject>
#include <QString>
#include <QWidget>

class Athlete; // defined in the preamble, which includes this file first

class Context : public QObject
{
    Q_OBJECT

  public:
    Context() = default;

    QWidget* mainWindow = nullptr;
    Athlete* athlete = nullptr;

    // Real Context.h:181 — Athlete::close() emits this as its FIRST act,
    // before the DEC-043 join and before MainWindow::removeAthleteTab's
    // `delete athlete; delete context;`.
    void notifyAthleteClose(QString folder, Context* context) { emit athleteClose(folder, context); }

  signals:

    void athleteClose(QString folder, Context* context);
};

#endif // _GC_OPENDATA_LIFETIME_STUB_CONTEXT_H
