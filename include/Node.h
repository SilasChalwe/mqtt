#ifndef NODE_H
#define NODE_H

#include <Arduino.h>
#include <vector>

enum class LoadMode {
    Auto,
    Fixed
};

struct Node {
    int id = 0;
    String name;

    float currentDraw = 0.0f;
    float voltage = 230.0f;
    float power = 0.0f;
    float energyWh = 0.0f;
    unsigned long lastActiveUpdateMs = 0;

    int priority = 0;
    int relayPin = -1;
    LoadMode mode = LoadMode::Auto;
    bool isActive = false;
    float wireFriction = 0.0f;

    bool hasSchedule = false;
    int scheduleStartMinute = -1;
    int scheduleEndMinute = -1;
    int scheduleBoost = 1000;
    int scheduleMode = 1;
    float scheduleMinimumSoc = 20.0f;
    int scheduleRequiredRuntimeMinutes = 0;
    int scheduleRuntimeTodayMinutes = 0;
    int scheduleLastRuntimeMinute = -1;

    std::vector<Node*> children;

    bool isLoad() const;
    bool isBranch() const;
    void recalculatePower();
    bool isFixed() const;
    const char* typeString() const;
    void accumulateEnergy(unsigned long nowMs);
};

#endif
