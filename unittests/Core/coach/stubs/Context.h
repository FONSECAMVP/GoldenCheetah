#pragma once
// Minimal stub for GCToolExecutor unit tests — replaces src/Core/Context.h
#include <QtGlobal>
#define CONFIG_SEASONS 0x400

class Athlete;
class Context {
public:
    Athlete *athlete = nullptr;
    void notifyConfigChanged(qint32) {}
};
