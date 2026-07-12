#include "../include/MQTTResponse.h"

namespace {
String serializeDoc(JsonDocument& doc) {
    String out;
    serializeJson(doc, out);
    return out;
}

void addEnvelope(DynamicJsonDocument& doc, bool ok, const char* type, const char* requestTopic) {
    doc["ok"] = ok;
    doc["type"] = type;
    doc["request_topic"] = requestTopic ? requestTopic : "";
}
}

namespace MQTTResponse {

const char* forcedState(Node* node) {
    return node ? node->forcedState() : "not_forced";
}

void addNode(JsonObject obj, Node* node, const String& parentName) {
    if (!node) return;
    obj["name"] = node->name;
    if (parentName.length() > 0) {
        obj["parent"] = parentName;
    }
    obj["amps"] = node->currentDraw;
    obj["voltage"] = node->voltage;
    obj["power_w"] = node->power;
    obj["energy_wh"] = node->energyWh;
    obj["priority"] = node->priority;
    obj["pin"] = node->relayPin;
    obj["friction"] = node->wireFriction;
    obj["forced"] = node->isForced;
    obj["forced_state"] = forcedState(node);
    obj["active"] = node->isActive;
}

String ok(const char* type, const char* requestTopic) {
    DynamicJsonDocument doc(256);
    addEnvelope(doc, true, type, requestTopic);
    JsonObject data = doc.createNestedObject("data");
    (void)data;
    return serializeDoc(doc);
}

String message(const char* type, const char* requestTopic, const char* message) {
    DynamicJsonDocument doc(384);
    addEnvelope(doc, true, type, requestTopic);
    doc["message"] = message ? message : "";
    JsonObject data = doc.createNestedObject("data");
    (void)data;
    return serializeDoc(doc);
}

String error(const char* requestTopic, const char* code, const char* message) {
    DynamicJsonDocument doc(384);
    addEnvelope(doc, false, "error", requestTopic);
    doc["code"] = code ? code : "error";
    doc["message"] = message ? message : "";
    return serializeDoc(doc);
}

String relayChanged(const char* requestTopic, Node* node, const char* requestedState) {
    DynamicJsonDocument doc(512);
    addEnvelope(doc, true, "control.relay_changed", requestTopic);
    JsonObject data = doc.createNestedObject("data");
    if (requestedState) {
        data["requested_state"] = requestedState;
    }
    addNode(data, node);
    return serializeDoc(doc);
}

String nodeSummary(Node* node, const String& parentName) {
    DynamicJsonDocument doc(512);
    JsonObject obj = doc.to<JsonObject>();
    addNode(obj, node, parentName);
    return serializeDoc(doc);
}

String optimisationResult(float availableCurrent, JsonArray loads) {
    DynamicJsonDocument doc(256);
    addEnvelope(doc, true, "control.result", "esp32/command/control/execute");
    doc["data"]["available_current"] = availableCurrent;
    doc["data"]["load_count"] = loads.size();
    return serializeDoc(doc);
}

}
