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

#ifndef _GC_AthleteContext_h
#define _GC_AthleteContext_h

#include "Context.h"
#include "Athlete.h"
#include "Measures.h"
#include "RideItem.h"
#include "RideCache.h"
#include "RideFileCache.h"
#include "Season.h"
#include "Seasons.h"
#include "Zones.h"
#include "HrZones.h"
#include "IntervalItem.h"

#include <QObject>
#include <QDate>
#include <QDateTime>
#include <QList>
#include <QVector>
#include <QMap>

// Data structure for aggregated athlete context
struct AthleteProfile {
    // Basic athlete info
    QString name;
    QString sport = "cycling";
    QStringList activeSports;   // all sports the athlete practices (Bike, Run, Swim, etc.)
    double weight = 0.0;
    double height = 0.0;
    int ftp = 0;
    int maxHR = 0;
    int restHR = 0;

    // Body composition
    struct BodyComposition {
        double fatKg = 0.0;
        double muscleKg = 0.0;
        double bonesKg = 0.0;
        double leanKg = 0.0;
        double fatPercent = 0.0;
    } body;

    // Training metrics
    struct PMCMetrics {
        double ctl = 0.0;      // Chronic Training Load
        double atl = 0.0;      // Acute Training Load
        double tsb = 0.0;      // Training Stress Balance
        double rampRate = 0.0;
    } pmc;

    // HRV data
    struct HRVMetrics {
        double rmssd = 0.0;
        double sdnn = 0.0;
        QString trend; // "improving", "stable", "declining"
        int trendDays = 0;
    } hrv;

    // Recent performance
    struct PerformanceMetrics {
        double avgPower = 0.0;
        double normalizedPower = 0.0;
        double intensityFactor = 0.0;
        double powerDuration = 0.0; // Best power for current duration
        int totalRides = 0;
        double totalTime = 0.0;
        double totalDistance = 0.0;
        double totalWork = 0.0;
    } performance;

    // Goals and plans
    QString primaryGoal;
    QString eventDate;
    QString eventType;
    int weeksToEvent = 0;
};

struct RideSummary {
    QDateTime date;
    QString sport;
    double duration = 0.0;
    double distance = 0.0;
    double avgPower = 0.0;
    double maxPower = 0.0;
    double avgHR = 0.0;
    double tss = 0.0;
    double intensityFactor = 0.0;
    QString description;
    QMap<QString, double> metrics;

    // Rich metadata from ride file tags & external sources
    QString title;           // Workout Title
    QString keywords;        // user tags/keywords
    QString objective;       // training objective
    QString device;          // recording device
    QString route;           // route/location name
    QString workoutCode;     // workout type code
    bool isIndoor = false;   // trainer/indoor ride
    bool isCommute = false;  // commute flag
    double rpe = 0.0;       // Rate of Perceived Exertion (0-10)

    // Device-detected performance estimates (from Garmin, Wahoo, etc.)
    double deviceVO2max = 0.0;
    double aerobicTE = 0.0;     // Aerobic Training Effect
    double anaerobicTE = 0.0;   // Anaerobic Training Effect
    double recoveryTime = 0.0;  // hours
    double epoc = 0.0;          // excess post-exercise oxygen consumption

    // Additional cycling metrics
    double avgCadence = 0.0;
    double avgSpeed = 0.0;
    double avgTemp = 0.0;
    double leftRightBalance = 0.0;
    double aerobicDecoupling = 0.0;

    // Running-specific metrics
    double avgPace = 0.0;               // min/km
    double avgRunCadence = 0.0;         // spm
    double avgGroundContactTime = 0.0;  // ms
    double avgVerticalOscillation = 0.0;// cm
    double avgStrideLength = 0.0;       // m

    // Swimming-specific metrics
    double avgSwimPace = 0.0;   // min/100m
    double avgStrokeRate = 0.0;
    double swolf = 0.0;
};

// Interval summary within a ride
struct IntervalSummary {
    QString name;
    QString type;        // "device", "user", "peak_power", "effort", "climb"
    double startSecs = 0.0;
    double stopSecs = 0.0;
    double duration = 0.0;
    double avgPower = 0.0;
    double maxPower = 0.0;
    double avgHR = 0.0;
    double distance = 0.0;
    double avgCadence = 0.0;
    bool isTest = false;
};

// Season plan with phases and events
struct SeasonPlanSummary {
    QString seasonName;
    QDate start;
    QDate end;
    struct PhaseSummary {
        QString name;
        QString type;   // "prep", "base", "build", "peak", "camp"
        QDate start;
        QDate end;
    };
    QList<PhaseSummary> phases;
    struct EventSummary {
        QString name;
        QDate date;
        int priority = 0;
        QString description;
    };
    QList<EventSummary> events;
};

// Power curve bests at key durations
struct PowerCurveBests {
    double peak5s = 0.0;
    double peak1min = 0.0;
    double peak5min = 0.0;
    double peak20min = 0.0;
    double peak60min = 0.0;
    // W/kg equivalents
    double peak5sWkg = 0.0;
    double peak1minWkg = 0.0;
    double peak5minWkg = 0.0;
    double peak20minWkg = 0.0;
    double peak60minWkg = 0.0;
    QDate dateOf5s, dateOf1min, dateOf5min, dateOf20min, dateOf60min;
};

// Zone distribution for a ride or aggregated period
struct ZoneDistribution {
    QVector<double> powerZoneSeconds;   // seconds in each power zone (L1..L10)
    QVector<double> powerZonePercent;   // percentage in each power zone
    QStringList powerZoneNames;         // zone labels
    QVector<double> hrZoneSeconds;      // seconds in each HR zone (H1..H8)
    QVector<double> hrZonePercent;      // percentage in each HR zone
    QStringList hrZoneNames;            // zone labels
};

// Weekly training summary
struct WeeklyTrainingSummary {
    QDate weekStart;
    int rideCount = 0;
    double totalDuration = 0.0;     // seconds
    double totalDistance = 0.0;      // km
    double totalTSS = 0.0;
    double avgIntensityFactor = 0.0;
    double avgPower = 0.0;
    double maxPower = 0.0;
    double totalWork = 0.0;         // kJ
    double totalElevation = 0.0;    // m
    double endCTL = 0.0;
    double endATL = 0.0;
    double endTSB = 0.0;
    QString dominantWorkoutType;    // most frequent type that week
};

// Workout type classification
struct WorkoutClassification {
    QString type;       // "endurance", "tempo", "threshold", "vo2max", "sprint", "recovery", "race"
    int count = 0;
    double totalDuration = 0.0;
    double avgTSS = 0.0;
};

// Training recommendation struct - placed outside class for easier access
struct TrainingRecommendation {
    QString type; // "recovery", "endurance", "threshold", "vo2max", "rest day"
    QString rationale;
    double suggestedDuration;
    double suggestedIntensity;
    int priority;
};

class AthleteContextAggregator : public QObject
{
    Q_OBJECT

public:
    explicit AthleteContextAggregator(QObject* parent = nullptr);
    ~AthleteContextAggregator() override = default;

    // Update with current context
    void updateContext(Context* context);

    // Get current aggregated profile
    AthleteProfile getProfile() const { return profile_; }

    // Get recent rides summary
    QList<RideSummary> getRecentRides(int days = 90) const;

    // Get PMC history for trend analysis
    QVector<double> getPMCHistory(int days = 90) const;

    // Get HRV history
    QVector<double> getHRVHistory(int days = 30) const;

    // Get performance trends
    QMap<QString, QVector<double>> getMetricTrends(const QStringList& metrics, int days = 90) const;

    // Analyze patterns in the data
    struct PatternAnalysis {
        QStringList strengths;
        QStringList weaknesses;
        QStringList fatigueIndicators;
        QStringList improvementAreas;
        QString overallAssessment;
    };
    PatternAnalysis analyzePatterns() const;

    // Get training recommendations
    QList<TrainingRecommendation> getDailyRecommendations() const;

    // Get personalized insights
    QStringList getPersonalizedInsights() const;

    // Get power curve bests for a date range
    PowerCurveBests getPowerCurveBests(int days = 365) const;

    // Get zone distribution aggregated over recent rides
    ZoneDistribution getZoneDistribution(int days = 90) const;

    // Get weekly training summaries
    QList<WeeklyTrainingSummary> getWeeklySummaries(int weeks = 12) const;

    // Classify workouts by type
    QList<WorkoutClassification> getWorkoutTypeBreakdown(int days = 90) const;

    // Get year-over-year comparison for the current period
    struct PeriodComparison {
        double ctlCurrent = 0.0, ctlPrevious = 0.0;
        double volumeCurrent = 0.0, volumePrevious = 0.0;
        double tssCurrent = 0.0, tssPrevious = 0.0;
        int ridesCurrent = 0, ridesPrevious = 0;
    };
    PeriodComparison getYearOverYearComparison(int periodDays = 90) const;

    // Get interval summaries for recent rides (key efforts, tests, climbs)
    QList<IntervalSummary> getKeyIntervals(int days = 90) const;

    // Get season plan with phases and events
    SeasonPlanSummary getSeasonPlan() const;

    // Get sport-specific ride breakdown (how many rides per sport)
    QMap<QString, int> getSportBreakdown(int days = 90) const;

    // Get body composition history
    struct BodyCompTrend {
        double currentWeight = 0.0;
        double weightChange30d = 0.0;
        double currentFatPercent = 0.0;
        double fatPercentChange30d = 0.0;
    };
    BodyCompTrend getBodyCompositionTrend() const;

    // Save/Load athlete profile
    void saveProfile();
    void loadProfile();

signals:
    void profileUpdated(const AthleteProfile& profile);
    void recommendationsUpdated(const QList<TrainingRecommendation>& recommendations);

private:
    void calculatePMC();
    void calculateHRVTrends();
    void analyzeRecentPerformance();
    void detectFatiguePatterns();
    QString classifyWorkoutType(const RideSummary& ride) const;

    Context* currentContext_ = nullptr;
    AthleteProfile profile_;
    QList<RideSummary> recentRides_;
    QVector<double> pmcHistory_;
    QVector<double> hrvHistory_;
    PatternAnalysis patterns_;
};

#endif // _GC_AthleteContext_h
