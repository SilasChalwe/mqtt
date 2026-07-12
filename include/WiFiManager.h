#ifndef WIFI_MANAGER_H
#define WIFI_MANAGER_H

#include <WiFi.h>

class WiFiManager {
public:
    bool begin(const char* ssid, const char* password);
    bool isConnected();
};

#endif