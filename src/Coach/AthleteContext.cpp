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

#include "AthleteContext.h"
#include "PMCData.h"
#include "Zones.h"
#include "HrZones.h"
#include "RideFileCache.h"

#include <QSettings>
#include <QDebug>
#include <QtMath>
#include <algorithm>

AthleteContextAggregator::AthleteContextAggregator(QObject* parent)
    : QObject(parent)
{
}

void AthleteContextAggregator::updateContext(Context* context)
{
    if (!context || !context->athlete) {
        qWarning() << "AthleteContextAggregator: Invalid context";
        return;
    }

    currentContext_ = context;
    Athlete* athlete = context->athlete;

    // Clear previous data
    profile_ = AthleteProfile();
    recentRides_.clear();
    pmcHistory_.clear();
    hrvHistory_.clear();

    // Basic athlete info
    profile_.name = athlete->cyclist;
    profile_.sport = "cycling"; // Default, could be extended

    // Get weight, height, and body composition from measures
    if (athlete->measures) {
        QDate today = QDate::currentDate();
        MeasuresGroup* bodyGroup = athlete->measures->getGroup(Measures::Body);
        if (bodyGroup) {
            QStringList fieldNames = bodyGroup->getFieldNames();

            auto fieldVal = [&](const QString &name, int fallback = -1) -> double {
                int idx = fieldNames.indexOf(name);
                if (idx < 0) idx = fallback;
                return (idx >= 0) ? bodyGroup->getFieldValue(today, idx) : 0.0;
            };

            profile_.weight = fieldVal("Weight", 0);
            profile_.height = fieldVal("Height", (fieldNames.size() > 1) ? 1 : -1);
            profile_.body.fatKg = fieldVal("Fat");
            profile_.body.muscleKg = fieldVal("Muscle");
            profile_.body.bonesKg = fieldVal("Bones");
            profile_.body.leanKg = fieldVal("Lean");
            profile_.body.fatPercent = fieldVal("Fat Percent");
        }
    }

    // Detect active sports from recent rides
    if (athlete->rideCache) {
        QDate cutoff = QDate::currentDate().addDays(-365);
        QSet<QString> sports;
        for (RideItem* item : athlete->rideCache->rides()) {
            if (!item || item->dateTime.date() < cutoff) continue;
            QString s = item->sport;
            if (!s.isEmpty()) sports.insert(s);
        }
        profile_.activeSports = sports.values();
        if (profile_.activeSports.isEmpty())
            profile_.activeSports << "Bike";
    }

    // Get FTP from zones
    if (athlete->zones("Bike")) {
        int zoneRange = athlete->zones("Bike")->whichRange(QDate::currentDate());
        profile_.ftp = athlete->zones("Bike")->getCP(zoneRange);
    }

    // Get HR zones
    if (athlete->hrZones("Bike")) {
        int zoneRange = athlete->hrZones("Bike")->whichRange(QDate::currentDate());
        profile_.maxHR = athlete->hrZones("Bike")->getMaxHr(zoneRange);
        profile_.restHR = athlete->hrZones("Bike")->getRestHr(zoneRange);
    }

    // Calculate PMC metrics
    calculatePMC();

    // Calculate HRV trends
    calculateHRVTrends();

    // Analyze recent performance
    analyzeRecentPerformance();

    // Detect fatigue patterns
    detectFatiguePatterns();

    // Get goals from current season
    const Season* season = context->currentSeason();
    if (season) {
        profile_.primaryGoal = season->getName();
        QDate eventDate = season->getEnd();
        profile_.eventDate = eventDate.toString(Qt::ISODate);
        profile_.weeksToEvent = QDate::currentDate().daysTo(eventDate) / 7;
    }

    emit profileUpdated(profile_);
}

QList<RideSummary> AthleteContextAggregator::getRecentRides(int days) const
{
    if (!currentContext_ || !currentContext_->athlete) {
        return QList<RideSummary>();
    }

    QList<RideSummary> rides;
    RideCache* cache = currentContext_->athlete->rideCache;
    if (!cache) return rides;

    QDate cutoffDate = QDate::currentDate().addDays(-days);

    for (RideItem* item : cache->rides()) {
        if (!item) continue;

        QDate rideDate = item->dateTime.date();
        if (rideDate < cutoffDate) continue;

        RideSummary summary;
        summary.date = item->dateTime;
        summary.sport = item->sport;
        summary.duration = item->getForSymbol("workout_time");
        summary.distance = item->getForSymbol("total_distance");
        summary.avgPower = item->getForSymbol("average_power");
        summary.maxPower = item->getForSymbol("max_power");
        summary.avgHR = item->getForSymbol("average_hr");
        summary.tss = item->getForSymbol("coggan_tss");
        summary.intensityFactor = item->getForSymbol("coggan_if");
        summary.description = item->getText("Notes", "");

        // Store additional metrics
        summary.metrics["np"] = item->getForSymbol("coggan_np");
        summary.metrics["vi"] = item->getForSymbol("variability_index");
        summary.metrics["work"] = item->getForSymbol("total_work");
        summary.metrics["elevation"] = item->getForSymbol("elevation_gain");

        // Rich metadata from ride tags (external sources & user input)
        summary.title = item->getText("Workout Title", "");
        summary.keywords = item->getText("Keywords", "");
        summary.objective = item->getText("Objective", "");
        summary.device = item->getText("Device", "");
        summary.route = item->getText("Route", "");
        summary.workoutCode = item->getText("Workout Code", "");
        summary.isIndoor = (item->getText("Trainer", "") == "1");
        summary.isCommute = (item->getText("Commute", "") == "1");

        // RPE from metadata
        QString rpeStr = item->getText("RPE", "");
        if (!rpeStr.isEmpty()) summary.rpe = rpeStr.toDouble();

        // Device-detected performance estimates (Garmin, Wahoo, etc.)
        QString vo2Str = item->getText("VO2max detected", "");
        if (!vo2Str.isEmpty()) summary.deviceVO2max = vo2Str.toDouble();
        QString ateStr = item->getText("Aerobic Training Effect", "");
        if (!ateStr.isEmpty()) summary.aerobicTE = ateStr.toDouble();
        QString anteStr = item->getText("Anaerobic Training Effect", "");
        if (!anteStr.isEmpty()) summary.anaerobicTE = anteStr.toDouble();
        QString recStr = item->getText("Recovery Time", "");
        if (!recStr.isEmpty()) summary.recoveryTime = recStr.toDouble();
        QString epocStr = item->getText("EPOC", "");
        if (!epocStr.isEmpty()) summary.epoc = epocStr.toDouble();

        // Additional cycling metrics
        summary.avgCadence = item->getForSymbol("average_cad");
        summary.avgSpeed = item->getForSymbol("average_speed");
        summary.avgTemp = item->getForSymbol("average_temp");
        summary.leftRightBalance = item->getForSymbol("left_right_balance");
        summary.aerobicDecoupling = item->getForSymbol("aerobic_decoupling");

        // Running-specific metrics
        if (summary.sport == "Run") {
            summary.avgPace = item->getForSymbol("pace");
            summary.avgRunCadence = item->getForSymbol("average_run_cad");
            summary.avgGroundContactTime = item->getForSymbol("average_run_ground_contact");
            summary.avgVerticalOscillation = item->getForSymbol("average_run_vert_oscillation");
            summary.avgStrideLength = item->getForSymbol("average_stride_length");
        }

        // Swimming-specific metrics
        if (summary.sport == "Swim") {
            summary.avgSwimPace = item->getForSymbol("pace_swim");
            summary.avgStrokeRate = item->getForSymbol("stroke_rate");
            summary.swolf = item->getForSymbol("swolf");
        }

        rides.append(summary);
    }

    // Sort by date descending
    std::sort(rides.begin(), rides.end(), [](const RideSummary& a, const RideSummary& b) {
        return a.date > b.date;
    });

    return rides;
}

QVector<double> AthleteContextAggregator::getPMCHistory(int days) const
{
    if (!currentContext_ || !currentContext_->athlete) {
        return QVector<double>();
    }

    QVector<double> history;
    PMCData* pmc = currentContext_->athlete->getPMCFor("BikeStress");
    if (!pmc) return history;

    QDate startDate = QDate::currentDate().addDays(-days);
    QDate endDate = QDate::currentDate();

    for (QDate date = startDate; date <= endDate; date = date.addDays(1)) {
        double ctl = pmc->lts(date);
        history.append(ctl);
    }

    return history;
}

QVector<double> AthleteContextAggregator::getHRVHistory(int days) const
{
    if (!currentContext_ || !currentContext_->athlete || !currentContext_->athlete->measures) {
        return QVector<double>();
    }

    QVector<double> history;
    Measures* measures = currentContext_->athlete->measures;
    QDate startDate = QDate::currentDate().addDays(-days);
    QDate endDate = QDate::currentDate();

    // Get HRV measures group
    MeasuresGroup* hrvGroup = measures->getGroup(Measures::Hrv);
    if (!hrvGroup) return history;

    // Find RMSSD field index
    QStringList fieldNames = hrvGroup->getFieldNames();
    int rmssdFieldIndex = fieldNames.indexOf("RMSSD");
    if (rmssdFieldIndex < 0) return history;

    for (QDate date = startDate; date <= endDate; date = date.addDays(1)) {
        double rmssd = hrvGroup->getFieldValue(date, rmssdFieldIndex);
        if (rmssd > 0) {
            history.append(rmssd);
        }
    }

    return history;
}

QMap<QString, QVector<double>> AthleteContextAggregator::getMetricTrends(
    const QStringList& metrics, int days) const
{
    QMap<QString, QVector<double>> trends;
    
    if (!currentContext_ || !currentContext_->athlete) {
        return trends;
    }

    RideCache* cache = currentContext_->athlete->rideCache;
    if (!cache) return trends;

    QDate cutoffDate = QDate::currentDate().addDays(-days);

    // Initialize vectors for each metric
    for (const QString& metric : metrics) {
        trends[metric] = QVector<double>();
    }

    // Collect data
    for (RideItem* item : cache->rides()) {
        if (!item) continue;
        if (item->dateTime.date() < cutoffDate) continue;

        for (const QString& metric : metrics) {
            double value = item->getForSymbol(metric);
            trends[metric].append(value);
        }
    }

    return trends;
}

AthleteContextAggregator::PatternAnalysis AthleteContextAggregator::analyzePatterns() const
{
    return patterns_;
}

QList<TrainingRecommendation> 
AthleteContextAggregator::getDailyRecommendations() const
{
    QList<TrainingRecommendation> recommendations;

    if (!currentContext_ || !currentContext_->athlete) {
        return recommendations;
    }

    // Analyze TSB for recovery needs
    double tsb = profile_.pmc.tsb;
    double ctl = profile_.pmc.ctl;
    double atl = profile_.pmc.atl;

    // Recovery recommendation
    if (tsb < -30) {
        TrainingRecommendation rec;
        rec.type = "recovery";
        rec.rationale = "Your Training Stress Balance is very negative, indicating high fatigue. "
                       "Focus on recovery to prevent overtraining.";
        rec.suggestedDuration = 60.0; // minutes
        rec.suggestedIntensity = 0.5; // 50% of FTP
        rec.priority = 1;
        recommendations.append(rec);
    } else if (tsb < -10) {
        TrainingRecommendation rec;
        rec.type = "endurance";
        rec.rationale = "Moderate fatigue detected. Easy endurance work will help maintain fitness "
                       "while allowing recovery.";
        rec.suggestedDuration = 90.0;
        rec.suggestedIntensity = 0.65;
        rec.priority = 2;
        recommendations.append(rec);
    } else if (tsb > 10) {
        // Well rested - can handle intensity
        TrainingRecommendation rec;
        rec.type = "threshold";
        rec.rationale = "You're well rested. This is a good time for high-intensity threshold work.";
        rec.suggestedDuration = 75.0;
        rec.suggestedIntensity = 0.95;
        rec.priority = 1;
        recommendations.append(rec);
    } else {
        // Balanced state
        TrainingRecommendation rec;
        rec.type = "endurance";
        rec.rationale = "Your training load is balanced. Continue with steady endurance work.";
        rec.suggestedDuration = 120.0;
        rec.suggestedIntensity = 0.70;
        rec.priority = 2;
        recommendations.append(rec);
    }

    // Check HRV trends
    if (profile_.hrv.trend == "declining") {
        TrainingRecommendation rec;
        rec.type = "rest day";
        rec.rationale = "Your HRV has been declining, suggesting accumulated fatigue. "
                       "Consider a rest day or very light activity.";
        rec.suggestedDuration = 30.0;
        rec.suggestedIntensity = 0.4;
        rec.priority = 1;
        recommendations.append(rec);
    }

    // Sort by priority
    std::sort(recommendations.begin(), recommendations.end(), 
        [](const TrainingRecommendation& a, const TrainingRecommendation& b) {
            return a.priority < b.priority;
        });

    return recommendations;
}

QStringList AthleteContextAggregator::getPersonalizedInsights() const
{
    QStringList insights;

    // CTL insights
    if (profile_.pmc.ctl > 100) {
        insights.append("Your fitness (CTL) is very high at " + 
                       QString::number(profile_.pmc.ctl, 'f', 1) + 
                       ". You're in excellent shape!");
    } else if (profile_.pmc.ctl < 40) {
        insights.append("Your fitness (CTL) is relatively low. "
                       "Consistent training will help build your base.");
    }

    // TSB insights
    if (profile_.pmc.tsb < -30) {
        insights.append("High fatigue detected. Prioritize recovery to avoid overtraining.");
    } else if (profile_.pmc.tsb > 20) {
        insights.append("You're well rested and ready for hard training or racing.");
    }

    // Ramp rate insights
    if (profile_.pmc.rampRate > 5) {
        insights.append("Your training load is increasing rapidly. "
                       "Monitor for signs of overreaching.");
    }

    // HRV insights
    if (profile_.hrv.trend == "improving") {
        insights.append("Your HRV trend is positive, indicating good adaptation to training.");
    } else if (profile_.hrv.trend == "declining") {
        insights.append("Declining HRV suggests you may need more recovery.");
    }

    // Performance insights
    if (profile_.performance.totalRides > 0) {
        double avgTSS = profile_.pmc.atl;
        if (avgTSS > 100) {
            insights.append("You're maintaining a high training volume. Great consistency!");
        }
    }

    return insights;
}

void AthleteContextAggregator::saveProfile()
{
    if (!currentContext_) return;

    QSettings settings;
    settings.beginGroup("coach/profile");
    settings.setValue("lastUpdate", QDateTime::currentDateTime());
    settings.setValue("ftp", profile_.ftp);
    settings.setValue("weight", profile_.weight);
    settings.endGroup();
}

void AthleteContextAggregator::loadProfile()
{
    QSettings settings;
    settings.beginGroup("coach/profile");
    // Load cached profile data if needed
    settings.endGroup();
}

void AthleteContextAggregator::calculatePMC()
{
    if (!currentContext_ || !currentContext_->athlete) return;

    PMCData* pmc = currentContext_->athlete->getPMCFor("BikeStress");
    if (!pmc) return;

    QDate today = QDate::currentDate();
    
    profile_.pmc.ctl = pmc->lts(today);
    profile_.pmc.atl = pmc->sts(today);
    profile_.pmc.tsb = pmc->sb(today);

    // Calculate ramp rate (CTL change over last 7 days)
    QDate weekAgo = today.addDays(-7);
    double ctlWeekAgo = pmc->lts(weekAgo);
    profile_.pmc.rampRate = profile_.pmc.ctl - ctlWeekAgo;

    // Store history
    pmcHistory_ = getPMCHistory(90);
}

void AthleteContextAggregator::calculateHRVTrends()
{
    if (!currentContext_ || !currentContext_->athlete || !currentContext_->athlete->measures) {
        return;
    }

    Measures* measures = currentContext_->athlete->measures;
    QDate today = QDate::currentDate();

    // Get HRV measures group
    MeasuresGroup* hrvGroup = measures->getGroup(Measures::Hrv);
    if (hrvGroup) {
        QStringList fieldNames = hrvGroup->getFieldNames();
        
        // Get RMSSD
        int rmssdFieldIndex = fieldNames.indexOf("RMSSD");
        if (rmssdFieldIndex >= 0) {
            profile_.hrv.rmssd = hrvGroup->getFieldValue(today, rmssdFieldIndex);
        }
        
        // Get SDNN
        int sdnnFieldIndex = fieldNames.indexOf("SDNN");
        if (sdnnFieldIndex >= 0) {
            profile_.hrv.sdnn = hrvGroup->getFieldValue(today, sdnnFieldIndex);
        }
    }

    // Get HRV history
    hrvHistory_ = getHRVHistory(30);

    if (hrvHistory_.size() < 7) {
        profile_.hrv.trend = "insufficient data";
        return;
    }

    // Calculate trend (compare recent 7 days to previous 7 days)
    int midpoint = hrvHistory_.size() / 2;
    double recentAvg = 0.0;
    double previousAvg = 0.0;
    int recentCount = 0;
    int previousCount = 0;

    for (int i = midpoint; i < hrvHistory_.size(); i++) {
        recentAvg += hrvHistory_[i];
        recentCount++;
    }
    for (int i = 0; i < midpoint; i++) {
        previousAvg += hrvHistory_[i];
        previousCount++;
    }

    if (recentCount > 0) recentAvg /= recentCount;
    if (previousCount > 0) previousAvg /= previousCount;

    double change = ((recentAvg - previousAvg) / previousAvg) * 100.0;

    if (change > 5.0) {
        profile_.hrv.trend = "improving";
    } else if (change < -5.0) {
        profile_.hrv.trend = "declining";
    } else {
        profile_.hrv.trend = "stable";
    }

    profile_.hrv.trendDays = hrvHistory_.size();
}

void AthleteContextAggregator::analyzeRecentPerformance()
{
    // Use adaptive lookback: try 90 days first, expand if no rides found
    recentRides_ = getRecentRides(90);
    if (recentRides_.isEmpty()) {
        recentRides_ = getRecentRides(365);
    }
    if (recentRides_.isEmpty()) {
        recentRides_ = getRecentRides(365 * 7);
    }

    if (recentRides_.isEmpty()) {
        return;
    }

    profile_.performance.totalRides = recentRides_.size();
    
    double totalTime = 0.0;
    double totalDistance = 0.0;
    double totalWork = 0.0;
    double totalPower = 0.0;
    double totalNP = 0.0;
    int powerCount = 0;

    for (const RideSummary& ride : recentRides_) {
        totalTime += ride.duration;
        totalDistance += ride.distance;
        totalWork += ride.metrics.value("work", 0.0);
        
        if (ride.avgPower > 0) {
            totalPower += ride.avgPower;
            powerCount++;
        }
        
        totalNP += ride.metrics.value("np", 0.0);
    }

    profile_.performance.totalTime = totalTime;
    profile_.performance.totalDistance = totalDistance;
    profile_.performance.totalWork = totalWork;
    
    if (powerCount > 0) {
        profile_.performance.avgPower = totalPower / powerCount;
        profile_.performance.normalizedPower = totalNP / powerCount;
        
        if (profile_.ftp > 0) {
            profile_.performance.intensityFactor = 
                profile_.performance.normalizedPower / profile_.ftp;
        }
    }
}

void AthleteContextAggregator::detectFatiguePatterns()
{
    patterns_ = PatternAnalysis();

    // Analyze strengths
    if (profile_.pmc.ctl > 80) {
        patterns_.strengths.append("High fitness level (CTL > 80)");
    }
    if (profile_.performance.totalRides > 50) {
        patterns_.strengths.append("Excellent training consistency");
    }
    if (profile_.hrv.trend == "improving") {
        patterns_.strengths.append("Positive recovery adaptation");
    }

    // Analyze weaknesses
    if (profile_.pmc.ctl < 40) {
        patterns_.weaknesses.append("Low fitness base - needs consistent training");
    }
    if (profile_.performance.totalRides < 10) {
        patterns_.weaknesses.append("Low training frequency");
    }

    // Fatigue indicators
    if (profile_.pmc.tsb < -30) {
        patterns_.fatigueIndicators.append("Very high fatigue (TSB < -30)");
    }
    if (profile_.hrv.trend == "declining") {
        patterns_.fatigueIndicators.append("Declining HRV trend");
    }
    if (profile_.pmc.rampRate > 8) {
        patterns_.fatigueIndicators.append("Rapid training load increase");
    }

    // Improvement areas
    if (profile_.performance.intensityFactor < 0.75) {
        patterns_.improvementAreas.append("Consider adding more intensity to training");
    }
    if (profile_.pmc.ctl < 60 && profile_.weeksToEvent < 12) {
        patterns_.improvementAreas.append("Build fitness base before event");
    }

    // Overall assessment
    if (patterns_.fatigueIndicators.size() > 2) {
        patterns_.overallAssessment = "High fatigue - prioritize recovery";
    } else if (profile_.pmc.tsb > 10 && profile_.pmc.ctl > 60) {
        patterns_.overallAssessment = "Well rested and fit - ready for hard training or racing";
    } else if (profile_.pmc.ctl > 70) {
        patterns_.overallAssessment = "Good fitness - maintain consistency";
    } else {
        patterns_.overallAssessment = "Building fitness - stay consistent";
    }
}

QString AthleteContextAggregator::classifyWorkoutType(const RideSummary& ride) const
{
    if (profile_.ftp <= 0) return "unknown";

    double ifValue = ride.intensityFactor;
    double duration = ride.duration;

    // Classification based on IF and duration heuristics
    if (ifValue < 0.55) return "recovery";
    if (ifValue < 0.75) return "endurance";
    if (ifValue < 0.85) return "tempo";
    if (ifValue < 0.95) return "threshold";
    if (ifValue < 1.05) {
        // Could be VO2max intervals or sustained threshold
        if (duration < 3600) return "vo2max";
        return "threshold";
    }
    if (ifValue >= 1.05) {
        if (duration < 1800) return "sprint";
        return "race";
    }

    return "endurance";
}

PowerCurveBests AthleteContextAggregator::getPowerCurveBests(int days) const
{
    PowerCurveBests bests;

    if (!currentContext_ || !currentContext_->athlete) return bests;

    QDate from = QDate::currentDate().addDays(-days);
    QDate to = QDate::currentDate();

    // Use RideFileCache to get aggregated mean-max power across the date range
    QVector<float> wpk;
    QVector<QDate> dates;
    QVector<float> meanMax = RideFileCache::meanMaxPowerFor(
        currentContext_, wpk, from, to, &dates, "Bike");

    // Extract bests at key durations (index = seconds)
    // meanMax[0] is unused, meanMax[1] = 1s best, meanMax[5] = 5s best, etc.
    auto extractBest = [&](int seconds) -> double {
        if (seconds < meanMax.size()) return static_cast<double>(meanMax[seconds]);
        return 0.0;
    };
    auto extractWpk = [&](int seconds) -> double {
        if (seconds < wpk.size()) return static_cast<double>(wpk[seconds]);
        return 0.0;
    };
    auto extractDate = [&](int seconds) -> QDate {
        if (seconds < dates.size()) return dates[seconds];
        return QDate();
    };

    bests.peak5s = extractBest(5);
    bests.peak1min = extractBest(60);
    bests.peak5min = extractBest(300);
    bests.peak20min = extractBest(1200);
    bests.peak60min = extractBest(3600);

    bests.peak5sWkg = extractWpk(5);
    bests.peak1minWkg = extractWpk(60);
    bests.peak5minWkg = extractWpk(300);
    bests.peak20minWkg = extractWpk(1200);
    bests.peak60minWkg = extractWpk(3600);

    bests.dateOf5s = extractDate(5);
    bests.dateOf1min = extractDate(60);
    bests.dateOf5min = extractDate(300);
    bests.dateOf20min = extractDate(1200);
    bests.dateOf60min = extractDate(3600);

    return bests;
}

ZoneDistribution AthleteContextAggregator::getZoneDistribution(int days) const
{
    ZoneDistribution dist;

    if (!currentContext_ || !currentContext_->athlete) return dist;

    RideCache* cache = currentContext_->athlete->rideCache;
    if (!cache) return dist;

    QDate cutoffDate = QDate::currentDate().addDays(-days);

    // Get zone names from configured zones
    const Zones* zones = currentContext_->athlete->zones("Bike");
    const HrZones* hrZones = currentContext_->athlete->hrZones("Bike");

    int zoneRange = -1;
    int hrZoneRange = -1;
    int numPowerZones = 0;
    int numHrZones = 0;

    if (zones) {
        zoneRange = zones->whichRange(QDate::currentDate());
        if (zoneRange >= 0) {
            dist.powerZoneNames = zones->getZoneNames(zoneRange);
            numPowerZones = dist.powerZoneNames.size();
            dist.powerZoneSeconds.resize(numPowerZones);
            dist.powerZoneSeconds.fill(0.0);
        }
    }

    if (hrZones) {
        hrZoneRange = hrZones->whichRange(QDate::currentDate());
        if (hrZoneRange >= 0) {
            dist.hrZoneNames = hrZones->getZoneNames(hrZoneRange);
            numHrZones = dist.hrZoneNames.size();
            dist.hrZoneSeconds.resize(numHrZones);
            dist.hrZoneSeconds.fill(0.0);
        }
    }

    // Accumulate time-in-zone from each ride using metric symbols
    for (RideItem* item : cache->rides()) {
        if (!item) continue;
        if (item->dateTime.date() < cutoffDate) continue;

        // Power zones: time_in_zone_L1 .. time_in_zone_L10
        for (int z = 0; z < numPowerZones && z < 10; z++) {
            QString symbol = QString("time_in_zone_L%1").arg(z + 1);
            dist.powerZoneSeconds[z] += item->getForSymbol(symbol);
        }

        // HR zones: time_in_zone_H1 .. time_in_zone_H8
        for (int z = 0; z < numHrZones && z < 8; z++) {
            QString symbol = QString("time_in_zone_H%1").arg(z + 1);
            dist.hrZoneSeconds[z] += item->getForSymbol(symbol);
        }
    }

    // Calculate percentages
    double totalPowerTime = 0.0;
    for (double s : dist.powerZoneSeconds) totalPowerTime += s;

    dist.powerZonePercent.resize(numPowerZones);
    for (int z = 0; z < numPowerZones; z++) {
        dist.powerZonePercent[z] = (totalPowerTime > 0)
            ? (dist.powerZoneSeconds[z] / totalPowerTime) * 100.0
            : 0.0;
    }

    double totalHrTime = 0.0;
    for (double s : dist.hrZoneSeconds) totalHrTime += s;

    dist.hrZonePercent.resize(numHrZones);
    for (int z = 0; z < numHrZones; z++) {
        dist.hrZonePercent[z] = (totalHrTime > 0)
            ? (dist.hrZoneSeconds[z] / totalHrTime) * 100.0
            : 0.0;
    }

    return dist;
}

QList<WeeklyTrainingSummary> AthleteContextAggregator::getWeeklySummaries(int weeks) const
{
    QList<WeeklyTrainingSummary> summaries;

    if (!currentContext_ || !currentContext_->athlete) return summaries;

    RideCache* cache = currentContext_->athlete->rideCache;
    if (!cache) return summaries;

    PMCData* pmc = currentContext_->athlete->getPMCFor("BikeStress");

    QDate today = QDate::currentDate();
    // Start from the most recent Monday
    QDate currentMonday = today.addDays(-(today.dayOfWeek() - 1));

    for (int w = 0; w < weeks; w++) {
        QDate weekStart = currentMonday.addDays(-7 * w);
        QDate weekEnd = weekStart.addDays(6);

        WeeklyTrainingSummary summary;
        summary.weekStart = weekStart;

        double totalIF = 0.0;
        int ifCount = 0;
        QMap<QString, int> typeCounts;

        for (RideItem* item : cache->rides()) {
            if (!item) continue;
            QDate rideDate = item->dateTime.date();
            if (rideDate < weekStart || rideDate > weekEnd) continue;

            summary.rideCount++;
            summary.totalDuration += item->getForSymbol("workout_time");
            summary.totalDistance += item->getForSymbol("total_distance");
            summary.totalTSS += item->getForSymbol("coggan_tss");
            summary.totalWork += item->getForSymbol("total_work");
            summary.totalElevation += item->getForSymbol("elevation_gain");

            double avgP = item->getForSymbol("average_power");
            double maxP = item->getForSymbol("max_power");
            summary.avgPower += avgP;
            if (maxP > summary.maxPower) summary.maxPower = maxP;

            double ifVal = item->getForSymbol("coggan_if");
            if (ifVal > 0) {
                totalIF += ifVal;
                ifCount++;
            }

            // Classify this ride
            RideSummary rs;
            rs.intensityFactor = ifVal;
            rs.duration = item->getForSymbol("workout_time");
            QString type = classifyWorkoutType(rs);
            typeCounts[type]++;
        }

        if (summary.rideCount > 0) {
            summary.avgPower /= summary.rideCount;
        }
        if (ifCount > 0) {
            summary.avgIntensityFactor = totalIF / ifCount;
        }

        // Find dominant workout type
        int maxCount = 0;
        for (auto it = typeCounts.constBegin(); it != typeCounts.constEnd(); ++it) {
            if (it.value() > maxCount) {
                maxCount = it.value();
                summary.dominantWorkoutType = it.key();
            }
        }

        // Get end-of-week PMC values
        if (pmc) {
            QDate pmcDate = (weekEnd <= today) ? weekEnd : today;
            summary.endCTL = pmc->lts(pmcDate);
            summary.endATL = pmc->sts(pmcDate);
            summary.endTSB = pmc->sb(pmcDate);
        }

        summaries.append(summary);
    }

    return summaries;
}

QList<WorkoutClassification> AthleteContextAggregator::getWorkoutTypeBreakdown(int days) const
{
    QMap<QString, WorkoutClassification> classMap;

    QList<RideSummary> rides = getRecentRides(days);
    for (const RideSummary& ride : rides) {
        QString type = classifyWorkoutType(ride);
        WorkoutClassification& wc = classMap[type];
        wc.type = type;
        wc.count++;
        wc.totalDuration += ride.duration;
        wc.avgTSS += ride.tss;
    }

    QList<WorkoutClassification> result;
    for (auto it = classMap.constBegin(); it != classMap.constEnd(); ++it) {
        WorkoutClassification wc = it.value();
        if (wc.count > 0) wc.avgTSS /= wc.count;
        result.append(wc);
    }

    // Sort by count descending
    std::sort(result.begin(), result.end(),
        [](const WorkoutClassification& a, const WorkoutClassification& b) {
            return a.count > b.count;
        });

    return result;
}

AthleteContextAggregator::PeriodComparison
AthleteContextAggregator::getYearOverYearComparison(int periodDays) const
{
    PeriodComparison comp;

    if (!currentContext_ || !currentContext_->athlete) return comp;

    PMCData* pmc = currentContext_->athlete->getPMCFor("BikeStress");
    QDate today = QDate::currentDate();

    // Current period
    QList<RideSummary> currentRides = getRecentRides(periodDays);
    for (const RideSummary& ride : currentRides) {
        comp.volumeCurrent += ride.duration;
        comp.tssCurrent += ride.tss;
        comp.ridesCurrent++;
    }
    if (pmc) comp.ctlCurrent = pmc->lts(today);

    // Same period last year
    QDate lastYearEnd = today.addYears(-1);
    QDate lastYearStart = lastYearEnd.addDays(-periodDays);

    RideCache* cache = currentContext_->athlete->rideCache;
    if (!cache) return comp;

    for (RideItem* item : cache->rides()) {
        if (!item) continue;
        QDate rideDate = item->dateTime.date();
        if (rideDate < lastYearStart || rideDate > lastYearEnd) continue;

        comp.volumePrevious += item->getForSymbol("workout_time");
        comp.tssPrevious += item->getForSymbol("coggan_tss");
        comp.ridesPrevious++;
    }
    if (pmc) comp.ctlPrevious = pmc->lts(lastYearEnd);

    return comp;
}

QList<IntervalSummary> AthleteContextAggregator::getKeyIntervals(int days) const
{
    QList<IntervalSummary> result;

    if (!currentContext_ || !currentContext_->athlete) return result;

    RideCache* cache = currentContext_->athlete->rideCache;
    if (!cache) return result;

    QDate cutoffDate = QDate::currentDate().addDays(-days);

    for (RideItem* item : cache->rides()) {
        if (!item) continue;
        if (item->dateTime.date() < cutoffDate) continue;

        for (IntervalItem* interval : item->intervals()) {
            if (!interval) continue;

            // Only include meaningful intervals: user-created, device laps,
            // performance tests, efforts, and climbs
            RideFileInterval::IntervalType itype = interval->type;
            if (itype != RideFileInterval::USER &&
                itype != RideFileInterval::DEVICE &&
                itype != RideFileInterval::EFFORT &&
                itype != RideFileInterval::CLIMB)
                continue;

            IntervalSummary is;
            is.name = interval->name;
            is.startSecs = interval->start;
            is.stopSecs = interval->stop;
            is.duration = interval->stop - interval->start;
            is.avgPower = interval->getForSymbol("average_power");
            is.maxPower = interval->getForSymbol("max_power");
            is.avgHR = interval->getForSymbol("average_hr");
            is.distance = interval->getForSymbol("total_distance");
            is.avgCadence = interval->getForSymbol("average_cad");
            is.isTest = interval->istest();

            switch (itype) {
                case RideFileInterval::USER:   is.type = "user"; break;
                case RideFileInterval::DEVICE: is.type = "device"; break;
                case RideFileInterval::EFFORT: is.type = "effort"; break;
                case RideFileInterval::CLIMB:  is.type = "climb"; break;
                default: is.type = "other"; break;
            }

            result.append(is);
        }
    }

    return result;
}

SeasonPlanSummary AthleteContextAggregator::getSeasonPlan() const
{
    SeasonPlanSummary plan;

    if (!currentContext_ || !currentContext_->athlete) return plan;

    Seasons* seasons = currentContext_->athlete->seasons;
    if (!seasons) return plan;

    QDate today = QDate::currentDate();

    // Find the current active season (the one containing today)
    for (const Season& season : seasons->seasons) {
        QDate start = season.getStart();
        QDate end = season.getEnd();

        if (start <= today && end >= today) {
            plan.seasonName = season.getName();
            plan.start = start;
            plan.end = end;

            // Extract phases
            for (const Phase& phase : season.phases) {
                SeasonPlanSummary::PhaseSummary ps;
                ps.name = phase.getName();
                ps.start = phase.getStart();
                ps.end = phase.getEnd();

                switch (phase.getType()) {
                    case Phase::prep:  ps.type = "prep"; break;
                    case Phase::base:  ps.type = "base"; break;
                    case Phase::build: ps.type = "build"; break;
                    case Phase::peak:  ps.type = "peak"; break;
                    case Phase::camp:  ps.type = "camp"; break;
                    default:           ps.type = "phase"; break;
                }
                plan.phases.append(ps);
            }

            // Extract events
            for (const SeasonEvent& event : season.events) {
                SeasonPlanSummary::EventSummary es;
                es.name = event.name;
                es.date = event.date;
                es.priority = event.priority;
                es.description = event.description;
                plan.events.append(es);
            }

            break; // use the first matching season
        }
    }

    return plan;
}

QMap<QString, int> AthleteContextAggregator::getSportBreakdown(int days) const
{
    QMap<QString, int> breakdown;

    if (!currentContext_ || !currentContext_->athlete) return breakdown;

    RideCache* cache = currentContext_->athlete->rideCache;
    if (!cache) return breakdown;

    QDate cutoffDate = QDate::currentDate().addDays(-days);

    for (RideItem* item : cache->rides()) {
        if (!item) continue;
        if (item->dateTime.date() < cutoffDate) continue;

        QString sport = item->sport;
        if (sport.isEmpty()) sport = "Bike";
        breakdown[sport]++;
    }

    return breakdown;
}

AthleteContextAggregator::BodyCompTrend
AthleteContextAggregator::getBodyCompositionTrend() const
{
    BodyCompTrend trend;

    if (!currentContext_ || !currentContext_->athlete || !currentContext_->athlete->measures)
        return trend;

    MeasuresGroup* bodyGroup = currentContext_->athlete->measures->getGroup(Measures::Body);
    if (!bodyGroup) return trend;

    QStringList fieldNames = bodyGroup->getFieldNames();
    QDate today = QDate::currentDate();
    QDate thirtyDaysAgo = today.addDays(-30);

    int weightIdx = fieldNames.indexOf("Weight");
    if (weightIdx < 0) weightIdx = 0;
    int fatPctIdx = fieldNames.indexOf("Fat Percent");

    trend.currentWeight = bodyGroup->getFieldValue(today, weightIdx);
    double weightBefore = bodyGroup->getFieldValue(thirtyDaysAgo, weightIdx);
    if (weightBefore > 0 && trend.currentWeight > 0)
        trend.weightChange30d = trend.currentWeight - weightBefore;

    if (fatPctIdx >= 0) {
        trend.currentFatPercent = bodyGroup->getFieldValue(today, fatPctIdx);
        double fatBefore = bodyGroup->getFieldValue(thirtyDaysAgo, fatPctIdx);
        if (fatBefore > 0 && trend.currentFatPercent > 0)
            trend.fatPercentChange30d = trend.currentFatPercent - fatBefore;
    }

    return trend;
}
