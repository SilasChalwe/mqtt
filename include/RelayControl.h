#ifndef RELAY_CONTROL_H
#define RELAY_CONTROL_H

#include <Arduino.h>
#include <vector>
#include "Node.h"

class RelayControl {
public:
    static void begin(int pin);
    static void set(int pin, bool active);
    static void apply(const std::vector<Node*>& loads);
};

void setRelayState(int pin, bool active);

#endif
