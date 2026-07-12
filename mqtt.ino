#include "config/Config.h"
#include "include/Battery.h"
#include "include/SolarManager.h"
#include "include/PowerEstimator.h"
#include "include/WiFiManager.h"
#include "include/MQTTManager.h"
#include "include/CommandHandler.h"
#include "include/TimeManager.h"
#include "lib/src/BestFirstSearch.h"
#include <ArduinoJson.h>
#include <cmath>

static float sanitizeReading(float value) {
    return (isnan(value) || isinf(value)) ? 0.0f : value;
}

// Shared sensor globals required by Battery.h and SolarManager.
Adafruit_INA219 ina219;
float battery_soc = 0.0f;
float battery_voltage = 0.0f;


WiFiManager wifi;
MQTTManager mqtt;
BestFirstSearch bfs;
SolarManager solar(35);
PowerEstimator estimator(solar, 0.2);

// FreeRTOS task handles
TaskHandle_t tempTaskHandle = NULL;
TaskHandle_t powerTaskHandle = NULL;
TaskHandle_t optimiseTaskHandle = NULL;

// Synchronisation primitive used to protect BFS tree and node operations
SemaphoreHandle_t bfsMutex = NULL;

// ------------------------------------------------------------
// Helper: run the optimisation and publish the result
// ------------------------------------------------------------
static bool canRunOptimisation() {
    if (!bfs.getRoot()) {
        Serial.println(">>> Auto-optimisation skipped (tree uninitialised)");
        return false;
    }
    if (bfs.getRoot()->children.empty()) {
        Serial.println(">>> Auto-optimisation skipped (tree empty)");
        return false;
    }
    return true;
}

void runOptimisation(float availableCurrent) {
    if (!canRunOptimisation()) {
        mqtt.publish("esp32/status/control/result", "Auto-optimisation skipped (tree empty)");
        return;
    }

    if (xSemaphoreTake(bfsMutex, pdMS_TO_TICKS(500)) == pdTRUE) {
        auto candidates = bfs.scout(availableCurrent);
        float availablePower = availableCurrent * battery_voltage;
        bfs.execute(candidates, availableCurrent, availablePower);

        // Build JSON with all loads (attributes + active state)
        DynamicJsonDocument resultDoc(2048);
        String timeStr = TimeManager::isTimeValid()
                         ? TimeManager::getFormattedTime("%Y-%m-%d %H:%M:%S")
                         : "N/A";
        resultDoc["timestamp"] = timeStr;
        resultDoc["available_current"] = availableCurrent;
        JsonArray loads = resultDoc.createNestedArray("loads");

        std::function<void(Node*, const String&)> collectLoads = [&](Node* node, const String& parentName) {
            if (!node) return;
            if (node->relayPin != -1) {
                JsonObject obj = loads.createNestedObject();
                obj["name"] = node->name;
                obj["parent"] = parentName;
                obj["amps"] = node->currentDraw;
                obj["priority"] = node->priority;
                obj["pin"] = node->relayPin;
                obj["friction"] = node->wireFriction;
                obj["forced"] = node->isForced;
                obj["forced_state"] = node->isForced ? (node->isActive ? "forced_on" : "forced_off") : "not_forced";
                obj["active"] = node->isActive;
            }
            for (Node* c : node->children) collectLoads(c, node->name);
        };

        for (Node* child : bfs.getRoot()->children) {
            collectLoads(child, "Main_DB");
        }

        String resultJson;
        serializeJson(resultDoc, resultJson);

        xSemaphoreGive(bfsMutex);

        mqtt.publish("esp32/status/control/result", resultJson.c_str());
        Serial.println(">>> Auto-optimisation result (JSON):");
        Serial.println(resultJson);
    } else {
        mqtt.publish("esp32/status/control/result", "Auto-optimisation skipped (mutex unavailable)");
        Serial.println(">>> Auto-optimisation skipped: failed to acquire BFS mutex");
    }
}

// ------------------------------------------------------------
// FreeRTOS task: run optimisation every OPTIMISE_INTERVAL
// ------------------------------------------------------------
void optimisePublishTask(void *pvParameters) {
    TickType_t xLastWakeTime = xTaskGetTickCount();
    const TickType_t xFrequency = pdMS_TO_TICKS(OPTIMISE_INTERVAL);

    while (true) {
        vTaskDelayUntil(&xLastWakeTime, xFrequency);
        runOptimisation(OPTIMISE_CURRENT);
    }
}

// ------------------------------------------------------------
// FreeRTOS task: publish temperature + time every PUBLISH_INTERVAL
// ------------------------------------------------------------
void tempPublishTask(void *pvParameters) {
    TickType_t xLastWakeTime = xTaskGetTickCount();
    const TickType_t xFrequency = pdMS_TO_TICKS(PUBLISH_INTERVAL);

    while (true) {
        vTaskDelayUntil(&xLastWakeTime, xFrequency);

        String timeStr = TimeManager::isTimeValid()
                         ? TimeManager::getFormattedTime("%Y-%m-%d %H:%M:%S")
                         : "N/A";

        char payload[128];
        snprintf(payload, sizeof(payload),
                 "{\"temp\":%.1f,\"time\":\"%s\"}",
                 temperatureRead(), timeStr.c_str());

        mqtt.publish(TOPIC_SENSOR_PUB, payload);
        Serial.printf("[%s] Published to %s: %s\n",
                      timeStr.c_str(), TOPIC_SENSOR_PUB, payload);
    }
}

void powerPublishTask(void *pvParameters) {
    TickType_t xLastWakeTime = xTaskGetTickCount();
    const TickType_t xFrequency = pdMS_TO_TICKS(PUBLISH_INTERVAL);

    while (true) {
        vTaskDelayUntil(&xLastWakeTime, xFrequency);

        // Update sensor readings and estimator state.
        solar.update();
        updateBattery();
        estimator.updateSensors();

        float currentBatteryVoltage = sanitizeReading(battery_voltage);
        float currentBatterySoC = sanitizeReading(battery_soc);
        float currentSolarVoltage = sanitizeReading(solar.getVoltage());
        float currentSolarCurrent = sanitizeReading(solar.getCurrent());
        float currentSolarPower = sanitizeReading(solar.getPower());
        float currentSolarEnergy = sanitizeReading(solar.getEnergyWh());

        String timeStr = TimeManager::isTimeValid()
                         ? TimeManager::getFormattedTime("%Y-%m-%d %H:%M:%S")
                         : "N/A";

        // Battery telemetry
        DynamicJsonDocument batteryDoc(256);
        batteryDoc["voltage"] = currentBatteryVoltage;
        batteryDoc["soc"] = currentBatterySoC;
        batteryDoc["battery_capacity_ah"] = BATTERY_CAPACITY_AH;
        batteryDoc["time"] = timeStr;
        String batteryJson;
        serializeJson(batteryDoc, batteryJson);
        mqtt.publish(TOPIC_BATTERY_PUB, batteryJson.c_str());
        Serial.printf("[%s] Published to %s: %s\n", timeStr.c_str(), TOPIC_BATTERY_PUB, batteryJson.c_str());

        // Solar telemetry
        DynamicJsonDocument solarDoc(256);
        solarDoc["voltage"] = currentSolarVoltage;
        solarDoc["current"] = currentSolarCurrent;
        solarDoc["power"] = currentSolarPower;
        solarDoc["energy_wh"] = currentSolarEnergy;
        solarDoc["time"] = timeStr;
        String solarJson;
        serializeJson(solarDoc, solarJson);
        mqtt.publish(TOPIC_SOLAR_PUB, solarJson.c_str());
        Serial.printf("[%s] Published to %s: %s\n", timeStr.c_str(), TOPIC_SOLAR_PUB, solarJson.c_str());

        // Estimator telemetry
        DynamicJsonDocument estimatorDoc(384);
        estimatorDoc["battery_soc"] = sanitizeReading(estimator.get_BatterySoC());
        estimatorDoc["battery_voltage"] = sanitizeReading(estimator.get_BatteryVoltage());
        estimatorDoc["solar_current"] = sanitizeReading(estimator.get_SolarCurrent());
        estimatorDoc["solar_power_w"] = sanitizeReading(currentSolarVoltage * currentSolarCurrent);
        estimatorDoc["battery_power_w"] = sanitizeReading(currentBatteryVoltage * std::max(0.0f, estimator.get_SolarCurrent()));
        float availableCurrent = estimator.get_CAvailable(BATTERY_CAPACITY_AH, ESTIMATOR_RUNTIME_HOURS, ESTIMATOR_FORCED_LOADS);
        estimatorDoc["available_current"] = sanitizeReading(availableCurrent);
        estimatorDoc["available_power_w"] = sanitizeReading(availableCurrent * currentBatteryVoltage);
        estimatorDoc["estimated_runtime_h"] = sanitizeReading(estimator.get_EstimatedRuntime(BATTERY_CAPACITY_AH, ESTIMATOR_FORCED_LOADS));
        estimatorDoc["energy_wh"] = sanitizeReading(currentSolarEnergy);
        estimatorDoc["time"] = timeStr;
        String estimatorJson;
        serializeJson(estimatorDoc, estimatorJson);
        mqtt.publish(TOPIC_ESTIMATOR_PUB, estimatorJson.c_str());
        Serial.printf("[%s] Published to %s: %s\n", timeStr.c_str(), TOPIC_ESTIMATOR_PUB, estimatorJson.c_str());
    }
}

// ------------------------------------------------------------
// Setup
// ------------------------------------------------------------
void setup() {
    Serial.begin(115200);

    if (!wifi.begin(WIFI_SSID, WIFI_PASSWORD)) {
        Serial.println("WiFi startup failed");
        while (true) {
            delay(1000);
        }
    }

    TimeManager::begin(NTP_SERVER, GMT_OFFSET_SEC, DAYLIGHT_OFFSET_SEC);

    mqtt.setBFS(&bfs);
    mqtt.begin(MQTT_HOST, MQTT_PORT, MQTT_CLIENT_ID);

    // Initialize battery and solar sensor state before telemetry publishing.
    initBattery();
    solar.begin();
    estimator.begin();

    bfsMutex = xSemaphoreCreateMutex();
    if (!bfsMutex) {
        Serial.println("Failed to create BFS mutex");
        while (true) {
            delay(1000);
        }
    }

    // Initialize the command queue and processor task (Option B)
    initCommandQueue(&bfs, &mqtt);

    if (!bfs.getRoot()) {
        Serial.println("Warning: BFS root not initialized. Optimisation will be skipped until tree is populated.");
    }

    // Temperature publishing task
    xTaskCreate(
        tempPublishTask,
        "TempPublish",
        4096,
        NULL,
        1,
        &tempTaskHandle
    );

    // Battery / Solar / Estimator telemetry publishing task
    xTaskCreate(
        powerPublishTask,
        "PowerPublish",
        8192,
        NULL,
        1,
        &powerTaskHandle
    );

    // Automatic optimisation task (every 6 seconds)
    xTaskCreate(
        optimisePublishTask,
        "OptimisePublish",
        10240,                              // ample stack for BFS algorithm
        NULL,
        1,
        &optimiseTaskHandle
    );
}

void loop() {
    vTaskDelay(pdMS_TO_TICKS(1000));
}