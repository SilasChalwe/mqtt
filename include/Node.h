#ifndef NODE_H
#define NODE_H

#include <Arduino.h>
#include <vector>

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
    bool isForced = false;
    bool isActive = false;
    float wireFriction = 0.0f;

    std::vector<Node*> children;

    bool isLoad() const;
    bool isBranch() const;
    void recalculatePower();
    const char* forcedState() const;
    void accumulateEnergy(unsigned long nowMs);
};

#endif
