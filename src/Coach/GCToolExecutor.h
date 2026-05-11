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

#ifndef _GC_GCToolExecutor_h
#define _GC_GCToolExecutor_h

#include "LLMService.h"

#include <QObject>
#include <QJsonObject>

class Context;
class QVBoxLayout;
class Seasons;

class GCToolExecutor : public QObject
{
    Q_OBJECT

public:
    explicit GCToolExecutor(Context* context, QObject* parent = nullptr);

    void registerTools(LLMService* service);
    static QList<LLMService::ToolDef> v1Tools();
    void setChatLayout(QVBoxLayout* layout);

public slots:
    void onToolCallRequested(const QString& callId, const QString& toolName, const QJsonObject& args);
    void onConfirmed(const QString& callId, const QString& toolName, const QJsonObject& args);
    void onCancelled(const QString& callId, const QString& toolName);
    void onPlanApplied(const QString& callId, const QJsonObject& plan);
    void onPlanCancelled(const QString& callId);

    // Pure functions: no Context dependency, testable in isolation
    static QString buildZwoXml(const QString& name, const QString& description, const QJsonArray& intervals);
    static QString sanitizeFilename(const QString& name);
    static bool validateDate(const QString& dateStr, QString& error);

signals:
    void toolResultReady(const QString& callId, const QString& toolName, const QJsonObject& result);
    void actionMessage(const QString& text);

private:
    QJsonObject executeCreateWorkout(const QJsonObject& args);
    QJsonObject executeScheduleWorkout(const QJsonObject& args);
    QJsonObject executeCreateSeasonEvent(const QJsonObject& args);
    void showConfirmCard(const QString& callId, const QString& toolName, const QJsonObject& args);
    void showPlanPreview(const QString& callId, const QJsonObject& plan);

    int ensureActiveSeason(Seasons* seasons, const QDate& date);

    static QJsonObject makeToolSchema(const QString& name, const QString& description,
                                      const QJsonObject& properties,
                                      const QJsonArray& required);

    Context* context_;
    LLMService* llmService_ = nullptr;
    QVBoxLayout* chatLayout_ = nullptr;
};

#endif // _GC_GCToolExecutor_h
