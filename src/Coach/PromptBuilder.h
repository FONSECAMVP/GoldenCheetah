/*
 * Copyright (c) 2024 GoldenCheetah Contributor
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

#ifndef _GC_PromptBuilder_h
#define _GC_PromptBuilder_h

#include "AthleteContext.h"

#include <QString>
#include <QStringList>
#include <QMap>

class PromptBuilder
{
public:
    enum CoachingPhase {
        Assessment,
        Analysis,
        Planning,
        Coaching
    };

    struct PromptTemplate {
        QString systemPrompt;
        QString userTemplate;
        QString description;
        QStringList requiredFields;
    };

    explicit PromptBuilder() = default;
    ~PromptBuilder() = default;

    // Build system prompt based on coaching phase
    QString buildSystemPrompt(CoachingPhase phase, const AthleteProfile& profile);

    // Build user prompts for different scenarios
    QString buildAssessmentPrompt(const QString& question, const AthleteProfile& profile);
    QString buildAnalysisPrompt(const QString& focus, const AthleteProfile& profile,
                                const QList<RideSummary>& rides);
    QString buildPlanningPrompt(const QString& goal, const AthleteProfile& profile,
                               int weeksToEvent);
    QString buildDailyCoachingPrompt(const QString& status, const AthleteProfile& profile,
                                     const QList<TrainingRecommendation>& recommendations);

    // Build context summaries
    QString summarizePMCData(const AthleteProfile& profile, const QVector<double>& history);
    QString summarizeHRVData(const AthleteProfile& profile, const QVector<double>& history);
    QString summarizeRecentRides(const QList<RideSummary>& rides, int days = 7);
    QString summarizePerformanceTrends(const QMap<QString, QVector<double>>& trends);

    // Extract structured information from user responses
    struct ExtractedInfo {
        QString goal;
        int weeksToGoal;
        QString goalType; // "race", "fitness", "weight", "volume", "specific"
        QStringList constraints;
        QString experienceLevel;
        QString availableTime;
        QString equipment;
        QString injuryHistory;
    };
    ExtractedInfo parseAssessmentResponse(const QString& response);

    // Get suggested follow-up questions
    QStringList getSuggestedFollowups(CoachingPhase phase);

    // Get preset quick questions
    QStringList getQuickQuestions(CoachingPhase phase);

    // Tool use guidance section (append to system prompt when provider supports tools)
    static QString buildToolUseSection();

    // Format data for LLM consumption
    static QString formatAthleteBrief(const AthleteProfile& profile);
    static QString formatRideHistory(const QList<RideSummary>& rides, int maxRides = 10);
    static QString formatTrainingPlan(const QList<TrainingRecommendation>& recommendations);
    static QString formatPowerCurveBests(const PowerCurveBests& bests);
    static QString formatZoneDistribution(const ZoneDistribution& dist);
    static QString formatWeeklySummaries(const QList<WeeklyTrainingSummary>& weeks);
    static QString formatWorkoutTypeBreakdown(const QList<WorkoutClassification>& types);
    static QString formatYearOverYearComparison(
        const AthleteContextAggregator::PeriodComparison& comp, int periodDays);

    // New format methods for enriched data
    static QString formatRideMetadata(const QList<RideSummary>& rides, int maxRides = 20);
    static QString formatKeyIntervals(const QList<IntervalSummary>& intervals);
    static QString formatSeasonPlan(const SeasonPlanSummary& plan);
    static QString formatSportBreakdown(const QMap<QString, int>& breakdown);
    static QString formatBodyComposition(const AthleteContextAggregator::BodyCompTrend& trend,
                                         const AthleteProfile& profile);
    static QString formatDevicePerformanceEstimates(const QList<RideSummary>& rides);

private:
    QString buildBaseSystemPrompt();
    QString getPhaseInstructions(CoachingPhase phase);
    QString getCoachingStyleGuidelines();
    QString formatNumericTrend(const QVector<double>& data, int periods = 4);
};

#endif // _GC_PromptBuilder_h
