#include "../include/RelayControl.h"
#include "../config/Config.h"

void RelayControl::begin(int pin) {
    if (pin < 0) return;
    pinMode(pin, OUTPUT);
    set(pin, false);
}

void RelayControl::set(int pin, bool active) {
    if (pin < 0) return;
    digitalWrite(pin, active == RELAY_ACTIVE_HIGH ? HIGH : LOW);
}

void RelayControl::apply(const std::vector<Node*>& loads) {
    for (Node* node : loads) {
        if (node && node->isLoad()) {
            set(node->relayPin, node->isActive);
        }
    }
}

void setRelayState(int pin, bool active) {
    RelayControl::set(pin, active);
}
