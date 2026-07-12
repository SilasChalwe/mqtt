#include "../include/WiFiManager.h"
#include <Arduino.h>                 // required for Serial, delay, etc.

/**
 * begin()
 * 
 * Attempts to connect to the specified WiFi network.
 * If the connection fails after 15 seconds, the ESP32 creates
 * its own Access Point named "ESP32-Setup" so you can still
 * connect to it for debugging or reconfiguration.
 */
bool WiFiManager::begin(const char* ssid, const char* password) {
    // Set WiFi mode to station (client)
    WiFi.mode(WIFI_STA);
    WiFi.begin(ssid, password);

    Serial.print("Connecting to WiFi");

    int attempts = 0;
    // Try to connect for up to 30 * 500 ms = 15 seconds
    while (WiFi.status() != WL_CONNECTED && attempts < 30) {
        delay(500);
        Serial.print(".");
        attempts++;
    }

    // If connected successfully
    if (WiFi.status() == WL_CONNECTED) {
        Serial.println("\nWiFi connected");
        Serial.print("IP address: ");
        Serial.println(WiFi.localIP());
        return true;
    }

    // If connection failed, start an Access Point
    Serial.println("\nWiFi connection failed. Starting Access Point...");

    // Switch to Access Point mode
    WiFi.mode(WIFI_AP);
    // Start a simple open network – no password
    WiFi.softAP("ESP32-Setup");

    Serial.print("AP IP address: ");
    Serial.println(WiFi.softAPIP());
    return false;
}

/**
 * isConnected()
 *
 * Returns true if the ESP32 has either:
 * - successfully connected to an external WiFi network, or
 * - at least one device is connected to its own Access Point.
 */
bool WiFiManager::isConnected() {
    return (WiFi.status() == WL_CONNECTED) || (WiFi.softAPgetStationNum() > 0);
}