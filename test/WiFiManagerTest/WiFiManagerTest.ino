#include <Arduino.h>
#include "../../config/Config.h"
#include "../../include/WiFiManager.h"

WiFiManager wifi;

void setup() {
    Serial.begin(115200);
    while (!Serial);

    Serial.println("WiFiManager connectivity test starting...");

    wifi.begin(WIFI_SSID, WIFI_PASSWORD);
    bool connected = wifi.isConnected();
    Serial.printf("WiFi connected: %s\n", connected ? "PASS" : "FAIL");

    if (!connected) {
        Serial.println("WiFiManager test failed: check SSID/password and network.");
    } else {
        Serial.println("WiFiManager test succeeded.");
    }
}

void loop() {
    // Nothing else needed for this connectivity test.
}
