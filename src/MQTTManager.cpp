#include <Arduino.h>
#include "../include/MQTTManager.h"
#include "../include/CommandHandler.h"      // <-- ADD THIS LINE
#include "esp_crt_bundle.h"
#include <stdio.h>

void MQTTManager::begin(const char* host, int port, const char* client_id) {
    esp_mqtt_client_config_t mqtt_cfg = {};
    mqtt_cfg.broker.address.hostname = host;
    mqtt_cfg.broker.address.transport = MQTT_TRANSPORT_OVER_WSS;
    mqtt_cfg.broker.address.port = port;
    mqtt_cfg.broker.verification.crt_bundle_attach = esp_crt_bundle_attach;
    mqtt_cfg.session.protocol_ver = MQTT_PROTOCOL_V_3_1_1;
    mqtt_cfg.credentials.client_id = client_id;

    client = esp_mqtt_client_init(&mqtt_cfg);
    if (client == nullptr) {
        Serial.println("Failed to initialise MQTT client");
        return;
    }

    esp_mqtt_client_register_event(client, MQTT_EVENT_ANY, &MQTTManager::eventHandler, this);
    esp_mqtt_client_start(client);
}

void MQTTManager::subscribe(const char* topic, int qos) {
    if (client) {
        esp_mqtt_client_subscribe(client, topic, qos);
    }
}

void MQTTManager::publish(const char* topic, const char* payload, int qos, bool retain) {
    if (!client) {
        Serial.printf("MQTT publish failed: client uninitialized for topic %s\n", topic ? topic : "(null)");
        return;
    }
    if (!topic || topic[0] == '\0') {
        Serial.println("MQTT publish failed: empty topic");
        return;
    }
    if (!payload) {
        payload = "";
    }
    esp_mqtt_client_publish(client, topic, payload, 0, qos, retain);
}

void MQTTManager::ping() {
    static int pingCount = 0;
    char payload[32];
    snprintf(payload, sizeof(payload), "ping %d", pingCount++);
    publish("ping", payload);
    Serial.printf("PING sent: %s\n", payload);
}

void MQTTManager::loop() {
    // MQTT client runs in its own task
}

void MQTTManager::eventHandler(void* handler_args, esp_event_base_t base,
                               int32_t event_id, void* event_data) {
    auto* self = static_cast<MQTTManager*>(handler_args);
    self->handleEvent(event_id, (esp_mqtt_event_handle_t)event_data);
}

void MQTTManager::handleEvent(int32_t event_id, esp_mqtt_event_handle_t event) {
    switch ((esp_mqtt_event_id_t)event_id) {
        case MQTT_EVENT_CONNECTED:
            Serial.println("MQTT connected!");
            subscribe("test/topic");
            subscribe("esp32/#");
            break;

        case MQTT_EVENT_DATA: {            // <-- OPEN BRACE HERE (FIX)
            String topic   = String(event->topic, event->topic_len);
            String payload = String(event->data, event->data_len);
            Serial.printf("Topic: %s, Payload: %s\n", topic.c_str(), payload.c_str());

            if (topic == "test/topic") {
                Serial.println("--- Incoming test message ---");
                Serial.println(payload);
            }

            // Delegate to CommandHandler via queue (non-blocking for MQTT thread)
            if (!enqueueCommand(topic, payload)) {
                // fallback: run directly if enqueue failed
                processCommand(topic, payload, bfs, this);
            }
            break;
        }                                  // <-- CLOSE BRACE HERE

        case MQTT_EVENT_ERROR:
            Serial.println("MQTT error");
            break;

        default:
            break;
    }
}