#ifndef MQTT_RESPONSE_H
#define MQTT_RESPONSE_H

#include <Arduino.h>
#include <ArduinoJson.h>
#include "Node.h"

namespace MQTTResponse {
    String ok(const char* type, const char* requestTopic);
    String message(const char* type, const char* requestTopic, const char* message);
    String error(const char* requestTopic, const char* code, const char* message);
    String relayChanged(const char* requestTopic, Node* node, const char* requestedState);
    String nodeSummary(Node* node, const String& parentName = "");
    String optimisationResult(float availableCurrent, JsonArray loads);

    void addNode(JsonObject obj, Node* node, const String& parentName = "");
    const char* forcedState(Node* node);
}

#endif
