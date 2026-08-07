#pragma once
// Minimal stub for GCToolExecutor unit tests — replaces src/Core/Season.h
#include <QString>
#include <QDate>
#include <QList>

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
    QString getName() const { return QString(); }
};
