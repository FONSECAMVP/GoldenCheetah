/*
 * Copyright (c) 2026 GoldenCheetah Contributor
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the Free
 * Software Foundation; either version 2 of the License, or (at your option)
 * any later version.
 */

// TEST-001 — REQ-001: "Tile renders with logo and short description;
//            clicking opens credentials dialog."
//
// Scope rationale: AddCloudWizard tile rendering (src/Cloud/AddCloudWizard.cpp
// lines 167-188) consumes only the CloudService registry contract — id(),
// uiName(), description(), type(), capabilities(), logo(), clone(). Driving
// the full QWizard requires a Context+Athlete and an event loop, which
// overshoots REQ-001's acceptance. The "clicking opens credentials dialog"
// half of REQ-001's acceptance is folded into REQ-002 where the
// GarminCredentialsPage is actually built. Here we lock the *contract* the
// wizard reads when building the tile.
//
// Negative path (DoD must-have bar): duplicate registration is rejected.
// This guards the CloudServiceFactory invariant that ids are unique.
//
// Cites: DEC-001 (solution shape), DEC-005 (CloudService subclass approach),
//        DES-003 (wizard pages), DES-004 (CloudService subclass shape).

#include "GarminConnect.h" // <-- intentionally missing in RED phase

#include <QImage>
#include <QString>
#include <QtTest/QtTest>

class TestGarminConnectTile : public QObject
{
    Q_OBJECT

  private slots:
    // Static-init in GarminConnect.cpp must register the service before main()
    // runs, mirroring the precedent set by Selfloops.cpp.
    void registeredInFactory()
    {
        const QStringList names = CloudServiceFactory::instance().serviceNames();
        QVERIFY2(names.contains(QStringLiteral("Garmin Connect")),
                 "Garmin Connect must self-register via static initializer");
        const CloudService* s = CloudServiceFactory::instance().service(QStringLiteral("Garmin Connect"));
        QVERIFY2(s != nullptr, "Factory::service(\"Garmin Connect\") must return the registered instance");
    }

    // The tile reads uiName() and description(). Both must be human-readable
    // and non-empty for REQ-001's "Tile renders with [...] short description".
    void serviceMetadata()
    {
        const CloudService* s = CloudServiceFactory::instance().service(QStringLiteral("Garmin Connect"));
        QVERIFY(s != nullptr);
        QCOMPARE(s->id(), QStringLiteral("Garmin Connect"));
        QVERIFY2(!s->uiName().isEmpty(), "uiName() must be non-empty for tile label");
        QVERIFY2(
            s->uiName().contains(QStringLiteral("Garmin"), Qt::CaseInsensitive),
            "uiName() must brand the service (A3 mutant kill: catches a missing override that falls back to 'None')");
        QVERIFY2(!s->description().isEmpty(), "description() must be non-empty for tile subtitle");
    }

    // Capability mask must be exactly Query|Download per DES-004. No write
    // surface — DEC-001 scoped this feature to read-only download.
    void capabilitiesAreQueryDownload()
    {
        const CloudService* s = CloudServiceFactory::instance().service(QStringLiteral("Garmin Connect"));
        QVERIFY(s != nullptr);
        const int caps = s->capabilities();
        const int expected = CloudService::Query | CloudService::Download;
        QCOMPARE(caps, expected);
        QVERIFY2((caps & CloudService::Upload) == 0, "Garmin Connect must NOT advertise Upload capability");
        QVERIFY2((caps & CloudService::OAuth) == 0, "Garmin Connect uses UserPass/MFA, not OAuth — see DEC-005");
    }

    // type() must be Activities so the wizard's Activities-tile loop picks it
    // up (AddCloudWizard.cpp line 171: `if (s->type() != wizard->type) continue;`).
    void typeIsActivities()
    {
        const CloudService* s = CloudServiceFactory::instance().service(QStringLiteral("Garmin Connect"));
        QVERIFY(s != nullptr);
        QCOMPARE(s->type(), int(CloudService::Activities));
    }

    // Logo must be non-null. QTEST_APPLESS_MAIN does not load Qt resources by
    // default, so logo() must either (a) Q_INIT_RESOURCE itself or (b) return
    // an in-code fallback. Either is acceptable; the contract is "non-null".
    void logoIsNonNull()
    {
        const CloudService* s = CloudServiceFactory::instance().service(QStringLiteral("Garmin Connect"));
        QVERIFY(s != nullptr);
        const QImage img = s->logo();
        QVERIFY2(!img.isNull(), "logo() must return a non-null QImage for tile");
        QVERIFY(img.width() > 0);
        QVERIFY(img.height() > 0);
    }

    // clone() with a null context is how the factory hands out instances
    // pre-context-bind. Must not crash, must return a usable subclass instance
    // with the same id().
    void cloneWithNullContextSucceeds()
    {
        const CloudService* s = CloudServiceFactory::instance().service(QStringLiteral("Garmin Connect"));
        QVERIFY(s != nullptr);
        CloudService* cloned = const_cast<CloudService*>(s)->clone(nullptr);
        QVERIFY2(cloned != nullptr, "clone(nullptr) must return a valid instance");
        QVERIFY2(cloned != s, "clone() must return a NEW instance, not the singleton — otherwise "
                              "delete cloned destroys the factory entry (A3 mutant kill)");
        QCOMPARE(cloned->id(), QStringLiteral("Garmin Connect"));
        delete cloned;
    }

    // Simulates the wizard tile-build loop (AddCloudWizard.cpp 167-188): the
    // service must be discoverable by iterating serviceNames() and filtering
    // on type==Activities. This is the closest possible mirror of REQ-001's
    // "Tile renders" path without standing up the full QWizard.
    void wizardTileLoopFindsGarmin()
    {
        bool found = false;
        QString foundUiName, foundDescription;
        const CloudServiceFactory& factory = CloudServiceFactory::instance();
        for (const QString& name : factory.serviceNames()) {
            const CloudService* s = factory.service(name);
            if (s == nullptr)
                continue;
            if (s->type() != CloudService::Activities)
                continue;
            if (s->id() != QStringLiteral("Garmin Connect"))
                continue;
            found = true;
            foundUiName = s->uiName();
            foundDescription = s->description();
            break;
        }
        QVERIFY2(found, "Wizard tile loop must surface Garmin Connect under Activities");
        QVERIFY(!foundUiName.isEmpty());
        QVERIFY(!foundDescription.isEmpty());
    }

    // NEGATIVE PATH (DoD must-have bar): registering a second instance with
    // the same id must be rejected by the factory. Protects the registry
    // uniqueness invariant — without this, a buggy plugin or double-include
    // could silently shadow the real service.
    void duplicateRegistrationRejected()
    {
        CloudServiceFactory& factory = CloudServiceFactory::instance();
        const int countBefore = factory.serviceCount();
        const bool accepted = factory.addService(new GarminConnect(nullptr));
        QVERIFY2(!accepted, "addService must refuse a second registration with the same id");
        QCOMPARE(factory.serviceCount(), countBefore);
    }
};

QTEST_APPLESS_MAIN(TestGarminConnectTile)
#include "testGarminConnectTile.moc"
