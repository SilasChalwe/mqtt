#include "../config/Config.h"

// WiFi settings
const char* WIFI_SSID = "covianhive";
const char* WIFI_PASSWORD = "********";

// MQTT broker settings
const char* MQTT_HOST = "mqtt.covianhive.tech";
const int MQTT_PORT = 443;
const char* MQTT_CLIENT_ID = "esp32-client";

// MQTT topics
const char* TOPIC_TEST_SUB = "esp32/command/test";
const char* TOPIC_SENSOR_PUB = "esp32/telemetry/temperature";
const char* TOPIC_BATTERY_PUB = "esp32/telemetry/battery";
const char* TOPIC_SOLAR_PUB = "esp32/telemetry/solar";
const char* TOPIC_ESTIMATOR_PUB = "esp32/telemetry/estimator";
const char* TOPIC_PING = "esp32/telemetry/ping";

// Relay settings
const bool RELAY_ACTIVE_HIGH = true;

// Energy settings
const float BATTERY_CAPACITY_AH = 100.0f;
const float BATTERY_VOLTAGE_MAX = 4.20f;
const float BATTERY_VOLTAGE_MIN = 3.40f;
const float INVERTER_MAX_AMPS = 15.0f;
const int SOLAR_VOLTAGE_ADC_PIN = 35;
const float SOLAR_VOLTAGE_DIVIDER_RATIO = 11.0f;
const float ESTIMATOR_RUNTIME_HOURS = 1.0f;
const float ESTIMATOR_FORCED_LOADS = 1.0f;

// Time settings
const char* NTP_SERVER = "pool.ntp.org";
const long GMT_OFFSET_SEC = 2 * 3600;
const int DAYLIGHT_OFFSET_SEC = 0;

// Timing intervals
const unsigned long PUBLISH_INTERVAL = 60000;
const unsigned long PING_INTERVAL = 15000;
const unsigned long STATUS_LIST_INTERVAL = 6000;
const unsigned long OPTIMISE_INTERVAL = 6000;
const float OPTIMISE_CURRENT = 20.0f;
