/*
 * Copyright (c) 2026 GoldenCheetah Contributor
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the Free
 * Software Foundation; either version 2 of the License, or (at your option)
 * any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc., 51
 * Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 */

#include "GCToolExecutor.h"
#include "ToolConfirmCard.h"
#include "PlanPreviewCard.h"

#include "Context.h"
#include "Athlete.h"
#include "Season.h"
#include "Seasons.h"

#include <QFile>
#include <QTextStream>
#include <QUuid>
#include <QDate>
#include <QDir>
#include <QVBoxLayout>
#include <QJsonArray>
#include <QJsonDocument>

GCToolExecutor::GCToolExecutor(Context* context, QObject* parent)
    : QObject(parent)
    , context_(context)
{
}

void GCToolExecutor::registerTools(LLMService* service)
{
    llmService_ = service;
    if (service->supportsTools()) {
        service->setTools(v1Tools());
    }
}

void GCToolExecutor::setChatLayout(QVBoxLayout* layout)
{
    chatLayout_ = layout;
}

QJsonObject GCToolExecutor::makeToolSchema(const QString& name, const QString& description,
                                            const QJsonObject& properties,
                                            const QJsonArray& required)
{
    Q_UNUSED(name)
    Q_UNUSED(description)
    return QJsonObject{
        {"type", "object"},
        {"properties", properties},
        {"required", required}
    };
}

QList<LLMService::ToolDef> GCToolExecutor::v1Tools()
{
    QList<LLMService::ToolDef> tools;

    // ---- create_workout ----
    {
        QJsonObject intervalProps{
            {"type", QJsonObject{
                {"type","string"},
                {"enum", QJsonArray{"warmup","cooldown","steadystate","intervals","ramp","free"}},
                {"description","Interval block type"}
            }},
            {"duration_seconds", QJsonObject{
                {"type","integer"},
                {"description","Duration of this block in seconds"}
            }},
            {"power_low_pct", QJsonObject{
                {"type","number"},
                {"description","Power as % of FTP (low end or fixed). 0-300."}
            }},
            {"power_high_pct", QJsonObject{
                {"type","number"},
                {"description","Power as % of FTP (high end for ramps/warmup/cooldown). 0-300."}
            }},
            {"repeat", QJsonObject{
                {"type","integer"},
                {"description","Number of repetitions (intervals type only)"}
            }},
            {"on_duration_seconds", QJsonObject{
                {"type","integer"},
                {"description","On-interval duration in seconds (intervals type only)"}
            }},
            {"off_duration_seconds", QJsonObject{
                {"type","integer"},
                {"description","Off/recovery interval duration in seconds (intervals type only)"}
            }},
            {"on_power_pct", QJsonObject{
                {"type","number"},
                {"description","Power during on-interval as % FTP"}
            }},
            {"off_power_pct", QJsonObject{
                {"type","number"},
                {"description","Power during off-interval as % FTP"}
            }}
        };

        QJsonObject workoutProps{
            {"name", QJsonObject{
                {"type","string"},
                {"description","Workout name"}
            }},
            {"description", QJsonObject{
                {"type","string"},
                {"description","Short description of the workout"}
            }},
            {"intervals", QJsonObject{
                {"type","array"},
                {"items", QJsonObject{
                    {"type","object"},
                    {"properties", intervalProps},
                    {"required", QJsonArray{"type","duration_seconds","power_low_pct"}}
                }},
                {"description","Ordered list of workout blocks"}
            }}
        };

        LLMService::ToolDef td;
        td.name = "create_workout";
        td.description = "Generate a structured .zwo workout file and save it to the athlete's workout library.";
        td.inputSchema = makeToolSchema(td.name, td.description, workoutProps,
                                        QJsonArray{"name","intervals"});
        tools.append(td);
    }

    // ---- schedule_workout ----
    {
        QJsonObject props{
            {"workout_name", QJsonObject{
                {"type","string"},
                {"description","Name of the workout to schedule"}
            }},
            {"date", QJsonObject{
                {"type","string"},
                {"description","ISO 8601 date (YYYY-MM-DD) for the workout"}
            }},
            {"notes", QJsonObject{
                {"type","string"},
                {"description","Optional notes for the calendar entry"}
            }},
            {"force", QJsonObject{
                {"type","boolean"},
                {"description","Set true to schedule even if another event already exists on that date"}
            }}
        };

        LLMService::ToolDef td;
        td.name = "schedule_workout";
        td.description = "Add a workout to the athlete's season calendar on a specific date. "
                         "If an event already exists on that date, this returns a conflict error "
                         "unless force:true is set.";
        td.inputSchema = makeToolSchema(td.name, td.description, props,
                                        QJsonArray{"workout_name","date"});
        tools.append(td);
    }

    // ---- create_season_event ----
    {
        QJsonObject props{
            {"name", QJsonObject{
                {"type","string"},
                {"description","Event name (e.g. race or goal name)"}
            }},
            {"date", QJsonObject{
                {"type","string"},
                {"description","ISO 8601 date (YYYY-MM-DD) of the event"}
            }},
            {"priority", QJsonObject{
                {"type","string"},
                {"enum", QJsonArray{"A","B","C"}},
                {"description","Priority: A = most important, C = low priority"}
            }},
            {"description", QJsonObject{
                {"type","string"},
                {"description","Optional description of the event"}
            }}
        };

        LLMService::ToolDef td;
        td.name = "create_season_event";
        td.description = "Add a race or goal event to the athlete's season plan.";
        td.inputSchema = makeToolSchema(td.name, td.description, props,
                                        QJsonArray{"name","date","priority"});
        tools.append(td);
    }

    // ---- create_training_plan ----
    {
        QJsonObject workoutInPlanProps{
            {"name", QJsonObject{
                {"type","string"},
                {"description","Workout name"}
            }},
            {"day", QJsonObject{
                {"type","string"},
                {"description","Day of week (e.g. Monday, Tuesday...)"}
            }},
            {"description", QJsonObject{
                {"type","string"},
                {"description","Brief workout description"}
            }},
            {"intervals", QJsonObject{
                {"type","array"},
                {"items", QJsonObject{{"type","object"}}},
                {"description","Workout intervals (same schema as create_workout intervals)"}
            }}
        };

        QJsonObject weekProps{
            {"name", QJsonObject{
                {"type","string"},
                {"description","Week label (e.g. Week 1 - Base)"}
            }},
            {"workouts", QJsonObject{
                {"type","array"},
                {"items", QJsonObject{
                    {"type","object"},
                    {"properties", workoutInPlanProps},
                    {"required", QJsonArray{"name","day"}}
                }},
                {"description","Workouts for this week"}
            }}
        };

        QJsonObject props{
            {"name", QJsonObject{
                {"type","string"},
                {"description","Training plan name"}
            }},
            {"start_date", QJsonObject{
                {"type","string"},
                {"description","ISO 8601 start date (YYYY-MM-DD)"}
            }},
            {"weeks", QJsonObject{
                {"type","array"},
                {"items", QJsonObject{
                    {"type","object"},
                    {"properties", weekProps},
                    {"required", QJsonArray{"workouts"}}
                }},
                {"minItems", 1},
                {"maxItems", 16},
                {"description","Array of weeks (1–16 weeks)"}
            }}
        };

        LLMService::ToolDef td;
        td.name = "create_training_plan";
        td.description = "Build a multi-week training plan with workouts and schedule them. Shows a preview card before applying.";
        td.inputSchema = makeToolSchema(td.name, td.description, props,
                                        QJsonArray{"name","start_date","weeks"});
        tools.append(td);
    }

    return tools;
}

void GCToolExecutor::onToolCallRequested(const QString& callId, const QString& toolName,
                                          const QJsonObject& args)
{
    if (QJsonDocument(args).toJson().size() > 65536) {
        QJsonObject result{{"error","Tool args payload too large"},{"cancelled",false}};
        emit toolResultReady(callId, toolName, result);
        return;
    }

    if (toolName == "create_training_plan") {
        showPlanPreview(callId, args);
        return;
    }

    showConfirmCard(callId, toolName, args);
}

void GCToolExecutor::showConfirmCard(const QString& callId, const QString& toolName,
                                      const QJsonObject& args)
{
    if (!chatLayout_) {
        onConfirmed(callId, toolName, args);
        return;
    }

    ToolConfirmCard* card = new ToolConfirmCard(callId, toolName, args, chatLayout_->parentWidget());
    connect(card, &ToolConfirmCard::confirmed, this, &GCToolExecutor::onConfirmed);
    connect(card, &ToolConfirmCard::cancelled, this, &GCToolExecutor::onCancelled);

    int stretchIdx = chatLayout_->count() - 1;
    chatLayout_->insertWidget(stretchIdx, card);

    if (chatLayout_->parentWidget() && chatLayout_->parentWidget()->parentWidget()) {
        chatLayout_->parentWidget()->updateGeometry();
    }
}

void GCToolExecutor::showPlanPreview(const QString& callId, const QJsonObject& plan)
{
    if (!chatLayout_) {
        onPlanApplied(callId, plan);
        return;
    }

    PlanPreviewCard* card = new PlanPreviewCard(callId, plan, chatLayout_->parentWidget());
    connect(card, &PlanPreviewCard::planApplied, this, &GCToolExecutor::onPlanApplied);
    connect(card, &PlanPreviewCard::planCancelled, this, &GCToolExecutor::onPlanCancelled);

    int stretchIdx = chatLayout_->count() - 1;
    chatLayout_->insertWidget(stretchIdx, card);

    if (chatLayout_->parentWidget() && chatLayout_->parentWidget()->parentWidget()) {
        chatLayout_->parentWidget()->updateGeometry();
    }
}

void GCToolExecutor::onConfirmed(const QString& callId, const QString& toolName,
                                  const QJsonObject& args)
{
    QJsonObject result;
    if (toolName == "create_workout") {
        result = executeCreateWorkout(args);
    } else if (toolName == "schedule_workout") {
        result = executeScheduleWorkout(args);
    } else if (toolName == "create_season_event") {
        result = executeCreateSeasonEvent(args);
    } else {
        result = QJsonObject{{"error", QString("Unknown tool: %1").arg(toolName)}};
    }

    emit toolResultReady(callId, toolName, result);
}

void GCToolExecutor::onCancelled(const QString& callId, const QString& toolName)
{
    QJsonObject result{
        {"cancelled", true},
        {"message", "User cancelled the action"}
    };
    emit toolResultReady(callId, toolName, result);
}

void GCToolExecutor::onPlanApplied(const QString& callId, const QJsonObject& plan)
{
    QJsonArray weeks = plan["weeks"].toArray();
    if (weeks.isEmpty()) {
        emit toolResultReady(callId, "create_training_plan",
            QJsonObject{{"error","Plan must contain at least one week"}});
        return;
    }
    if (weeks.size() > 16) {
        emit toolResultReady(callId, "create_training_plan",
            QJsonObject{{"error","Plan exceeds 16 weeks maximum"}});
        return;
    }

    QString startDateStr = plan["start_date"].toString();
    QDate startDate = QDate::fromString(startDateStr, Qt::ISODate);
    if (!startDate.isValid()) {
        emit toolResultReady(callId, "create_training_plan",
            QJsonObject{{"error","Invalid start_date in plan"}});
        return;
    }

    int workoutsCreated = 0;
    int scheduled = 0;
    QStringList writtenFiles; // track for atomic rollback on failure

    static const QMap<QString, int> dayOffsets = {
        {"Monday",0},{"Tuesday",1},{"Wednesday",2},{"Thursday",3},
        {"Friday",4},{"Saturday",5},{"Sunday",6}
    };

    for (int w = 0; w < weeks.size(); ++w) {
        QJsonObject week = weeks[w].toObject();
        QJsonArray workouts = week["workouts"].toArray();
        QDate weekStart = startDate.addDays(w * 7);

        for (const QJsonValue& woVal : workouts) {
            QJsonObject wo = woVal.toObject();
            QString woName = wo["name"].toString();
            QString dayStr = wo["day"].toString();
            QString woDesc = wo["description"].toString();
            QJsonArray intervals = wo["intervals"].toArray();

            if (intervals.isEmpty() && !woDesc.isEmpty()) {
                QJsonObject placeholder{
                    {"type","steadystate"},
                    {"duration_seconds",3600},
                    {"power_low_pct",65.0}
                };
                intervals.append(placeholder);
            }

            if (!woName.isEmpty() && !intervals.isEmpty()) {
                QJsonObject createArgs{
                    {"name", woName},
                    {"description", woDesc},
                    {"intervals", intervals}
                };
                QJsonObject createResult = executeCreateWorkout(createArgs);
                if (createResult["status"].toString() == "success") {
                    workoutsCreated++;
                    writtenFiles.append(createResult["path"].toString());
                } else {
                    // Atomic rollback: delete all files written so far
                    for (const QString& path : writtenFiles) {
                        QFile::remove(path);
                    }
                    emit toolResultReady(callId, "create_training_plan",
                        QJsonObject{{"error",
                            QString("Plan rolled back after failure: %1").arg(
                                createResult["error"].toString())}});
                    return;
                }
            }

            if (!woName.isEmpty() && !dayStr.isEmpty() && dayOffsets.contains(dayStr)) {
                int offset = dayOffsets[dayStr];
                QDate schedDate = weekStart.addDays(offset);
                QJsonObject schedArgs{
                    {"workout_name", woName},
                    {"date", schedDate.toString(Qt::ISODate)},
                    {"notes", woDesc}
                };
                QJsonObject schedResult = executeScheduleWorkout(schedArgs);
                if (schedResult["status"].toString() == "success") {
                    scheduled++;
                }
            }
        }
    }

    emit actionMessage(tr("Training plan applied: %1 workouts created, %2 events scheduled.")
        .arg(workoutsCreated).arg(scheduled));
    emit toolResultReady(callId, "create_training_plan", QJsonObject{
        {"status","success"},
        {"workouts_created", workoutsCreated},
        {"events_scheduled", scheduled},
        {"message", QString("Plan applied: %1 workouts created, %2 scheduled")
            .arg(workoutsCreated).arg(scheduled)}
    });
}

void GCToolExecutor::onPlanCancelled(const QString& callId)
{
    QJsonObject result{
        {"cancelled", true},
        {"message", "User cancelled the training plan"}
    };
    emit toolResultReady(callId, "create_training_plan", result);
}

QJsonObject GCToolExecutor::executeCreateWorkout(const QJsonObject& args)
{
    QString name = args["name"].toString().trimmed();
    QString description = args["description"].toString();
    QJsonArray intervals = args["intervals"].toArray();

    if (name.isEmpty()) {
        return QJsonObject{{"error","Workout name is required"}};
    }
    if (intervals.isEmpty()) {
        return QJsonObject{{"error","At least one interval block is required"}};
    }

    for (const QJsonValue& iv : intervals) {
        QJsonObject obj = iv.toObject();
        double powerLow = obj["power_low_pct"].toDouble(-1.0);
        double powerHigh = obj["power_high_pct"].toDouble(powerLow);
        double onPower = obj["on_power_pct"].toDouble(-1.0);
        double offPower = obj["off_power_pct"].toDouble(-1.0);
        for (double p : {powerLow, powerHigh, onPower, offPower}) {
            if (p < 0.0) continue; // -1.0 sentinel = field absent
            if (p > 300.0) {
                return QJsonObject{{"error", QString("Power value %1% is out of range [0,300]").arg(p)}};
            }
        }
    }

    if (!context_ || !context_->athlete || !context_->athlete->home) {
        return QJsonObject{{"error","Athlete context unavailable"}};
    }

    QDir workoutDir = context_->athlete->home->workouts();
    if (!workoutDir.exists()) {
        workoutDir.mkpath(".");
    }

    QString filename = sanitizeFilename(name) + ".zwo";
    QString fullPath = workoutDir.filePath(filename);

    QString xml = buildZwoXml(name, description, intervals);

    QFile file(fullPath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        return QJsonObject{{"error", QString("Could not write file: %1").arg(fullPath)}};
    }
    QTextStream out(&file);
    out << xml;
    file.close();

    emit actionMessage(tr("Workout '%1' saved to library.").arg(name));

    return QJsonObject{
        {"status","success"},
        {"filename", filename},
        {"path", fullPath},
        {"message", QString("Workout '%1' saved to library").arg(name)}
    };
}

// Returns index of best-matching season for date, auto-creating a year season if none exist.
int GCToolExecutor::ensureActiveSeason(Seasons* seasons, const QDate& date)
{
    if (seasons->seasons.isEmpty()) {
        int year = date.isValid() ? date.year() : QDate::currentDate().year();
        seasons->newSeason(QString("%1 Season").arg(year),
                           QDate(year, 1, 1), QDate(year, 12, 31),
                           Season::season);
        emit actionMessage(tr("No season found — created '%1 Season' automatically.").arg(year));
    }

    QDate today = QDate::currentDate();
    for (int i = 0; i < seasons->seasons.size(); ++i) {
        if (seasons->seasons[i].getStart() <= today && today <= seasons->seasons[i].getEnd())
            return i;
    }
    return 0;
}

QJsonObject GCToolExecutor::executeScheduleWorkout(const QJsonObject& args)
{
    QString workoutName = args["workout_name"].toString().trimmed();
    QString dateStr = args["date"].toString().trimmed();
    QString notes = args["notes"].toString();
    bool force = args["force"].toBool(false);

    if (workoutName.isEmpty()) {
        return QJsonObject{{"error","workout_name is required"}};
    }

    QString dateError;
    if (!validateDate(dateStr, dateError)) {
        return QJsonObject{{"error", dateError}};
    }

    QDate date = QDate::fromString(dateStr, Qt::ISODate);

    if (!context_ || !context_->athlete || !context_->athlete->seasons) {
        return QJsonObject{{"error","Seasons context unavailable"}};
    }

    Seasons* seasons = context_->athlete->seasons;
    int targetIdx = ensureActiveSeason(seasons, date);

    // REQ-006: detect conflicts before writing
    if (!force) {
        QStringList existingOnDate;
        for (const SeasonEvent& ev : seasons->seasons[targetIdx].events) {
            if (ev.date == date) {
                existingOnDate.append(ev.name);
            }
        }
        if (!existingOnDate.isEmpty()) {
            return QJsonObject{
                {"status", "conflict"},
                {"existing", existingOnDate.join(", ")},
                {"message", QString("Date %1 already has: %2. "
                    "Tell the athlete and ask if they want to replace it. "
                    "If yes, call schedule_workout again with force:true.")
                    .arg(dateStr, existingOnDate.join(", "))}
            };
        }
    }

    QString eventDesc = notes.isEmpty() ? QString("Workout: %1").arg(workoutName)
                                        : QString("Workout: %1\n%2").arg(workoutName, notes);

    SeasonEvent event(workoutName, date, 0, eventDesc, QUuid::createUuid().toString());
    seasons->seasons[targetIdx].events.append(event);
    seasons->writeSeasons();
    context_->notifyConfigChanged(CONFIG_SEASONS);

    emit actionMessage(tr("Workout '%1' scheduled on %2.").arg(workoutName, dateStr));

    return QJsonObject{
        {"status","success"},
        {"message", QString("Workout '%1' scheduled on %2").arg(workoutName, dateStr)}
    };
}

QJsonObject GCToolExecutor::executeCreateSeasonEvent(const QJsonObject& args)
{
    QString name = args["name"].toString().trimmed();
    QString dateStr = args["date"].toString().trimmed();
    QString priorityStr = args["priority"].toString("A");
    QString description = args["description"].toString();

    if (name.isEmpty()) {
        return QJsonObject{{"error","Event name is required"}};
    }

    QString dateError;
    if (!validateDate(dateStr, dateError)) {
        return QJsonObject{{"error", dateError}};
    }

    QDate date = QDate::fromString(dateStr, Qt::ISODate);

    int priority = 0;
    if (priorityStr == "B") priority = 1;
    else if (priorityStr == "C") priority = 2;

    if (!context_ || !context_->athlete || !context_->athlete->seasons) {
        return QJsonObject{{"error","Seasons context unavailable"}};
    }

    Seasons* seasons = context_->athlete->seasons;
    int targetIdx = ensureActiveSeason(seasons, date);

    SeasonEvent event(name, date, priority, description, QUuid::createUuid().toString());
    seasons->seasons[targetIdx].events.append(event);
    seasons->writeSeasons();
    context_->notifyConfigChanged(CONFIG_SEASONS);

    emit actionMessage(tr("Event '%1' added on %2 (Priority %3).").arg(name, dateStr, priorityStr));

    return QJsonObject{
        {"status","success"},
        {"message", QString("Event '%1' added on %2 with priority %3").arg(name, dateStr, priorityStr)}
    };
}

QString GCToolExecutor::buildZwoXml(const QString& name, const QString& description,
                                     const QJsonArray& intervals)
{
    QString xml;
    QTextStream out(&xml);

    out << "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n";
    out << "<workout_file>\n";
    out << "  <author>GoldenCheetah AI Coach</author>\n";
    out << "  <name>" << name.toHtmlEscaped() << "</name>\n";
    out << "  <description>" << description.toHtmlEscaped() << "</description>\n";
    out << "  <sportType>bike</sportType>\n";
    out << "  <workout>\n";

    for (const QJsonValue& iv : intervals) {
        QJsonObject obj = iv.toObject();
        QString type = obj["type"].toString();
        int duration = obj["duration_seconds"].toInt(300);
        double powerLow = obj["power_low_pct"].toDouble(50.0) / 100.0;
        double powerHigh = obj["power_high_pct"].toDouble(obj["power_low_pct"].toDouble(50.0)) / 100.0;

        if (type == "warmup") {
            out << QString("    <Warmup Duration=\"%1\" PowerLow=\"%2\" PowerHigh=\"%3\"/>\n")
                       .arg(duration).arg(powerLow, 0,'f',3).arg(powerHigh, 0,'f',3);
        } else if (type == "cooldown") {
            out << QString("    <Cooldown Duration=\"%1\" PowerLow=\"%2\" PowerHigh=\"%3\"/>\n")
                       .arg(duration).arg(powerLow, 0,'f',3).arg(powerHigh, 0,'f',3);
        } else if (type == "steadystate") {
            out << QString("    <SteadyState Duration=\"%1\" Power=\"%2\"/>\n")
                       .arg(duration).arg(powerLow, 0,'f',3);
        } else if (type == "intervals") {
            int repeat = obj["repeat"].toInt(1);
            int onDur = obj["on_duration_seconds"].toInt(duration / 2);
            int offDur = obj["off_duration_seconds"].toInt(duration / 2);
            double onPower = obj["on_power_pct"].toDouble(obj["power_low_pct"].toDouble(100.0)) / 100.0;
            double offPower = obj["off_power_pct"].toDouble(50.0) / 100.0;
            out << QString("    <IntervalsT Repeat=\"%1\" OnDuration=\"%2\" OffDuration=\"%3\" OnPower=\"%4\" OffPower=\"%5\"/>\n")
                       .arg(repeat).arg(onDur).arg(offDur)
                       .arg(onPower, 0,'f',3).arg(offPower, 0,'f',3);
        } else if (type == "ramp") {
            out << QString("    <Ramp Duration=\"%1\" PowerLow=\"%2\" PowerHigh=\"%3\"/>\n")
                       .arg(duration).arg(powerLow, 0,'f',3).arg(powerHigh, 0,'f',3);
        } else { // free
            out << QString("    <FreeRide Duration=\"%1\" FlatRoad=\"1\"/>\n").arg(duration);
        }
    }

    out << "  </workout>\n";
    out << "</workout_file>\n";

    return xml;
}

QString GCToolExecutor::sanitizeFilename(const QString& name)
{
    QString result = name;
    const QString invalid = "/\\:*?\"<>|";
    for (QChar c : invalid) {
        result.replace(c, '_');
    }
    result = result.trimmed();
    if (result.isEmpty()) result = "workout";
    return result;
}

bool GCToolExecutor::validateDate(const QString& dateStr, QString& error)
{
    QDate date = QDate::fromString(dateStr, Qt::ISODate);
    if (!date.isValid()) {
        error = QString("Invalid date format '%1'. Use YYYY-MM-DD.").arg(dateStr);
        return false;
    }
    if (date < QDate::currentDate()) {
        error = QString("Date %1 is in the past. Please use a future date.").arg(dateStr);
        return false;
    }
    return true;
}
