#include "../include/CommandHandler.h"
#include "../include/Battery.h"
#include "../lib/src/BestFirstSearch.h"
#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>
#include <ArduinoJson.h>
#include "../include/TimeManager.h"
#include <LittleFS.h>
#include <functional>
#include <freertos/queue.h>
#include <vector>

// Command queue globals (Option B)
QueueHandle_t commandQueue = NULL;
static BestFirstSearch* g_bfs = nullptr;
static MQTTManager* g_mqtt = nullptr;

// Task that dequeues commands and calls processCommand() in a single consumer
static void commandProcessorTask(void* pvParameters) {
    CommandMessage* msg = nullptr;
    while (true) {
        if (xQueueReceive(commandQueue, &msg, portMAX_DELAY) == pdTRUE) {
            if (msg) {
                processCommand(msg->topic, msg->payload, g_bfs, g_mqtt);
                delete msg;
                msg = nullptr;
            }
        }
    }
}

void initCommandQueue(BestFirstSearch* bfs, MQTTManager* mqtt) {
    if (commandQueue != NULL) return; // already initialized
    g_bfs = bfs;
    g_mqtt = mqtt;
    commandQueue = xQueueCreate(10, sizeof(CommandMessage*));
    if (commandQueue == NULL) {
        Serial.println("Failed to create command queue");
        return;
    }
    xTaskCreate(commandProcessorTask, "CmdProcessor", 8192, NULL, 2, NULL);
}

bool enqueueCommand(const String& topic, const String& payload) {
    if (commandQueue == NULL) return false;
    CommandMessage* m = new CommandMessage{topic, payload};
    if (m == nullptr) return false;
    BaseType_t ok = xQueueSend(commandQueue, &m, pdMS_TO_TICKS(100));
    if (ok != pdTRUE) {
        delete m;
        return false;
    }
    return true;
}

static bool validateCommandContext(BestFirstSearch* bfs, MQTTManager* mqtt, const String& topic) {
    if (!mqtt) return false;
    if (!bfs) {
        mqtt->publish("esp32/status/error", "Internal error: BFS unavailable");
        Serial.printf("Command failed: %s - BFS unavailable\n", topic.c_str());
        return false;
    }
    return true;
}

static bool isRootNodeName(const char* name) {
    return name != nullptr && String(name) == "Main_DB";
}

static bool parentExists(BestFirstSearch* bfs, const String& parentName) {
    if (parentName.length() == 0) return true;
    if (parentName == "Main_DB") return true;
    return bfs->getNode(parentName) != nullptr;
}

static void publishParentMissingError(MQTTManager* mqtt, const String& parentName) {
    String msg = "Parent branch '" + parentName + "' does not exist. Create the parent under Main_DB before adding this child.";
    mqtt->publish("esp32/status/error", msg.c_str());
    Serial.println(msg);
}

// Check whether a relay pin is already assigned anywhere in the tree
static const int CONFIG_PIN_STATUS_CANDIDATE_PINS[] = {
    2, 4, 5, 12, 13, 14, 15, 16, 17, 18, 19,
    21, 22, 23, 25, 26, 27, 32, 33
};

static bool pinInUse(BestFirstSearch* bfs, int pin) {
    if (pin < 0) return false;
    std::function<bool(Node*)> search = [&](Node* node) -> bool {
        if (!node) return false;
        if (node->relayPin == pin) return true;
        for (Node* c : node->children) {
            if (search(c)) return true;
        }
        return false;
    };
    return search(bfs->getRoot());
}

static void collectUsedPins(Node* node, std::vector<int>& usedPins) {
    if (!node) return;
    if (node->relayPin != -1) {
        usedPins.push_back(node->relayPin);
    }
    for (Node* child : node->children) {
        collectUsedPins(child, usedPins);
    }
}

static bool parseJsonDocument(const String& payload, DynamicJsonDocument& doc, const char* errorTopic,
                              const char* errorMessage, MQTTManager* mqtt) {
    DeserializationError err = deserializeJson(doc, payload);
    if (err) {
        Serial.printf("%s: %s\n", errorMessage, err.c_str());
        if (mqtt) mqtt->publish(errorTopic, errorMessage);
        return false;
    }
    return true;
}

// Return a short string describing forced state: "forced_on", "forced_off", or "not_forced"
static const char* forcedStateLabel(Node* node) {
    if (!node) return "not_forced";
    if (!node->isForced) return "not_forced";
    return node->isActive ? "forced_on" : "forced_off";
}

// Helper to format tree node recursively (used by save_tree)
void formatNode(Node* node, String& output, int indent = 0) {
    if (!node) return;
    for (int i = 0; i < indent; i++) output += "  ";
    output += node->name;
    output += " (amps=" + String(node->currentDraw, 1) +
              ", pri=" + String(node->priority) +
              ", pin=" + String(node->relayPin) +
              ", friction=" + String(node->wireFriction, 2) +
              ", forced=" + String(node->isForced ? "true" : "false") +
              ", active=" + String(node->isActive ? "true" : "false") + ")\n";
    for (Node* child : node->children) {
        formatNode(child, output, indent + 1);
    }
}

void processCommand(const String& topic, const String& payload,
                    BestFirstSearch* bfs, MQTTManager* mqtt) {
    if (!validateCommandContext(bfs, mqtt, topic)) return;

    // ---- Add a single load ----
    if (topic == "esp32/command/config/add") {
        StaticJsonDocument<256> doc;
        DeserializationError err = deserializeJson(doc, payload);
        if (err) {
            Serial.println("JSON parse error in add");
            mqtt->publish("esp32/status/error", "Invalid JSON for add");
            return;
        }

        const char* parent   = doc["parent"] | "Main_DB";
        const char* name     = doc["name"];
        float amps           = doc["amps"] | 0.0f;
        float voltage        = doc["voltage"] | 230.0f;
        int priority         = doc["priority"] | 0;
        int pin              = doc["pin"] | -1;
        float friction       = doc["friction"] | 0.1f;
        bool forced          = doc["forced"] | false;

        if (name == nullptr || String(name).length() == 0) {
            Serial.println("Missing 'name' in add");
            mqtt->publish("esp32/status/error", "Missing 'name'");
            return;
        }

        if (!bfs->getRoot()) {
            mqtt->publish("esp32/status/error", "Load tree unavailable");
            Serial.println("Add failed: tree root unavailable");
            return;
        }

        if (bfsMutex == NULL) {
            mqtt->publish("esp32/status/error", "Internal error: mutex unavailable");
            Serial.println("Add failed: bfsMutex not created");
            return;
        }

        if (xSemaphoreTake(bfsMutex, pdMS_TO_TICKS(500)) != pdTRUE) {
            mqtt->publish("esp32/status/error", "BFS busy");
            Serial.println("Add failed: BFS mutex unavailable");
            return;
        }

        String parentName = parent;
        if (!parentExists(bfs, parentName)) {
            xSemaphoreGive(bfsMutex);
            publishParentMissingError(mqtt, parentName);
            return;
        }

        // Enforce unique names across the entire tree
        if (bfs->getNode(name) != nullptr) {
            xSemaphoreGive(bfsMutex);
            String msg = String("Name already exists: ") + name;
            mqtt->publish("esp32/status/error", msg.c_str());
            Serial.println(msg);
            return;
        }

        // Enforce unique relay pin usage across the tree
        if (pin >= 0 && pinInUse(bfs, pin)) {
            xSemaphoreGive(bfsMutex);
            char msgBuf[64];
            snprintf(msgBuf, sizeof(msgBuf), "Pin %d already in use", pin);
            mqtt->publish("esp32/status/error", msgBuf);
            Serial.println(msgBuf);
            return;
        }

        Node* node = bfs->createAndAddNode(parentName.c_str(), name, amps, priority, pin, friction, forced, voltage);
        Serial.println(node ? "Node added" : "Add failed");
        mqtt->publish("esp32/status/config/node_added", node ? name : "failed");

        xSemaphoreGive(bfsMutex);
    }

    // ---- Bulk add ----
    else if (topic == "esp32/command/config/add_bulk") {
        size_t capacity = payload.length() + 512;
        DynamicJsonDocument doc(capacity);
        if (doc.capacity() == 0) {
            Serial.printf("Bulk add failed: out of memory (needed %d bytes)\n", capacity);
            mqtt->publish("esp32/status/error", "Out of memory for bulk add");
            return;
        }

        DeserializationError err = deserializeJson(doc, payload);
        if (err) {
            Serial.print("JSON parse error in bulk add: ");
            Serial.println(err.c_str());
            mqtt->publish("esp32/status/error", "Invalid JSON array for bulk add");
            return;
        }

        JsonArray arr = doc.as<JsonArray>();
        int count = 0;
        int skipped = 0;
        if (bfsMutex == NULL) {
            mqtt->publish("esp32/status/error", "Internal error: mutex unavailable");
            Serial.println("Bulk add failed: bfsMutex not created");
            return;
        }

        if (xSemaphoreTake(bfsMutex, pdMS_TO_TICKS(1000)) != pdTRUE) {
            mqtt->publish("esp32/status/error", "BFS busy");
            Serial.println("Bulk add failed: BFS mutex unavailable");
            return;
        }

        for (JsonObject obj : arr) {
            const char* parent   = obj["parent"] | "Main_DB";
            const char* name     = obj["name"];
            float amps           = obj["amps"] | 0.0f;
            float voltage        = obj["voltage"] | 230.0f;
            int priority         = obj["priority"] | 0;
            int pin              = obj["pin"] | -1;
            float friction       = obj["friction"] | 0.1f;
            bool forced          = obj["forced"] | false;

            if (name == nullptr || String(name).length() == 0) {
                skipped++;
                Serial.println("Bulk add skipped entry: missing name");
                continue;
            }

            String parentName = parent;
            if (!parentExists(bfs, parentName)) {
                skipped++;
                Serial.printf("Bulk add skipped: parent '%s' does not exist\n", parentName.c_str());
                continue;
            }
            // Enforce unique name
            if (bfs->getNode(name) != nullptr) {
                skipped++;
                Serial.printf("Bulk add skipped: name '%s' already exists\n", name);
                continue;
            }

            // Enforce unique pin usage
            if (pin >= 0 && pinInUse(bfs, pin)) {
                skipped++;
                Serial.printf("Bulk add skipped: pin %d already in use\n", pin);
                continue;
            }

            Node* node = bfs->createAndAddNode(parentName.c_str(), name, amps, priority, pin, friction, forced, voltage);
            if (node) {
                count++;
                Serial.println("Node added");
            } else {
                skipped++;
                Serial.printf("Failed to add node '%s' under parent '%s'\n", name, parentName.c_str());
            }
        }

        char msg[64];
        snprintf(msg, sizeof(msg), "Added %d loads, skipped %d invalid entries", count, skipped);
        mqtt->publish("esp32/status/config/bulk_added", msg);

        xSemaphoreGive(bfsMutex);
    }

    // ---- Update a load ----
    else if (topic == "esp32/command/config/update") {
        if (!bfs->getRoot()) {
            mqtt->publish("esp32/status/error", "Load tree unavailable");
            Serial.println("Update failed: tree root unavailable");
            return;
        }

        if (bfsMutex == NULL) {
            mqtt->publish("esp32/status/error", "Internal error: mutex unavailable");
            Serial.println("Update failed: bfsMutex not created");
            return;
        }

        if (xSemaphoreTake(bfsMutex, pdMS_TO_TICKS(500)) != pdTRUE) {
            mqtt->publish("esp32/status/error", "BFS busy");
            Serial.println("Update failed: BFS mutex unavailable");
            return;
        }

        StaticJsonDocument<256> doc;
        DeserializationError err = deserializeJson(doc, payload);
        if (err) {
            Serial.println("JSON parse error in update");
            mqtt->publish("esp32/status/error", "Invalid JSON for update");
            xSemaphoreGive(bfsMutex);
            return;
        }

        const char* name = doc["name"];
        if (name == nullptr || String(name).length() == 0) {
            Serial.println("Missing 'name' in update");
            mqtt->publish("esp32/status/error", "Missing 'name'");
            xSemaphoreGive(bfsMutex);
            return;
        }

        float amps   = doc["amps"] | -1.0f;
        float voltage = doc["voltage"] | -1.0f;
        int priority = doc["priority"] | -1;
        bool forced  = doc["forced"] | false;

        bool ok = bfs->updateNode(name, amps, priority, forced, voltage);
        Serial.println(ok ? "Update OK" : "Update failed");
        mqtt->publish("esp32/status/config/update_done", ok ? name : "failed");

        xSemaphoreGive(bfsMutex);
    }

    // ---- Delete a load ----
    else if (topic == "esp32/command/config/delete") {
        if (!bfs->getRoot()) {
            mqtt->publish("esp32/status/error", "Load tree unavailable");
            Serial.println("Delete failed: tree root unavailable");
            return;
        }

        if (bfsMutex == NULL) {
            mqtt->publish("esp32/status/error", "Internal error: mutex unavailable");
            Serial.println("Delete failed: bfsMutex not created");
            return;
        }

        if (xSemaphoreTake(bfsMutex, pdMS_TO_TICKS(500)) != pdTRUE) {
            mqtt->publish("esp32/status/error", "BFS busy");
            Serial.println("Delete failed: BFS mutex unavailable");
            return;
        }

        StaticJsonDocument<128> doc;
        DeserializationError err = deserializeJson(doc, payload);
        if (err) {
            Serial.println("JSON parse error in delete");
            mqtt->publish("esp32/status/error", "Invalid JSON for delete");
            xSemaphoreGive(bfsMutex);
            return;
        }

        const char* name = doc["name"];
        if (name == nullptr || String(name).length() == 0) {
            Serial.println("Missing 'name' in delete");
            mqtt->publish("esp32/status/error", "Missing 'name'");
            xSemaphoreGive(bfsMutex);
            return;
        }

        if (isRootNodeName(name)) {
            mqtt->publish("esp32/status/error", "Cannot delete root node");
            Serial.println("Delete failed: root node cannot be removed");
            xSemaphoreGive(bfsMutex);
            return;
        }

        bool removed = bfs->removeNode(name);
        mqtt->publish("esp32/status/config/delete_done", removed ? name : "failed");

        xSemaphoreGive(bfsMutex);
    }

    // ---- List all stored nodes ----
    else if (topic == "esp32/command/config/list") {
        if (!bfs->getRoot()) {
            mqtt->publish("esp32/status/config/list", "[]");
            Serial.println("=== Current Load Tree ===");
            Serial.println("Tree is empty.");
            Serial.println("=========================");
            return;
        }

        if (bfsMutex == NULL) {
            mqtt->publish("esp32/status/error", "Internal error: mutex unavailable");
            Serial.println("List failed: bfsMutex not created");
            return;
        }

        if (xSemaphoreTake(bfsMutex, pdMS_TO_TICKS(500)) != pdTRUE) {
            mqtt->publish("esp32/status/error", "BFS busy");
            Serial.println("List failed: BFS mutex unavailable");
            return;
        }

        if (bfs->getRoot()->children.empty()) {
            mqtt->publish("esp32/status/config/list", "[]");
            Serial.println("=== Current Load Tree ===");
            Serial.println("Tree is empty.");
            Serial.println("=========================");
            xSemaphoreGive(bfsMutex);
            return;
        }

        DynamicJsonDocument doc(4096);
        JsonArray arr = doc.to<JsonArray>();

        std::function<void(Node*, JsonArray)> collectNodes = [&](Node* node, JsonArray parentArr) {
            if (!node) return;
            JsonObject obj = parentArr.createNestedObject();
            obj["name"]     = node->name;
            obj["amps"]     = node->currentDraw;
            obj["voltage"]  = node->voltage;
            obj["power_w"]  = node->power;
            obj["energy_wh"] = node->energyWh;
            obj["priority"] = node->priority;
            obj["pin"]      = node->relayPin;
            obj["forced"]   = node->isForced;
            obj["forced_state"] = forcedStateLabel(node);
            obj["active"]   = node->isActive;

            for (Node* child : node->children) {
                collectNodes(child, parentArr);
            }
        };

        for (Node* child : bfs->getRoot()->children) {
            collectNodes(child, arr);
        }

        String jsonStr;
        serializeJson(arr, jsonStr);
        mqtt->publish("esp32/status/config/list", jsonStr.c_str());

        Serial.println("=== Current Load Tree (JSON) ===");
        Serial.println(jsonStr);
        Serial.println("================================");

        xSemaphoreGive(bfsMutex);
    }

    // ---- Return relay pin usage status ----
    else if (topic == "esp32/command/config/pin_status") {
        if (!bfs->getRoot()) {
            mqtt->publish("esp32/status/config/pin_status", "{\"used\":[],\"available\":[]}");
            return;
        }

        if (bfsMutex == NULL) {
            mqtt->publish("esp32/status/error", "Internal error: mutex unavailable");
            Serial.println("Pin status failed: bfsMutex not created");
            return;
        }

        if (xSemaphoreTake(bfsMutex, pdMS_TO_TICKS(500)) != pdTRUE) {
            mqtt->publish("esp32/status/error", "BFS busy");
            Serial.println("Pin status failed: BFS mutex unavailable");
            return;
        }

        std::vector<int> usedPins;
        collectUsedPins(bfs->getRoot(), usedPins);

        DynamicJsonDocument pinDoc(1024);
        JsonArray usedArr = pinDoc.createNestedArray("used");
        for (int pin : usedPins) {
            usedArr.add(pin);
        }

        JsonArray availableArr = pinDoc.createNestedArray("available");
        for (int i = 0; i < (int)(sizeof(CONFIG_PIN_STATUS_CANDIDATE_PINS) / sizeof(CONFIG_PIN_STATUS_CANDIDATE_PINS[0])); i++) {
            int pin = CONFIG_PIN_STATUS_CANDIDATE_PINS[i];
            if (!pinInUse(bfs, pin)) {
                availableArr.add(pin);
            }
        }

        String out;
        serializeJson(pinDoc, out);
        mqtt->publish("esp32/status/config/pin_status", out.c_str());

        xSemaphoreGive(bfsMutex);
    }

    // ---- Get a specific node ----
    else if (topic == "esp32/command/config/get_node") {
        StaticJsonDocument<128> doc;
        DeserializationError err = deserializeJson(doc, payload);
        if (err) {
            Serial.println("JSON parse error in get_node");
            mqtt->publish("esp32/status/error", "Invalid JSON for get_node");
            return;
        }

        const char* name = doc["name"];
        if (name == nullptr) {
            Serial.println("Missing 'name' in get_node");
            mqtt->publish("esp32/status/error", "Missing 'name'");
            return;
        }

        if (bfsMutex == NULL) {
            mqtt->publish("esp32/status/error", "Internal error: mutex unavailable");
            Serial.println("Get node failed: bfsMutex not created");
            return;
        }

        if (xSemaphoreTake(bfsMutex, pdMS_TO_TICKS(500)) != pdTRUE) {
            mqtt->publish("esp32/status/error", "BFS busy");
            Serial.println("Get node failed: BFS mutex unavailable");
            return;
        }

        Node* node = bfs->getNode(name);
        if (!node) {
            mqtt->publish("esp32/status/config/get_node", "{\"error\":\"not found\"}");
            Serial.printf("Node '%s' not found\n", name);
            xSemaphoreGive(bfsMutex);
            return;
        }

        StaticJsonDocument<256> resp;
        resp["name"]     = node->name;
        resp["amps"]     = node->currentDraw;
        resp["voltage"]  = node->voltage;
        resp["power_w"]  = node->power;
        resp["energy_wh"] = node->energyWh;
        resp["priority"] = node->priority;
        resp["pin"]      = node->relayPin;
        resp["friction"] = node->wireFriction;
        resp["forced"]   = node->isForced;
        resp["forced_state"] = forcedStateLabel(node);
        resp["active"]   = node->isActive;

        String jsonStr;
        serializeJson(resp, jsonStr);
        mqtt->publish("esp32/status/config/get_node", jsonStr.c_str());

        Serial.println("=== Node Details ===");
        Serial.println(jsonStr);
        Serial.println("====================");

        xSemaphoreGive(bfsMutex);
    }

    // ---- Execute optimisation ----
    else if (topic == "esp32/command/control/execute") {
        if (!bfs->getRoot() || bfs->getRoot()->children.empty()) {
            mqtt->publish("esp32/status/control/result", "Auto-optimisation skipped (tree empty)");
            Serial.println(">>> Auto-optimisation skipped (tree empty)");
            return;
        }

        if (bfsMutex == NULL) {
            mqtt->publish("esp32/status/error", "Internal error: mutex unavailable");
            Serial.println("Execute failed: bfsMutex not created");
            return;
        }

        if (xSemaphoreTake(bfsMutex, pdMS_TO_TICKS(2000)) != pdTRUE) {
            mqtt->publish("esp32/status/error", "BFS busy");
            Serial.println("Execute failed: BFS mutex unavailable");
            return;
        }

        float available = 10.0f;
        StaticJsonDocument<64> doc;
        DeserializationError err = deserializeJson(doc, payload);
        if (!err) {
            available = doc["available_current"] | 10.0f;
        } else {
            available = payload.toFloat();
            if (available <= 0.0f) available = 10.0f;
        }

        if (available <= 0.0f) available = 10.0f;

        // Immediate confirmation
        mqtt->publish("esp32/status/control/execute_received", "Running optimisation...");
        Serial.println(">>> Execute command received, running optimisation...");

        auto candidates = bfs->scout(available);
        float availablePower = available * battery_voltage;
        bfs->execute(candidates, available, availablePower);

        // Serial output
        Serial.println("========= OPTIMISATION RESULT =========");
        Serial.printf("Available current: %.1f A\n", available);
        String onList;
        for (auto n : candidates) {
            Serial.printf("  %-20s %s  (%.1f A, priority %d, friction %.2f)\n",
                          n->name.c_str(),
                          n->isActive ? "ON " : "OFF",
                          n->currentDraw,
                          n->priority,
                          n->wireFriction);
            if (n->isActive) onList += n->name + ",";
        }
        if (onList.length()) onList.remove(onList.length() - 1);
        String timeStr = TimeManager::isTimeValid()
                         ? TimeManager::getFormattedTime("%Y-%m-%d %H:%M:%S")
                         : "N/A";
        Serial.printf("Active loads: %s\n", onList.c_str());
        Serial.printf("Optimisation run at: %s\n", timeStr.c_str());
        Serial.println("=========================================");

        // MQTT result with timestamp -- include all loads and their attributes
        DynamicJsonDocument resultDoc(2048);
        resultDoc["timestamp"] = timeStr;
        resultDoc["available_current"] = available;
        JsonArray loads = resultDoc.createNestedArray("loads");

        std::function<void(Node*, const String&)> collectLoads = [&](Node* node, const String& parentName) {
            if (!node) return;
            // If this node is an appliance (has a pin), include it
            if (node->relayPin != -1) {
                JsonObject obj = loads.createNestedObject();
                obj["name"] = node->name;
                obj["parent"] = parentName;
                obj["amps"] = node->currentDraw;
                obj["voltage"] = node->voltage;
                obj["power_w"] = node->power;
                obj["energy_wh"] = node->energyWh;
                obj["priority"] = node->priority;
                obj["pin"] = node->relayPin;
                obj["friction"] = node->wireFriction;
                obj["forced"] = node->isForced;
                obj["forced_state"] = forcedStateLabel(node);
                obj["active"] = node->isActive;
            }
            for (Node* c : node->children) {
                collectLoads(c, node->name);
            }
        };

        // collect from root's children (do not include Main_DB as a load)
        for (Node* child : bfs->getRoot()->children) {
            collectLoads(child, "Main_DB");
        }

        String resultJson;
        serializeJson(resultDoc, resultJson);
        mqtt->publish("esp32/status/control/result", resultJson.c_str());

        // Save to LittleFS
        if (!LittleFS.begin(true)) {
            Serial.println("LittleFS mount failed – can't save log");
        } else {
            File logFile = LittleFS.open("/results.log", FILE_APPEND);
            if (logFile) {
                String timeStr = TimeManager::isTimeValid()
                                 ? TimeManager::getFormattedTime("%Y-%m-%d %H:%M:%S")
                                 : "N/A";
                logFile.printf("[%s] Available: %.1f A -> %s\n",
                               timeStr.c_str(), available, onList.c_str());
                logFile.close();
                Serial.println("Result saved to /results.log");
            } else {
                Serial.println("Failed to open log file");
            }
        }
        xSemaphoreGive(bfsMutex);
    }

    // ---- Save full tree to file ----
    else if (topic == "esp32/command/config/save_tree") {
        if (!bfs->getRoot()) {
            mqtt->publish("esp32/status/error", "Load tree unavailable");
            Serial.println("Save tree failed: tree root unavailable");
            return;
        }

        if (bfsMutex == NULL) {
            mqtt->publish("esp32/status/error", "Internal error: mutex unavailable");
            Serial.println("Save tree failed: bfsMutex not created");
            return;
        }

        if (xSemaphoreTake(bfsMutex, pdMS_TO_TICKS(1000)) != pdTRUE) {
            mqtt->publish("esp32/status/error", "BFS busy");
            Serial.println("Save tree failed: BFS mutex unavailable");
            return;
        }

        if (!LittleFS.begin(true)) {
            Serial.println("LittleFS mount failed");
            mqtt->publish("esp32/status/error", "LittleFS mount failed");
            xSemaphoreGive(bfsMutex);
            return;
        }

        File treeFile = LittleFS.open("/tree.txt", FILE_WRITE);
        if (!treeFile) {
            Serial.println("Failed to create tree file");
            mqtt->publish("esp32/status/error", "Cannot write tree file");
            xSemaphoreGive(bfsMutex);
            return;
        }

        String treeStr = "=== Load Tree ===\n";
        formatNode(bfs->getRoot(), treeStr);
        treeFile.print(treeStr);
        treeFile.close();
        Serial.println("Tree saved to /tree.txt");
        Serial.println(treeStr);
        mqtt->publish("esp32/status/config/tree_saved", "OK");

        xSemaphoreGive(bfsMutex);
    }

    // ---- Return full nested tree as JSON ----
    else if (topic == "esp32/command/config/tree") {
        if (!bfs->getRoot()) {
            mqtt->publish("esp32/status/config/tree", "{}");
            Serial.println("Tree request: tree unavailable");
            return;
        }

        if (bfsMutex == NULL) {
            mqtt->publish("esp32/status/error", "Internal error: mutex unavailable");
            Serial.println("Tree request failed: bfsMutex not created");
            return;
        }

        if (xSemaphoreTake(bfsMutex, pdMS_TO_TICKS(1000)) != pdTRUE) {
            mqtt->publish("esp32/status/error", "BFS busy");
            Serial.println("Tree request failed: BFS mutex unavailable");
            return;
        }

        DynamicJsonDocument doc(8192);
        JsonObject rootObj = doc.to<JsonObject>();
        rootObj["name"] = bfs->getRoot()->name;

        std::function<JsonObject(Node*, JsonObject&)> nodeToJson = [&](Node* node, JsonObject& parentObj) -> JsonObject {
            JsonObject obj = parentObj.createNestedObject(node->name);
            obj["name"] = node->name;
            obj["amps"] = node->currentDraw;
            obj["voltage"] = node->voltage;
            obj["power_w"] = node->power;
            obj["energy_wh"] = node->energyWh;
            obj["priority"] = node->priority;
            obj["pin"] = node->relayPin;
            obj["friction"] = node->wireFriction;
            obj["forced"] = node->isForced;
            obj["active"] = node->isActive;
            JsonArray children = obj.createNestedArray("children");
            for (Node* c : node->children) {
                JsonObject childObj = children.createNestedObject();
                childObj["name"] = c->name;
                childObj["amps"] = c->currentDraw;
                childObj["voltage"] = c->voltage;
                childObj["power_w"] = c->power;
                childObj["energy_wh"] = c->energyWh;
                childObj["priority"] = c->priority;
                childObj["pin"] = c->relayPin;
                childObj["friction"] = c->wireFriction;
                childObj["forced"] = c->isForced;
                childObj["forced_state"] = forcedStateLabel(c);
                childObj["active"] = c->isActive;
                // Recursively add grandchildren
                if (!c->children.empty()) {
                    JsonArray gc = childObj.createNestedArray("children");
                    std::function<void(Node*, JsonArray&)> addChildren = [&](Node* n, JsonArray& arr) {
                        for (Node* g : n->children) {
                            JsonObject go = arr.createNestedObject();
                            go["name"] = g->name;
                            go["amps"] = g->currentDraw;
                            go["priority"] = g->priority;
                            go["pin"] = g->relayPin;
                            go["friction"] = g->wireFriction;
                            go["forced"] = g->isForced;
                            go["forced_state"] = forcedStateLabel(g);
                            go["active"] = g->isActive;
                            if (!g->children.empty()) {
                                JsonArray sub = go.createNestedArray("children");
                                addChildren(g, sub);
                            }
                        }
                    };
                    addChildren(c, gc);
                }
            }
            return obj;
        };

        // Build children under root
        JsonArray rootChildren = rootObj.createNestedArray("children");
        for (Node* child : bfs->getRoot()->children) {
            JsonObject childObj = rootChildren.createNestedObject();
            childObj["name"] = child->name;
            childObj["amps"] = child->currentDraw;
            childObj["priority"] = child->priority;
            childObj["pin"] = child->relayPin;
            childObj["friction"] = child->wireFriction;
            childObj["forced"] = child->isForced;
            childObj["forced_state"] = forcedStateLabel(child);
            childObj["active"] = child->isActive;
            if (!child->children.empty()) {
                JsonArray cArr = childObj.createNestedArray("children");
                std::function<void(Node*, JsonArray&)> addChildren = [&](Node* n, JsonArray& arr) {
                    for (Node* g : n->children) {
                        JsonObject go = arr.createNestedObject();
                        go["name"] = g->name;
                        go["amps"] = g->currentDraw;
                        go["priority"] = g->priority;
                        go["pin"] = g->relayPin;
                        go["friction"] = g->wireFriction;
                        go["forced"] = g->isForced;
                        go["forced_state"] = forcedStateLabel(g);
                        go["active"] = g->isActive;
                        if (!g->children.empty()) {
                            JsonArray sub = go.createNestedArray("children");
                            addChildren(g, sub);
                        }
                    }
                };
                addChildren(child, cArr);
            }
        }

        String out;
        serializeJson(rootObj, out);
        mqtt->publish("esp32/status/config/tree", out.c_str());
        Serial.println("=== Tree JSON ===");
        Serial.println(out);

        xSemaphoreGive(bfsMutex);
    }

    // ---- Manual relay toggle ----
    else if (topic.startsWith("esp32/command/control/relay/")) {
        if (!bfs->getRoot()) {
            mqtt->publish("esp32/status/error", "Load tree unavailable");
            Serial.println("Relay toggle failed: tree root unavailable");
            return;
        }

        if (bfsMutex == NULL) {
            mqtt->publish("esp32/status/error", "Internal error: mutex unavailable");
            Serial.println("Relay toggle failed: bfsMutex not created");
            return;
        }

        if (xSemaphoreTake(bfsMutex, pdMS_TO_TICKS(500)) != pdTRUE) {
            mqtt->publish("esp32/status/error", "BFS busy");
            Serial.println("Relay toggle failed: BFS mutex unavailable");
            return;
        }

        String nodeName = topic.substring(strlen("esp32/command/control/relay/"));
        if (nodeName.length() == 0) {
            mqtt->publish("esp32/status/error", "Invalid relay node name");
            xSemaphoreGive(bfsMutex);
            return;
        }

        Node* node = bfs->getNode(nodeName);
        if (!node || node->relayPin < 0) {
            mqtt->publish("esp32/status/error", "Invalid relay node");
            xSemaphoreGive(bfsMutex);
            return;
        }

        String state = payload;
        StaticJsonDocument<64> doc;
        if (deserializeJson(doc, payload) == DeserializationError::Ok) {
            state = doc["state"] | state;
        }

        if (state == "on" || state == "1") {
            digitalWrite(node->relayPin, HIGH);
            node->isActive = true;
        } else if (state == "off" || state == "0") {
            digitalWrite(node->relayPin, LOW);
            node->isActive = false;
        } else {
            mqtt->publish("esp32/status/error", "Invalid relay state (use 'on'/'off')");
            xSemaphoreGive(bfsMutex);
            return;
        }
        mqtt->publish("esp32/status/control/relay_changed", nodeName.c_str());

        xSemaphoreGive(bfsMutex);
    }
}