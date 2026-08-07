#pragma once
// Minimal stub for GCToolExecutor unit tests — replaces src/Core/Athlete.h
#include <QDir>
#include "Seasons.h"

class AthleteDirectoryStructure {
public:
    QDir workouts() { return QDir(); }
};

class Athlete {
public:
    AthleteDirectoryStructure *home = nullptr;
    Seasons *seasons = nullptr;
};
