/*
 * Copyright (c) 2026 GoldenCheetah Contributor
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the Free
 * Software Foundation; either version 2 of the License, or (at your option)
 * any later version.
 */

#include "Coach/GCToolExecutor.h"

#include <QTest>
#include <QDate>
#include <QJsonArray>
#include <QJsonObject>

class TestCoachTools : public QObject
{
    Q_OBJECT

private slots:

    // TEST-001 — validateDate: past date rejected (REQ-005, REQ-007)
    void validateDate_pastDate_rejected()
    {
        QString error;
        QString past = QDate::currentDate().addDays(-1).toString(Qt::ISODate);
        QVERIFY(!GCToolExecutor::validateDate(past, error));
        QVERIFY(!error.isEmpty());
    }

    // TEST-001 — validateDate: today accepted
    void validateDate_today_accepted()
    {
        QString error;
        QString today = QDate::currentDate().toString(Qt::ISODate);
        QVERIFY(GCToolExecutor::validateDate(today, error));
    }

    // TEST-001 — validateDate: future date accepted
    void validateDate_future_accepted()
    {
        QString error;
        QString future = QDate::currentDate().addDays(30).toString(Qt::ISODate);
        QVERIFY(GCToolExecutor::validateDate(future, error));
    }

    // TEST-001 — validateDate: invalid format rejected
    void validateDate_invalidFormat_rejected()
    {
        QString error;
        QVERIFY(!GCToolExecutor::validateDate("not-a-date", error));
        QVERIFY(!GCToolExecutor::validateDate("2026/05/11", error));
        QVERIFY(!GCToolExecutor::validateDate("", error));
    }

    // TEST-002 — sanitizeFilename: unsafe chars replaced (REQ-001)
    void sanitizeFilename_removesSlash()
    {
        QCOMPARE(GCToolExecutor::sanitizeFilename("Workout/Name"), QString("Workout_Name"));
    }

    void sanitizeFilename_removesColon()
    {
        QCOMPARE(GCToolExecutor::sanitizeFilename("5x5min:90%"), QString("5x5min_90%"));
    }

    void sanitizeFilename_removesMultipleSpecialChars()
    {
        QCOMPARE(GCToolExecutor::sanitizeFilename("A:B*C?D<E>F"), QString("A_B_C_D_E_F"));
    }

    void sanitizeFilename_emptyAfterTrim_returnsDefault()
    {
        QCOMPARE(GCToolExecutor::sanitizeFilename(""), QString("workout"));
        QCOMPARE(GCToolExecutor::sanitizeFilename("   "), QString("workout"));
    }

    void sanitizeFilename_normalName_unchanged()
    {
        QCOMPARE(GCToolExecutor::sanitizeFilename("VO2max Intervals"), QString("VO2max Intervals"));
    }

    // TEST-003 — buildZwoXml: XML structure correct (REQ-001)
    void buildZwoXml_steadyState_correctStructure()
    {
        QJsonArray intervals;
        QJsonObject iv;
        iv["type"] = "steadystate";
        iv["duration_seconds"] = 1800;
        iv["power_low_pct"] = 75.0;
        intervals.append(iv);

        QString xml = GCToolExecutor::buildZwoXml("FTP Test", "Steady at 75%", intervals);

        QVERIFY(xml.contains("<workout_file>"));
        QVERIFY(xml.contains("<name>FTP Test</name>"));
        QVERIFY(xml.contains("<description>Steady at 75%</description>"));
        QVERIFY(xml.contains("<SteadyState Duration=\"1800\""));
        QVERIFY(xml.contains("Power=\"0.750\""));
        QVERIFY(xml.contains("</workout_file>"));
    }

    void buildZwoXml_warmup_correctTag()
    {
        QJsonArray intervals;
        QJsonObject iv;
        iv["type"] = "warmup";
        iv["duration_seconds"] = 600;
        iv["power_low_pct"] = 40.0;
        iv["power_high_pct"] = 60.0;
        intervals.append(iv);

        QString xml = GCToolExecutor::buildZwoXml("WU", "", intervals);
        QVERIFY(xml.contains("<Warmup Duration=\"600\""));
        QVERIFY(xml.contains("PowerLow=\"0.400\""));
        QVERIFY(xml.contains("PowerHigh=\"0.600\""));
    }

    void buildZwoXml_cooldown_correctTag()
    {
        QJsonArray intervals;
        QJsonObject iv;
        iv["type"] = "cooldown";
        iv["duration_seconds"] = 300;
        iv["power_low_pct"] = 50.0;
        iv["power_high_pct"] = 30.0;
        intervals.append(iv);

        QString xml = GCToolExecutor::buildZwoXml("CD", "", intervals);
        QVERIFY(xml.contains("<Cooldown Duration=\"300\""));
    }

    void buildZwoXml_intervals_correctRepeat()
    {
        QJsonArray intervals;
        QJsonObject iv;
        iv["type"] = "intervals";
        iv["duration_seconds"] = 900;
        iv["power_low_pct"] = 100.0;
        iv["repeat"] = 5;
        iv["on_duration_seconds"] = 60;
        iv["off_duration_seconds"] = 60;
        iv["on_power_pct"] = 120.0;
        iv["off_power_pct"] = 50.0;
        intervals.append(iv);

        QString xml = GCToolExecutor::buildZwoXml("5x1min VO2max", "", intervals);
        QVERIFY(xml.contains("<IntervalsT Repeat=\"5\""));
        QVERIFY(xml.contains("OnDuration=\"60\""));
        QVERIFY(xml.contains("OffDuration=\"60\""));
        QVERIFY(xml.contains("OnPower=\"1.200\""));
        QVERIFY(xml.contains("OffPower=\"0.500\""));
    }

    void buildZwoXml_ramp_correctTag()
    {
        QJsonArray intervals;
        QJsonObject iv;
        iv["type"] = "ramp";
        iv["duration_seconds"] = 1200;
        iv["power_low_pct"] = 50.0;
        iv["power_high_pct"] = 100.0;
        intervals.append(iv);

        QString xml = GCToolExecutor::buildZwoXml("Ramp Test", "", intervals);
        QVERIFY(xml.contains("<Ramp Duration=\"1200\""));
    }

    void buildZwoXml_free_correctTag()
    {
        QJsonArray intervals;
        QJsonObject iv;
        iv["type"] = "free";
        iv["duration_seconds"] = 600;
        iv["power_low_pct"] = 50.0;
        intervals.append(iv);

        QString xml = GCToolExecutor::buildZwoXml("Free Ride", "", intervals);
        QVERIFY(xml.contains("<FreeRide Duration=\"600\""));
    }

    void buildZwoXml_htmlEscapesNameAndDescription()
    {
        QJsonArray intervals;
        QJsonObject iv;
        iv["type"] = "steadystate";
        iv["duration_seconds"] = 600;
        iv["power_low_pct"] = 50.0;
        intervals.append(iv);

        QString xml = GCToolExecutor::buildZwoXml("<Tricky & \"Name\">", "<Desc>", intervals);
        QVERIFY(xml.contains("&lt;Tricky &amp; &quot;Name&quot;&gt;"));
        QVERIFY(xml.contains("&lt;Desc&gt;"));
    }

    // TEST-004 — v1Tools: all 4 tools present (REQ-001, REQ-004, REQ-007, REQ-009)
    void v1Tools_hasFourTools()
    {
        QList<LLMService::ToolDef> tools = GCToolExecutor::v1Tools();
        QCOMPARE(tools.size(), 4);
        QStringList names;
        for (const auto& t : tools) names.append(t.name);
        QVERIFY(names.contains("create_workout"));
        QVERIFY(names.contains("schedule_workout"));
        QVERIFY(names.contains("create_season_event"));
        QVERIFY(names.contains("create_training_plan"));
    }

    void v1Tools_createWorkout_requiredFields()
    {
        auto tools = GCToolExecutor::v1Tools();
        LLMService::ToolDef def;
        for (const auto& t : tools) { if (t.name == "create_workout") { def = t; break; } }

        QJsonArray req = def.inputSchema["required"].toArray();
        QStringList reqList;
        for (const auto& v : req) reqList.append(v.toString());
        QVERIFY(reqList.contains("name"));
        QVERIFY(reqList.contains("intervals"));
    }

    void v1Tools_seasonEvent_priorityRequired()
    {
        auto tools = GCToolExecutor::v1Tools();
        LLMService::ToolDef def;
        for (const auto& t : tools) { if (t.name == "create_season_event") { def = t; break; } }

        QJsonArray req = def.inputSchema["required"].toArray();
        QStringList reqList;
        for (const auto& v : req) reqList.append(v.toString());
        QVERIFY(reqList.contains("priority")); // REQ-008: priority MUST be A/B/C
    }

    void v1Tools_trainingPlan_weeksConstraints()
    {
        auto tools = GCToolExecutor::v1Tools();
        LLMService::ToolDef def;
        for (const auto& t : tools) { if (t.name == "create_training_plan") { def = t; break; } }

        QJsonObject weeks = def.inputSchema["properties"].toObject()["weeks"].toObject();
        QCOMPARE(weeks["maxItems"].toInt(), 16); // A2: cap at 16 weeks
        QCOMPARE(weeks["minItems"].toInt(), 1);  // A2: at least 1 week
    }

    void v1Tools_scheduleWorkout_dateAndNameRequired()
    {
        auto tools = GCToolExecutor::v1Tools();
        LLMService::ToolDef def;
        for (const auto& t : tools) { if (t.name == "schedule_workout") { def = t; break; } }

        QJsonArray req = def.inputSchema["required"].toArray();
        QStringList reqList;
        for (const auto& v : req) reqList.append(v.toString());
        QVERIFY(reqList.contains("workout_name"));
        QVERIFY(reqList.contains("date"));
    }
};

QTEST_MAIN(TestCoachTools)
#include "testCoachTools.moc"
