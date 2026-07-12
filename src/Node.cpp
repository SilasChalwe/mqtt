#include "../include/Node.h"

bool Node::isLoad() const {
    return relayPin >= 0;
}

bool Node::isBranch() const {
    return relayPin < 0;
}

void Node::recalculatePower() {
    power = currentDraw * voltage;
}

const char* Node::forcedState() const {
    if (!isForced) return "not_forced";
    return isActive ? "forced_on" : "forced_off";
}

void Node::accumulateEnergy(unsigned long nowMs) {
    if (!isActive) {
        lastActiveUpdateMs = nowMs;
        return;
    }
    unsigned long deltaMs = (nowMs >= lastActiveUpdateMs) ? (nowMs - lastActiveUpdateMs) : 0;
    energyWh += power * (deltaMs / 3600000.0f);
    lastActiveUpdateMs = nowMs;
}
