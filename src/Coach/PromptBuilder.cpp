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

#include "PromptBuilder.h"
#include "AthleteContext.h"

#include <QRegularExpression>
#include <QStringList>
#include <QtMath>

QString PromptBuilder::buildSystemPrompt(CoachingPhase phase, const AthleteProfile& profile)
{
    QString basePrompt = buildBaseSystemPrompt();
    QString phaseInstructions = getPhaseInstructions(phase);
    QString styleGuidelines = getCoachingStyleGuidelines();
    QString athleteBrief = formatAthleteBrief(profile);

    return QString("%1\n\n%2\n\n%3\n\n%4")
        .arg(basePrompt)
        .arg(phaseInstructions)
        .arg(styleGuidelines)
        .arg(athleteBrief);
}

QString PromptBuilder::buildAssessmentPrompt(const QString& question, 
                                             const AthleteProfile& profile)
{
    QString prompt = QString(
        "The athlete asks: \"%1\"\n\n"
        "Current athlete profile:\n%2\n\n"
        "Please provide a thorough assessment addressing their question. "
        "Ask follow-up questions to better understand their goals, constraints, "
        "and training history if needed."
    ).arg(question).arg(formatAthleteBrief(profile));

    return prompt;
}

QString PromptBuilder::buildAnalysisPrompt(const QString& focus, 
                                          const AthleteProfile& profile,
                                          const QList<RideSummary>& rides)
{
    QString rideHistory = formatRideHistory(rides, 10);
    
    QString prompt = QString(
        "Analysis focus: %1\n\n"
        "Athlete profile:\n%2\n\n"
        "Recent training history (last 10 rides):\n%3\n\n"
        "Please analyze the athlete's training patterns, identify strengths and weaknesses, "
        "and provide evidence-based insights. Focus on:\n"
        "- Training load progression (CTL, ATL, TSB)\n"
        "- Intensity distribution\n"
        "- Recovery patterns\n"
        "- Performance trends\n"
        "- Areas for improvement"
    ).arg(focus).arg(formatAthleteBrief(profile)).arg(rideHistory);

    return prompt;
}

QString PromptBuilder::buildPlanningPrompt(const QString& goal, 
                                          const AthleteProfile& profile,
                                          int weeksToEvent)
{
    QString prompt = QString(
        "Goal: %1\n"
        "Weeks to event: %2\n\n"
        "Athlete profile:\n%3\n\n"
        "Please create a periodized training plan that:\n"
        "- Builds toward the goal event\n"
        "- Respects current fitness level (CTL: %4)\n"
        "- Includes appropriate periodization phases\n"
        "- Balances intensity and volume\n"
        "- Includes recovery weeks\n"
        "- Provides specific workout recommendations\n\n"
        "Structure the plan by weeks, with clear objectives for each phase."
    ).arg(goal)
     .arg(weeksToEvent)
     .arg(formatAthleteBrief(profile))
     .arg(profile.pmc.ctl, 0, 'f', 1);

    return prompt;
}

QString PromptBuilder::buildDailyCoachingPrompt(const QString& status, 
                                               const AthleteProfile& profile,
                                               const QList<TrainingRecommendation>& recommendations)
{
    QString recsText = formatTrainingPlan(recommendations);
    
    QString prompt = QString(
        "Daily status: %1\n\n"
        "Athlete profile:\n%2\n\n"
        "Today's recommendations:\n%3\n\n"
        "Provide daily coaching guidance that:\n"
        "- Addresses the athlete's current status\n"
        "- Explains the rationale for today's training\n"
        "- Provides specific workout details if requested\n"
        "- Offers motivation and encouragement\n"
        "- Adjusts recommendations based on feedback"
    ).arg(status).arg(formatAthleteBrief(profile)).arg(recsText);

    return prompt;
}

QString PromptBuilder::summarizePMCData(const AthleteProfile& profile, 
                                       const QVector<double>& history)
{
    QString trend = formatNumericTrend(history, 4);
    
    QString summary = QString(
        "Performance Management Chart (PMC) Summary:\n"
        "- Current Fitness (CTL): %1\n"
        "- Current Fatigue (ATL): %2\n"
        "- Form (TSB): %3\n"
        "- Ramp Rate (7-day CTL change): %4\n"
        "- 90-day CTL trend: %5\n\n"
        "Interpretation:\n"
    ).arg(profile.pmc.ctl, 0, 'f', 1)
     .arg(profile.pmc.atl, 0, 'f', 1)
     .arg(profile.pmc.tsb, 0, 'f', 1)
     .arg(profile.pmc.rampRate, 0, 'f', 1)
     .arg(trend);

    // Add interpretation
    if (profile.pmc.tsb < -30) {
        summary += "- High fatigue state - recovery is priority\n";
    } else if (profile.pmc.tsb < -10) {
        summary += "- Moderate fatigue - balance training with recovery\n";
    } else if (profile.pmc.tsb > 10) {
        summary += "- Well rested - good time for hard training or racing\n";
    } else {
        summary += "- Balanced state - maintain current training approach\n";
    }

    if (profile.pmc.rampRate > 5) {
        summary += "- Training load increasing rapidly - monitor for overreaching\n";
    }

    return summary;
}

QString PromptBuilder::summarizeHRVData(const AthleteProfile& profile, 
                                       const QVector<double>& history)
{
    QString trend = formatNumericTrend(history, 4);
    
    QString summary = QString(
        "Heart Rate Variability (HRV) Summary:\n"
        "- Current RMSSD: %1 ms\n"
        "- Trend: %2\n"
        "- %3-day trend: %4\n\n"
        "Interpretation:\n"
    ).arg(profile.hrv.rmssd, 0, 'f', 1)
     .arg(profile.hrv.trend)
     .arg(profile.hrv.trendDays)
     .arg(trend);

    if (profile.hrv.trend == "improving") {
        summary += "- Positive adaptation to training\n";
        summary += "- Recovery is adequate\n";
    } else if (profile.hrv.trend == "declining") {
        summary += "- May indicate accumulated fatigue\n";
        summary += "- Consider additional recovery\n";
    } else {
        summary += "- Stable recovery status\n";
    }

    return summary;
}

QString PromptBuilder::summarizeRecentRides(const QList<RideSummary>& rides, int days)
{
    if (rides.isEmpty()) {
        return "No recent rides recorded.";
    }

    QString summary = QString("Recent %1-day training summary:\n").arg(days);
    
    int count = 0;
    double totalTime = 0.0;
    double totalDistance = 0.0;
    double totalTSS = 0.0;
    double avgIF = 0.0;
    int ifCount = 0;

    for (const RideSummary& ride : rides) {
        if (count >= days) break;
        
        totalTime += ride.duration;
        totalDistance += ride.distance;
        totalTSS += ride.tss;
        
        if (ride.intensityFactor > 0) {
            avgIF += ride.intensityFactor;
            ifCount++;
        }
        
        count++;
    }

    if (ifCount > 0) avgIF /= ifCount;

    summary += QString(
        "- Total rides: %1\n"
        "- Total time: %2 hours\n"
        "- Total distance: %3 km\n"
        "- Total TSS: %4\n"
        "- Average Intensity Factor: %5\n"
    ).arg(count)
     .arg(totalTime / 3600.0, 0, 'f', 1)
     .arg(totalDistance, 0, 'f', 1)
     .arg(totalTSS, 0, 'f', 0)
     .arg(avgIF, 0, 'f', 2);

    return summary;
}

QString PromptBuilder::summarizePerformanceTrends(const QMap<QString, QVector<double>>& trends)
{
    QString summary = "Performance trends:\n";

    for (auto it = trends.constBegin(); it != trends.constEnd(); ++it) {
        QString metric = it.key();
        const QVector<double>& values = it.value();
        
        if (values.isEmpty()) continue;

        QString trend = formatNumericTrend(values, 4);
        summary += QString("- %1: %2\n").arg(metric).arg(trend);
    }

    return summary;
}

PromptBuilder::ExtractedInfo PromptBuilder::parseAssessmentResponse(const QString& response)
{
    ExtractedInfo info;

    // Extract goal using regex patterns
    QRegularExpression goalPattern("(?:goal|target|aim)(?:\\s+is)?\\s+(?:to\\s+)?([^.!?]+)",
                                  QRegularExpression::CaseInsensitiveOption);
    QRegularExpressionMatch goalMatch = goalPattern.match(response);
    if (goalMatch.hasMatch()) {
        info.goal = goalMatch.captured(1).trimmed();
    }

    // Extract weeks/time to goal
    QRegularExpression weeksPattern("(\\d+)\\s+weeks?");
    QRegularExpressionMatch weeksMatch = weeksPattern.match(response);
    if (weeksMatch.hasMatch()) {
        info.weeksToGoal = weeksMatch.captured(1).toInt();
    }

    // Detect goal type
    QString lowerResponse = response.toLower();
    if (lowerResponse.contains("race") || lowerResponse.contains("event")) {
        info.goalType = "race";
    } else if (lowerResponse.contains("fitness") || lowerResponse.contains("health")) {
        info.goalType = "fitness";
    } else if (lowerResponse.contains("weight") || lowerResponse.contains("lose")) {
        info.goalType = "weight";
    } else if (lowerResponse.contains("volume") || lowerResponse.contains("distance")) {
        info.goalType = "volume";
    } else {
        info.goalType = "specific";
    }

    // Extract constraints
    QStringList constraintKeywords = {"limited", "busy", "work", "family", "injury", "time"};
    for (const QString& keyword : constraintKeywords) {
        if (lowerResponse.contains(keyword)) {
            info.constraints.append(keyword);
        }
    }

    // Detect experience level
    if (lowerResponse.contains("beginner") || lowerResponse.contains("new to")) {
        info.experienceLevel = "beginner";
    } else if (lowerResponse.contains("intermediate") || lowerResponse.contains("some experience")) {
        info.experienceLevel = "intermediate";
    } else if (lowerResponse.contains("advanced") || lowerResponse.contains("experienced")) {
        info.experienceLevel = "advanced";
    }

    return info;
}

QStringList PromptBuilder::getSuggestedFollowups(CoachingPhase phase)
{
    QStringList suggestions;

    switch (phase) {
    case Assessment:
        suggestions << "What are my current strengths and weaknesses?"
                   << "How fit am I right now?"
                   << "What should I focus on improving?"
                   << "Am I training too hard or not hard enough?";
        break;

    case Analysis:
        suggestions << "Analyze my training load progression"
                   << "How is my intensity distribution?"
                   << "Am I recovering adequately?"
                   << "What patterns do you see in my training?";
        break;

    case Planning:
        suggestions << "Create a 12-week training plan"
                   << "How should I periodize my training?"
                   << "What workouts should I do this week?"
                   << "Help me prepare for my upcoming event";
        break;

    case Coaching:
        suggestions << "What should I do today?"
                   << "How am I progressing toward my goal?"
                   << "Should I rest or train today?"
                   << "Give me a specific workout recommendation";
        break;
    }

    return suggestions;
}

QStringList PromptBuilder::getQuickQuestions(CoachingPhase phase)
{
    return getSuggestedFollowups(phase);
}

QString PromptBuilder::formatAthleteBrief(const AthleteProfile& profile)
{
    QString brief = QString(
        "Athlete: %1\n"
        "Sports: %2\n"
        "FTP: %3 W\n"
        "Weight: %4 kg\n"
    ).arg(profile.name.isEmpty() ? "Athlete" : profile.name)
     .arg(profile.activeSports.isEmpty() ? profile.sport : profile.activeSports.join(", "))
     .arg(profile.ftp)
     .arg(profile.weight, 0, 'f', 1);

    if (profile.ftp > 0 && profile.weight > 0)
        brief += QString("W/kg: %1\n").arg(profile.ftp / profile.weight, 0, 'f', 2);

    if (profile.maxHR > 0)
        brief += QString("Max HR: %1 bpm, Rest HR: %2 bpm\n").arg(profile.maxHR).arg(profile.restHR);

    brief += QString(
        "Current Fitness (CTL): %1\n"
        "Current Fatigue (ATL): %2\n"
        "Form (TSB): %3\n"
    ).arg(profile.pmc.ctl, 0, 'f', 1)
     .arg(profile.pmc.atl, 0, 'f', 1)
     .arg(profile.pmc.tsb, 0, 'f', 1);

    if (profile.hrv.rmssd > 0) {
        brief += QString("HRV (RMSSD): %1 ms (trend: %2)\n")
            .arg(profile.hrv.rmssd, 0, 'f', 1)
            .arg(profile.hrv.trend);
    }

    if (!profile.primaryGoal.isEmpty()) {
        brief += QString("Primary Goal: %1\n").arg(profile.primaryGoal);
        if (profile.weeksToEvent > 0) {
            brief += QString("Weeks to Event: %1\n").arg(profile.weeksToEvent);
        }
    }

    return brief;
}

QString PromptBuilder::formatRideHistory(const QList<RideSummary>& rides, int maxRides)
{
    if (rides.isEmpty()) {
        return "No recent rides recorded.";
    }

    QString history = "Recent rides:\n";

    int count = 0;
    for (const RideSummary& ride : rides) {
        if (count >= maxRides) break;

        // Base line: date, sport, duration, distance, power, TSS, IF
        QString sportTag = (!ride.sport.isEmpty() && ride.sport != "Bike")
                           ? QString(" [%1]").arg(ride.sport) : "";
        QString indoorTag = ride.isIndoor ? " (indoor)" : "";

        history += QString("%1. %2%3%4 - %5 min, %6 km")
            .arg(count + 1)
            .arg(ride.date.toString("yyyy-MM-dd"))
            .arg(sportTag)
            .arg(indoorTag)
            .arg(ride.duration / 60.0, 0, 'f', 0)
            .arg(ride.distance, 0, 'f', 1);

        if (ride.avgPower > 0)
            history += QString(", %1 W avg").arg(ride.avgPower, 0, 'f', 0);
        if (ride.avgHR > 0)
            history += QString(", %1 bpm").arg(ride.avgHR, 0, 'f', 0);
        if (ride.tss > 0)
            history += QString(", TSS: %1").arg(ride.tss, 0, 'f', 0);
        if (ride.intensityFactor > 0)
            history += QString(", IF: %1").arg(ride.intensityFactor, 0, 'f', 2);
        if (ride.avgCadence > 0)
            history += QString(", %1 rpm").arg(ride.avgCadence, 0, 'f', 0);
        history += "\n";

        // Running-specific line
        if (ride.sport == "Run" && ride.avgPace > 0) {
            history += QString("   Pace: %1 min/km").arg(ride.avgPace, 0, 'f', 2);
            if (ride.avgRunCadence > 0) history += QString(", cadence: %1 spm").arg(ride.avgRunCadence, 0, 'f', 0);
            if (ride.avgGroundContactTime > 0) history += QString(", GCT: %1 ms").arg(ride.avgGroundContactTime, 0, 'f', 0);
            if (ride.avgVerticalOscillation > 0) history += QString(", VO: %1 cm").arg(ride.avgVerticalOscillation, 0, 'f', 1);
            if (ride.avgStrideLength > 0) history += QString(", stride: %1 m").arg(ride.avgStrideLength, 0, 'f', 2);
            history += "\n";
        }

        // Swimming-specific line
        if (ride.sport == "Swim" && ride.avgSwimPace > 0) {
            history += QString("   Swim pace: %1 min/100m").arg(ride.avgSwimPace, 0, 'f', 2);
            if (ride.swolf > 0) history += QString(", SWOLF: %1").arg(ride.swolf, 0, 'f', 0);
            if (ride.avgStrokeRate > 0) history += QString(", stroke rate: %1").arg(ride.avgStrokeRate, 0, 'f', 1);
            history += "\n";
        }

        // Title, route, keywords
        if (!ride.title.isEmpty())
            history += QString("   Title: %1\n").arg(ride.title);
        if (!ride.route.isEmpty())
            history += QString("   Route: %1\n").arg(ride.route);

        // RPE and notes
        if (ride.rpe > 0)
            history += QString("   RPE: %1/10\n").arg(ride.rpe, 0, 'f', 1);
        if (!ride.description.isEmpty())
            history += QString("   Notes: %1\n").arg(ride.description);

        count++;
    }

    return history;
}

QString PromptBuilder::formatRideMetadata(const QList<RideSummary>& rides, int maxRides)
{
    // Summarize metadata patterns across rides (equipment, routes, keywords)
    QMap<QString, int> devices;
    QMap<QString, int> routes;
    QStringList allKeywords;
    int indoorCount = 0, outdoorCount = 0, commuteCount = 0;
    int withRPE = 0;
    double totalRPE = 0.0;

    int count = 0;
    for (const RideSummary& ride : rides) {
        if (count >= maxRides) break;
        count++;

        if (!ride.device.isEmpty()) devices[ride.device]++;
        if (!ride.route.isEmpty()) routes[ride.route]++;
        if (!ride.keywords.isEmpty()) {
            for (const QString& kw : ride.keywords.split(",", Qt::SkipEmptyParts))
                allKeywords.append(kw.trimmed());
        }
        if (ride.isIndoor) indoorCount++; else outdoorCount++;
        if (ride.isCommute) commuteCount++;
        if (ride.rpe > 0) { totalRPE += ride.rpe; withRPE++; }
    }

    if (count == 0) return "";

    QString result = "Activity Metadata Summary:\n";
    result += QString("- Indoor rides: %1, Outdoor rides: %2\n").arg(indoorCount).arg(outdoorCount);
    if (commuteCount > 0)
        result += QString("- Commute rides: %1\n").arg(commuteCount);

    if (withRPE > 0)
        result += QString("- Average RPE: %1/10 (from %2 rides)\n")
            .arg(totalRPE / withRPE, 0, 'f', 1).arg(withRPE);

    if (!devices.isEmpty()) {
        result += "- Devices: ";
        QStringList devList;
        for (auto it = devices.constBegin(); it != devices.constEnd(); ++it)
            devList.append(QString("%1 (%2)").arg(it.key()).arg(it.value()));
        result += devList.join(", ") + "\n";
    }

    if (!routes.isEmpty()) {
        result += "- Frequent routes: ";
        // Sort by count, show top 5
        QList<QPair<QString, int>> sorted;
        for (auto it = routes.constBegin(); it != routes.constEnd(); ++it)
            sorted.append({it.key(), it.value()});
        std::sort(sorted.begin(), sorted.end(),
                  [](const QPair<QString,int>& a, const QPair<QString,int>& b) { return a.second > b.second; });
        QStringList routeList;
        for (int i = 0; i < qMin(5, sorted.size()); i++)
            routeList.append(QString("%1 (%2x)").arg(sorted[i].first).arg(sorted[i].second));
        result += routeList.join(", ") + "\n";
    }

    if (!allKeywords.isEmpty()) {
        // Count keyword frequency
        QMap<QString, int> kwCount;
        for (const QString& kw : allKeywords) kwCount[kw]++;
        QStringList kwSummary;
        for (auto it = kwCount.constBegin(); it != kwCount.constEnd(); ++it)
            kwSummary.append(QString("%1 (%2)").arg(it.key()).arg(it.value()));
        result += "- Tags/Keywords: " + kwSummary.join(", ") + "\n";
    }

    return result;
}

QString PromptBuilder::formatKeyIntervals(const QList<IntervalSummary>& intervals)
{
    if (intervals.isEmpty()) return "";

    // Group by type and summarize
    QMap<QString, QList<const IntervalSummary*>> byType;
    for (const IntervalSummary& is : intervals)
        byType[is.type].append(&is);

    QString result = "Key Intervals from Recent Rides:\n";

    // Show performance tests first
    for (const IntervalSummary& is : intervals) {
        if (is.isTest) {
            result += QString("- TEST: \"%1\" - %2 min, %3 W avg, %4 W max, %5 bpm\n")
                .arg(is.name)
                .arg(is.duration / 60.0, 0, 'f', 1)
                .arg(is.avgPower, 0, 'f', 0)
                .arg(is.maxPower, 0, 'f', 0)
                .arg(is.avgHR, 0, 'f', 0);
        }
    }

    // Summarize efforts
    if (byType.contains("effort") && !byType["effort"].isEmpty()) {
        int count = byType["effort"].size();
        double avgPow = 0, avgDur = 0;
        for (const IntervalSummary* is : byType["effort"]) {
            avgPow += is->avgPower;
            avgDur += is->duration;
        }
        result += QString("- Sustained efforts: %1 total, avg %2 W for %3 min each\n")
            .arg(count)
            .arg(avgPow / count, 0, 'f', 0)
            .arg(avgDur / count / 60.0, 0, 'f', 1);
    }

    // Summarize climbs
    if (byType.contains("climb") && !byType["climb"].isEmpty()) {
        int count = byType["climb"].size();
        double avgPow = 0, avgDur = 0;
        for (const IntervalSummary* is : byType["climb"]) {
            avgPow += is->avgPower;
            avgDur += is->duration;
        }
        result += QString("- Climbs: %1 total, avg %2 W for %3 min each\n")
            .arg(count)
            .arg(avgPow / count, 0, 'f', 0)
            .arg(avgDur / count / 60.0, 0, 'f', 1);
    }

    // Show user-defined intervals (up to 10)
    if (byType.contains("user")) {
        int shown = 0;
        for (const IntervalSummary* is : byType["user"]) {
            if (shown >= 10) break;
            result += QString("- \"%1\" (%2): %3 min, %4 W avg")
                .arg(is->name).arg(is->type)
                .arg(is->duration / 60.0, 0, 'f', 1)
                .arg(is->avgPower, 0, 'f', 0);
            if (is->avgHR > 0) result += QString(", %1 bpm").arg(is->avgHR, 0, 'f', 0);
            result += "\n";
            shown++;
        }
    }

    return result;
}

QString PromptBuilder::formatSeasonPlan(const SeasonPlanSummary& plan)
{
    if (plan.seasonName.isEmpty()) return "";

    QDate today = QDate::currentDate();
    QString result = QString("Current Season Plan: %1 (%2 to %3)\n")
        .arg(plan.seasonName)
        .arg(plan.start.toString("yyyy-MM-dd"))
        .arg(plan.end.toString("yyyy-MM-dd"));

    if (!plan.phases.isEmpty()) {
        result += "\nTraining Phases:\n";
        for (const SeasonPlanSummary::PhaseSummary& phase : plan.phases) {
            QString status;
            if (today >= phase.start && today <= phase.end) status = " << CURRENT";
            else if (today < phase.start) status = " (upcoming)";
            else status = " (completed)";

            result += QString("- %1 [%2]: %3 to %4%5\n")
                .arg(phase.name).arg(phase.type)
                .arg(phase.start.toString("yyyy-MM-dd"))
                .arg(phase.end.toString("yyyy-MM-dd"))
                .arg(status);
        }
    }

    if (!plan.events.isEmpty()) {
        result += "\nScheduled Events:\n";
        for (const SeasonPlanSummary::EventSummary& event : plan.events) {
            int daysTo = today.daysTo(event.date);
            QString timing;
            if (daysTo > 0) timing = QString("in %1 days").arg(daysTo);
            else if (daysTo == 0) timing = "TODAY";
            else timing = QString("%1 days ago").arg(-daysTo);

            result += QString("- %1: %2 (%3)")
                .arg(event.name)
                .arg(event.date.toString("yyyy-MM-dd"))
                .arg(timing);
            if (event.priority > 0)
                result += QString(" [priority: %1]").arg(
                    event.priority == 1 ? "A" : (event.priority == 2 ? "B" : "C"));
            if (!event.description.isEmpty())
                result += QString(" - %1").arg(event.description);
            result += "\n";
        }
    }

    return result;
}

QString PromptBuilder::formatSportBreakdown(const QMap<QString, int>& breakdown)
{
    if (breakdown.isEmpty()) return "";

    int total = 0;
    for (int c : breakdown) total += c;

    QString result = "Sport Activity Breakdown (last 90 days):\n";
    for (auto it = breakdown.constBegin(); it != breakdown.constEnd(); ++it) {
        double pct = (total > 0) ? (it.value() * 100.0 / total) : 0;
        result += QString("- %1: %2 activities (%3%)\n")
            .arg(it.key()).arg(it.value()).arg(pct, 0, 'f', 0);
    }

    return result;
}

QString PromptBuilder::formatBodyComposition(
    const AthleteContextAggregator::BodyCompTrend& trend,
    const AthleteProfile& profile)
{
    if (trend.currentWeight <= 0) return "";

    QString result = "Body Composition:\n";
    result += QString("- Weight: %1 kg").arg(trend.currentWeight, 0, 'f', 1);
    if (trend.weightChange30d != 0)
        result += QString(" (30d change: %1%2 kg)")
            .arg(trend.weightChange30d > 0 ? "+" : "")
            .arg(trend.weightChange30d, 0, 'f', 1);
    result += "\n";

    if (profile.ftp > 0 && trend.currentWeight > 0)
        result += QString("- W/kg at FTP: %1\n")
            .arg(profile.ftp / trend.currentWeight, 0, 'f', 2);

    if (trend.currentFatPercent > 0) {
        result += QString("- Body fat: %1%").arg(trend.currentFatPercent, 0, 'f', 1);
        if (trend.fatPercentChange30d != 0)
            result += QString(" (30d change: %1%2%)")
                .arg(trend.fatPercentChange30d > 0 ? "+" : "")
                .arg(trend.fatPercentChange30d, 0, 'f', 1);
        result += "\n";
    }
    if (profile.body.muscleKg > 0)
        result += QString("- Muscle mass: %1 kg\n").arg(profile.body.muscleKg, 0, 'f', 1);
    if (profile.body.leanKg > 0)
        result += QString("- Lean mass: %1 kg\n").arg(profile.body.leanKg, 0, 'f', 1);

    return result;
}

QString PromptBuilder::formatDevicePerformanceEstimates(const QList<RideSummary>& rides)
{
    // Collect the most recent device-detected estimates
    double latestVO2max = 0;
    double latestATE = 0, latestAnTE = 0;
    QDate vo2maxDate, ateDate;

    for (const RideSummary& ride : rides) {
        if (ride.deviceVO2max > 0 && (vo2maxDate.isNull() || ride.date.date() > vo2maxDate)) {
            latestVO2max = ride.deviceVO2max;
            vo2maxDate = ride.date.date();
        }
        if (ride.aerobicTE > 0 && (ateDate.isNull() || ride.date.date() > ateDate)) {
            latestATE = ride.aerobicTE;
            latestAnTE = ride.anaerobicTE;
            ateDate = ride.date.date();
        }
    }

    if (latestVO2max <= 0 && latestATE <= 0) return "";

    QString result = "Device Performance Estimates:\n";
    if (latestVO2max > 0)
        result += QString("- VO2max (device estimate): %1 [%2]\n")
            .arg(latestVO2max, 0, 'f', 1)
            .arg(vo2maxDate.toString("yyyy-MM-dd"));
    if (latestATE > 0) {
        result += QString("- Aerobic Training Effect: %1").arg(latestATE, 0, 'f', 1);
        if (latestAnTE > 0)
            result += QString(", Anaerobic: %1").arg(latestAnTE, 0, 'f', 1);
        result += QString(" [%1]\n").arg(ateDate.toString("yyyy-MM-dd"));
    }

    return result;
}

QString PromptBuilder::formatTrainingPlan(const QList<TrainingRecommendation>& recommendations)
{
    if (recommendations.isEmpty()) {
        return "No specific recommendations at this time.";
    }

    QString plan = "Training recommendations:\n";
    
    int priority = 1;
    for (const TrainingRecommendation& rec : recommendations) {
        plan += QString(
            "%1. %2 (%3 min at %4% intensity)\n"
            "   Rationale: %5\n"
        ).arg(priority)
         .arg(rec.type)
         .arg(rec.suggestedDuration, 0, 'f', 0)
         .arg(rec.suggestedIntensity * 100, 0, 'f', 0)
         .arg(rec.rationale);

        priority++;
    }

    return plan;
}

QString PromptBuilder::formatPowerCurveBests(const PowerCurveBests& bests)
{
    if (bests.peak5s == 0 && bests.peak1min == 0 && bests.peak5min == 0) {
        return "";
    }

    QString result = "Power Curve - Personal Bests (last 12 months):\n";

    auto formatRow = [](const QString& label, double watts, double wkg, const QDate& date) {
        QString row = QString("- %1: %2 W").arg(label).arg(watts, 0, 'f', 0);
        if (wkg > 0) row += QString(" (%1 W/kg)").arg(wkg, 0, 'f', 2);
        if (date.isValid()) row += QString(" [%1]").arg(date.toString("yyyy-MM-dd"));
        return row + "\n";
    };

    result += formatRow("5 seconds (neuromuscular)", bests.peak5s, bests.peak5sWkg, bests.dateOf5s);
    result += formatRow("1 minute (anaerobic)", bests.peak1min, bests.peak1minWkg, bests.dateOf1min);
    result += formatRow("5 minutes (VO2max)", bests.peak5min, bests.peak5minWkg, bests.dateOf5min);
    result += formatRow("20 minutes (threshold est.)", bests.peak20min, bests.peak20minWkg, bests.dateOf20min);
    if (bests.peak60min > 0) {
        result += formatRow("60 minutes (endurance)", bests.peak60min, bests.peak60minWkg, bests.dateOf60min);
    }

    // Rider profile characterization
    if (bests.peak5min > 0 && bests.peak5s > 0) {
        double ratio = bests.peak5s / bests.peak5min;
        if (ratio > 3.5) result += "Rider profile: sprinter-type (high 5s/5min ratio)\n";
        else if (ratio < 2.5) result += "Rider profile: endurance-type (low 5s/5min ratio)\n";
        else result += "Rider profile: all-rounder\n";
    }

    return result;
}

QString PromptBuilder::formatZoneDistribution(const ZoneDistribution& dist)
{
    if (dist.powerZonePercent.isEmpty() && dist.hrZonePercent.isEmpty()) {
        return "";
    }

    QString result = "Training Intensity Distribution (last 90 days):\n";

    if (!dist.powerZonePercent.isEmpty()) {
        result += "\nPower Zones:\n";
        for (int z = 0; z < dist.powerZonePercent.size(); z++) {
            QString name = (z < dist.powerZoneNames.size()) ? dist.powerZoneNames[z] : QString("Z%1").arg(z + 1);
            result += QString("- %1: %2% (%3 hrs)\n")
                .arg(name)
                .arg(dist.powerZonePercent[z], 0, 'f', 1)
                .arg(dist.powerZoneSeconds[z] / 3600.0, 0, 'f', 1);
        }

        // Polarized training analysis
        double lowIntensity = 0, midIntensity = 0, highIntensity = 0;
        for (int z = 0; z < dist.powerZonePercent.size(); z++) {
            if (z < 2) lowIntensity += dist.powerZonePercent[z];       // Z1-Z2
            else if (z < 4) midIntensity += dist.powerZonePercent[z];  // Z3-Z4
            else highIntensity += dist.powerZonePercent[z];            // Z5+
        }
        result += QString("\nIntensity split: %1% low / %2% mid / %3% high")
            .arg(lowIntensity, 0, 'f', 0)
            .arg(midIntensity, 0, 'f', 0)
            .arg(highIntensity, 0, 'f', 0);

        if (lowIntensity > 70 && highIntensity > 15) {
            result += " (polarized - good distribution)\n";
        } else if (midIntensity > 40) {
            result += " (too much mid-zone - consider polarizing)\n";
        } else {
            result += "\n";
        }
    }

    if (!dist.hrZonePercent.isEmpty()) {
        result += "\nHeart Rate Zones:\n";
        for (int z = 0; z < dist.hrZonePercent.size(); z++) {
            QString name = (z < dist.hrZoneNames.size()) ? dist.hrZoneNames[z] : QString("Z%1").arg(z + 1);
            result += QString("- %1: %2% (%3 hrs)\n")
                .arg(name)
                .arg(dist.hrZonePercent[z], 0, 'f', 1)
                .arg(dist.hrZoneSeconds[z] / 3600.0, 0, 'f', 1);
        }
    }

    return result;
}

QString PromptBuilder::formatWeeklySummaries(const QList<WeeklyTrainingSummary>& weeks)
{
    if (weeks.isEmpty()) return "";

    QString result = "Weekly Training Progression:\n";
    result += "Week Starting | Rides | Hours | TSS | Avg IF | CTL | ATL | TSB | Type\n";
    result += "-------------|-------|-------|-----|--------|-----|-----|-----|-----\n";

    for (const WeeklyTrainingSummary& week : weeks) {
        if (week.rideCount == 0) continue;

        result += QString("%1 | %2 | %3 | %4 | %5 | %6 | %7 | %8 | %9\n")
            .arg(week.weekStart.toString("yyyy-MM-dd"))
            .arg(week.rideCount)
            .arg(week.totalDuration / 3600.0, 0, 'f', 1)
            .arg(week.totalTSS, 0, 'f', 0)
            .arg(week.avgIntensityFactor, 0, 'f', 2)
            .arg(week.endCTL, 0, 'f', 1)
            .arg(week.endATL, 0, 'f', 1)
            .arg(week.endTSB, 0, 'f', 1)
            .arg(week.dominantWorkoutType);
    }

    return result;
}

QString PromptBuilder::formatWorkoutTypeBreakdown(const QList<WorkoutClassification>& types)
{
    if (types.isEmpty()) return "";

    QString result = "Workout Type Distribution (last 90 days):\n";
    for (const WorkoutClassification& wc : types) {
        result += QString("- %1: %2 rides, %3 hrs total, avg TSS %4\n")
            .arg(wc.type)
            .arg(wc.count)
            .arg(wc.totalDuration / 3600.0, 0, 'f', 1)
            .arg(wc.avgTSS, 0, 'f', 0);
    }

    return result;
}

QString PromptBuilder::formatYearOverYearComparison(
    const AthleteContextAggregator::PeriodComparison& comp, int periodDays)
{
    QString result = QString("Year-over-Year Comparison (last %1 days vs same period last year):\n").arg(periodDays);
    result += QString("- Rides: %1 (was %2) %3\n")
        .arg(comp.ridesCurrent)
        .arg(comp.ridesPrevious)
        .arg(comp.ridesCurrent > comp.ridesPrevious ? "↑" : (comp.ridesCurrent < comp.ridesPrevious ? "↓" : "→"));
    result += QString("- Volume: %1 hrs (was %2 hrs) %3\n")
        .arg(comp.volumeCurrent / 3600.0, 0, 'f', 1)
        .arg(comp.volumePrevious / 3600.0, 0, 'f', 1)
        .arg(comp.volumeCurrent > comp.volumePrevious ? "↑" : (comp.volumeCurrent < comp.volumePrevious ? "↓" : "→"));
    result += QString("- TSS: %1 (was %2) %3\n")
        .arg(comp.tssCurrent, 0, 'f', 0)
        .arg(comp.tssPrevious, 0, 'f', 0)
        .arg(comp.tssCurrent > comp.tssPrevious ? "↑" : (comp.tssCurrent < comp.tssPrevious ? "↓" : "→"));
    result += QString("- Fitness (CTL): %1 (was %2) %3\n")
        .arg(comp.ctlCurrent, 0, 'f', 1)
        .arg(comp.ctlPrevious, 0, 'f', 1)
        .arg(comp.ctlCurrent > comp.ctlPrevious ? "↑" : (comp.ctlCurrent < comp.ctlPrevious ? "↓" : "→"));

    return result;
}

QString PromptBuilder::buildBaseSystemPrompt()
{
    return QString(
        "You are an expert endurance sports coach with deep knowledge of training physiology, "
        "periodization, and performance optimization for cycling, running, swimming, and "
        "multisport. You have comprehensive access to the athlete's complete training history "
        "from GoldenCheetah, including data imported from external devices and services "
        "(Garmin, Wahoo, Strava, TrainingPeaks, etc.) and activities recorded directly in the app.\n\n"
        "DATA AVAILABLE TO YOU:\n"
        "- Complete activity history across all sports with power, HR, pace, duration, distance, "
        "TSS, IF, cadence, speed, temperature, and elevation\n"
        "- Rich activity metadata: titles, routes, keywords/tags, workout objectives, RPE, "
        "indoor/outdoor flags, commute markers, and recording devices\n"
        "- Multi-sport breakdown showing activity distribution across disciplines\n"
        "- Power curve personal bests at key durations (5s, 1min, 5min, 20min, 60min) with W/kg\n"
        "- Training intensity distribution across power and heart rate zones\n"
        "- Week-by-week training progression with CTL/ATL/TSB trends\n"
        "- Workout type classification (endurance, tempo, threshold, VO2max, sprint, recovery)\n"
        "- Key intervals: structured efforts, performance tests, climbs, and user-defined segments\n"
        "- Season plan with training phases (prep, base, build, peak) and scheduled events\n"
        "- Year-over-year volume and fitness comparison\n"
        "- Performance Management Chart (PMC) with fitness, fatigue, and form\n"
        "- Heart Rate Variability (HRV) trends\n"
        "- Body composition data: weight, body fat %, muscle mass, and 30-day trends\n"
        "- Device performance estimates: VO2max, Aerobic/Anaerobic Training Effect, EPOC\n"
        "- Running-specific metrics: pace, cadence, ground contact time, vertical oscillation, "
        "stride length\n"
        "- Swimming-specific metrics: swim pace, SWOLF, stroke rate\n"
        "- Cycling-specific metrics: left/right power balance, aerobic decoupling, pedal dynamics\n\n"
        "Your coaching philosophy is based on:\n"
        "- Evidence-based training principles\n"
        "- Individualized approach based on athlete's data\n"
        "- Progressive overload with adequate recovery\n"
        "- Periodization for long-term development\n"
        "- Monitoring fatigue and adaptation\n"
        "- Sport-specific training when the athlete practices multiple disciplines\n\n"
        "IMPORTANT: You have the athlete's FULL training data below, including data from ALL "
        "sources (external imports and app-generated). Use specific numbers, dates, and trends "
        "from this data to support every recommendation. Reference their power curve, zone "
        "distribution, weekly progression, interval history, season plan, body composition, "
        "and fatigue state when analyzing performance or prescribing training. When the athlete "
        "practices multiple sports, provide discipline-specific insights and consider cross-training "
        "effects. Use activity metadata (route names, RPE, notes, keywords) for additional context. "
        "Compare current metrics to historical data to identify progress and areas for improvement. "
        "Be encouraging but realistic. Prioritize athlete health and long-term development over "
        "short-term gains."
    );
}

QString PromptBuilder::getPhaseInstructions(CoachingPhase phase)
{
    switch (phase) {
    case Assessment:
        return QString(
            "ASSESSMENT PHASE:\n"
            "Your role is to understand the athlete's current state, goals, and constraints. "
            "Ask clarifying questions to gather information about:\n"
            "- Training history and experience\n"
            "- Current fitness level and recent performance\n"
            "- Goals (short-term and long-term)\n"
            "- Available time and equipment\n"
            "- Any injuries or limitations\n"
            "- Motivation and preferences\n\n"
            "Provide an initial assessment of their current fitness based on available data."
        );

    case Analysis:
        return QString(
            "ANALYSIS PHASE:\n"
            "Your role is to analyze the athlete's training data and identify patterns. "
            "Focus on:\n"
            "- Training load progression (CTL, ATL, TSB trends)\n"
            "- Intensity distribution (time in zones)\n"
            "- Recovery patterns (HRV, rest days)\n"
            "- Performance trends (power, speed, endurance)\n"
            "- Strengths and weaknesses\n"
            "- Potential issues (overtraining, undertraining, imbalances)\n\n"
            "Provide evidence-based insights with specific data references."
        );

    case Planning:
        return QString(
            "PLANNING PHASE:\n"
            "Your role is to create structured training plans. Provide:\n"
            "- Periodized plan with clear phases (base, build, peak, taper)\n"
            "- Weekly structure with specific workout types\n"
            "- Progressive overload with recovery weeks\n"
            "- Specific workout prescriptions (duration, intensity, intervals)\n"
            "- Rationale for each phase and workout\n"
            "- Adjustments based on athlete's constraints\n\n"
            "Plans should be realistic, achievable, and aligned with the athlete's goals."
        );

    case Coaching:
        return QString(
            "DAILY COACHING PHASE:\n"
            "Your role is to provide day-to-day guidance. Focus on:\n"
            "- Today's workout recommendation based on current form\n"
            "- Adjustments based on fatigue, HRV, and recent training\n"
            "- Specific workout details (warm-up, intervals, cool-down)\n"
            "- Motivation and encouragement\n"
            "- Answering questions about execution\n"
            "- Feedback on completed workouts\n\n"
            "Be responsive to the athlete's current state and adjust recommendations accordingly."
        );

    default:
        return "";
    }
}

QString PromptBuilder::buildToolUseSection()
{
    return QString(
        "\n\nACTIONS YOU CAN TAKE:\n"
        "You have tools that write directly into GoldenCheetah. Use them when the athlete "
        "asks you to create or schedule something — not just describe it.\n\n"
        "create_workout — generate a structured .zwo workout and save it to the athlete's library.\n"
        "  - Use for: 'give me a workout', 'create intervals', 'make me a VO2max session', etc.\n"
        "  - Calibrate ALL power values to the athlete's current FTP shown in the data.\n"
        "  - Each interval needs: type, duration_seconds, power_low_pct.\n\n"
        "schedule_workout — add a workout to the athlete's calendar.\n"
        "  - Use future dates only (today or later).\n"
        "  - If the athlete says a date already has something, include force:true to override.\n\n"
        "create_season_event — add a race or key event to the season plan.\n"
        "  - Priority A = A-race (peak for this), B = secondary, C = training race.\n\n"
        "create_training_plan — design a multi-week plan for athlete review before it is applied.\n"
        "  - Use for: 'build me a plan', 'training program', '8-week schedule', etc.\n"
        "  - Calibrate load to CTL, ATL, TSB, and FTP from the athlete data.\n"
        "  - Respect existing season events (A-race date). Max 16 weeks.\n\n"
        "BEFORE calling any tool: write 1-2 sentences describing what you are creating and why. "
        "The athlete sees a confirmation dialog before anything is saved — explain your choices "
        "so they can make an informed decision."
    );
}

QString PromptBuilder::getCoachingStyleGuidelines()
{
    return QString(
        "COACHING STYLE GUIDELINES:\n"
        "- Be conversational and supportive\n"
        "- Use clear, jargon-free language (explain technical terms when used)\n"
        "- Reference specific data points to support recommendations\n"
        "- Acknowledge the athlete's efforts and progress\n"
        "- Be honest about challenges and realistic about timelines\n"
        "- Prioritize health and sustainability over performance\n"
        "- Encourage questions and dialogue\n"
        "- Adapt recommendations based on feedback"
    );
}

QString PromptBuilder::formatNumericTrend(const QVector<double>& data, int periods)
{
    if (data.size() < periods) {
        return "insufficient data";
    }

    int periodSize = data.size() / periods;
    QVector<double> periodAverages;

    for (int i = 0; i < periods; i++) {
        double sum = 0.0;
        int count = 0;
        int start = i * periodSize;
        int end = (i == periods - 1) ? data.size() : (i + 1) * periodSize;

        for (int j = start; j < end; j++) {
            sum += data[j];
            count++;
        }

        if (count > 0) {
            periodAverages.append(sum / count);
        }
    }

    // Calculate overall trend
    if (periodAverages.size() < 2) {
        return "stable";
    }

    double firstAvg = periodAverages.first();
    double lastAvg = periodAverages.last();
    double change = ((lastAvg - firstAvg) / firstAvg) * 100.0;

    QString trend;
    if (change > 10) {
        trend = "strongly increasing";
    } else if (change > 5) {
        trend = "increasing";
    } else if (change < -10) {
        trend = "strongly decreasing";
    } else if (change < -5) {
        trend = "decreasing";
    } else {
        trend = "stable";
    }

    // Add period values
    QString values = " (";
    for (int i = 0; i < periodAverages.size(); i++) {
        values += QString::number(periodAverages[i], 'f', 1);
        if (i < periodAverages.size() - 1) values += " → ";
    }
    values += ")";

    return trend + values;
}
