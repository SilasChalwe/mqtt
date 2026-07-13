#include "../include/TreeStorage.h"
#include "../include/MQTTResponse.h"
#include <ArduinoJson.h>
#include <functional>

#if __has_include(<LittleFS.h>)
#include <LittleFS.h>
#define TREE_STORAGE_HAS_LITTLEFS 1
#else
#define TREE_STORAGE_HAS_LITTLEFS 0
#endif

namespace {
bool parseFixedMode(JsonVariantConst typeValue, bool fallbackFixed) {
    if (typeValue.is<const char*>()) {
        String type = typeValue.as<const char*>();
        type.toLowerCase();
        if (type == "fixed") return true;
        if (type == "auto") return false;
    }
    return fallbackFixed;
}
void nodeToJson(Node* node, JsonObject obj) {
    if (!node) return;
    obj["name"] = node->name;
    obj["amps"] = node->currentDraw;
    obj["voltage"] = node->voltage;
    obj["power_w"] = node->power;
    obj["energy_wh"] = node->energyWh;
    obj["priority"] = node->priority;
    obj["pin"] = node->relayPin;
    obj["friction"] = node->wireFriction;
    obj["type"] = node->typeString();
    obj["active"] = node->isActive;

    JsonArray children = obj.createNestedArray("children");
    for (Node* child : node->children) {
        JsonObject childObj = children.createNestedObject();
        nodeToJson(child, childObj);
    }
}

bool loadChildren(BestFirstSearch& bfs, const char* parentName, JsonArray children) {
    for (JsonObject child : children) {
        const char* name = child["name"];
        if (!name || String(name).length() == 0) continue;

        float amps = child.containsKey("amps") ? child["amps"].as<float>() : (child.containsKey("currentDraw") ? child["currentDraw"].as<float>() : 0.0f);
        float voltage = child["voltage"] | 230.0f;
        int priority = child["priority"] | 0;
        int pin = child.containsKey("pin") ? child["pin"].as<int>() : (child.containsKey("relayPin") ? child["relayPin"].as<int>() : -1);
        float friction = child.containsKey("friction") ? child["friction"].as<float>() : (child.containsKey("wireFriction") ? child["wireFriction"].as<float>() : 0.1f);
        bool fixed = parseFixedMode(child["type"], child["forced"] | false);

        Node* node = bfs.createAndAddNode(parentName, name, amps, priority, pin, friction, fixed, voltage);
        if (!node) return false;
        node->power = child["power_w"] | node->power;
        node->energyWh = child["energy_wh"] | 0.0f;
        node->isActive = child["active"] | false;
        node->lastActiveUpdateMs = millis();

        if (child["children"].is<JsonArray>()) {
            if (!loadChildren(bfs, name, child["children"].as<JsonArray>())) return false;
        }
    }
    return true;
}
}

namespace TreeStorage {

bool begin() {
#if TREE_STORAGE_HAS_LITTLEFS
    if (LittleFS.begin(false)) return true;
    Serial.println("LittleFS mount failed; /tree.txt not available");
    return false;
#else
    return false;
#endif
}

String toJson(BestFirstSearch& bfs) {
    DynamicJsonDocument doc(8192);
    JsonObject root = doc.to<JsonObject>();
    nodeToJson(bfs.getRoot(), root);
    String out;
    serializeJson(doc, out);
    return out;
}

bool save(BestFirstSearch& bfs) {
#if TREE_STORAGE_HAS_LITTLEFS
    if (!bfs.getRoot()) return false;
    if (!begin()) return false;
    File treeFile = LittleFS.open(TreePath, FILE_WRITE);
    if (!treeFile) {
        Serial.println("Failed to open /tree.txt for writing");
        return false;
    }
    String json = toJson(bfs);
    size_t written = treeFile.print(json);
    treeFile.close();
    bool ok = written == json.length();
    Serial.println(ok ? "Tree saved to /tree.txt" : "Failed to write complete /tree.txt");
    return ok;
#else
    (void)bfs;
    return false;
#endif
}

bool load(BestFirstSearch& bfs) {
#if TREE_STORAGE_HAS_LITTLEFS
    if (!begin()) return false;
    if (!LittleFS.exists(TreePath)) {
        Serial.println("/tree.txt not found; starting with default root");
        return true;
    }
    File treeFile = LittleFS.open(TreePath, FILE_READ);
    if (!treeFile) return false;
    DynamicJsonDocument doc(8192);
    DeserializationError err = deserializeJson(doc, treeFile);
    treeFile.close();
    if (err) {
        Serial.printf("Failed to parse /tree.txt JSON: %s\n", err.c_str());
        return false;
    }

    bfs.clear();
    Node* root = bfs.getRoot();
    if (root) {
        root->name = doc["name"] | "Main_DB";
        root->isActive = doc["active"] | true;
        root->mode = parseFixedMode(doc["type"], doc["forced"] | true) ? LoadMode::Fixed : LoadMode::Auto;
        root->energyWh = doc["energy_wh"] | 0.0f;
        root->lastActiveUpdateMs = millis();
    }
    if (doc["children"].is<JsonArray>()) {
        return loadChildren(bfs, root ? root->name.c_str() : "Main_DB", doc["children"].as<JsonArray>());
    }
    return true;
#else
    (void)bfs;
    return false;
#endif
}

}
