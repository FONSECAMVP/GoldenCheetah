// Force-included before every .cpp in testCoachTools via -include.
// Defines the REAL include guards for GC headers and provides lightweight stubs
// so the full GC class hierarchy is never loaded in these unit tests.

#ifndef _GC_STUB_PREAMBLE_H
#define _GC_STUB_PREAMBLE_H

// Qt includes needed by stubs
#include <QtGlobal>
#include <QString>
#include <QDate>
#include <QDir>
#include <QList>

// === Season.h stub (guard matches src/Core/Season.h) ===
#ifndef SEASON_H_
#define SEASON_H_

class SeasonEvent {
public:
    SeasonEvent() {}
    SeasonEvent(const QString& n, const QDate& d, int p = 0,
                const QString& desc = QString(), const QString& uid = QString())
        : name(n), date(d), priority(p), description(desc), id(uid) {}
    QString name;
    QDate date;
    int priority = 0;
    QString description;
    QString id;
};

class Season {
public:
    enum SeasonType { season = 0, cycle = 1, adhoc = 2, temporary = 3 };
    QList<SeasonEvent> events;
    QDate getStart() const { return QDate(); }
    QDate getEnd() const { return QDate(); }
    QDate getStart(QDate) const { return QDate(); }
    QDate getEnd(QDate) const { return QDate(); }
};

#endif // SEASON_H_

// === Seasons.h stub (guard matches src/Core/Seasons.h) ===
#ifndef _seasons_h
#define _seasons_h

class Seasons {
public:
    QList<Season> seasons;
    int newSeason(const QString&, const QDate&, const QDate&, int) {
        seasons.append(Season());
        return seasons.size() - 1;
    }
    bool writeSeasons() { return true; }
};

#endif // _seasons_h

// === Athlete.h stub (guard matches src/Core/Athlete.h) ===
#ifndef _GC_Athlete_h
#define _GC_Athlete_h

class AthleteDirectoryStructure {
public:
    QDir workouts() { return QDir(); }
};

class Athlete {
public:
    AthleteDirectoryStructure *home = nullptr;
    Seasons *seasons = nullptr;
};

#endif // _GC_Athlete_h

// === Context.h stub (guard matches src/Core/Context.h) ===
#ifndef _GC_Context_h
#define _GC_Context_h

#define CONFIG_SEASONS 0x400

class Context {
public:
    Athlete *athlete = nullptr;
    void notifyConfigChanged(qint32) {}
};

#endif // _GC_Context_h

#endif // _GC_STUB_PREAMBLE_H
