#include <Arduino.h>
#include "../../config/Config.h"
#include "../../include/WiFiManager.h"
#include "../../include/MQTTManager.h"

WiFiManager wifi;
MQTTManager mqtt;

void setup() {
    Serial.begin(115200);
    while (!Serial);

    Serial.println("MQTTManager connectivity test starting...");
    wifi.begin(WIFI_SSID, WIFI_PASSWORD);
    bool connected = wifi.isConnected();
    Serial.printf("WiFi connected: %s\n", connected ? "yes" : "no");

    if (!connected) {
        Serial.println("Broker connectivity test requires WiFi. Check your credentials and network.");
        return;
    }

    mqtt.begin(MQTT_HOST, MQTT_PORT, MQTT_CLIENT_ID);
    Serial.println("MQTT begin() called; waiting for broker connection event...");
}

void loop() {
    mqtt.loop();
    static unsigned long lastPing = 0;

    if (millis() - lastPing > 5000) {
        mqtt.ping();
        lastPing = millis();
    }

    delay(100);
}
