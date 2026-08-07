#pragma once
// Minimal stub for GCToolExecutor unit tests — replaces src/Core/Seasons.h
#include "Season.h"
#include <QString>
#include <QDate>
#include <QList>

class Seasons {
public:
    QList<Season> seasons;
    int newSeason(const QString&, const QDate&, const QDate&, int) {
        seasons.append(Season());
        return seasons.size() - 1;
    }
    bool writeSeasons() { return true; }
};
