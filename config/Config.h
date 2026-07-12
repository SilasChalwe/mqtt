#ifndef CONFIG_H
#define CONFIG_H

// =====================================================================
//  WiFi Settings
//  Replace with your own network credentials.
// =====================================================================
const char* WIFI_SSID     = "covianhive";   // WiFi network name
const char* WIFI_PASSWORD = "********";     // WiFi password

// =====================================================================
//  MQTT Broker Settings
//  The MQTT broker address, port, and the client ID that identifies
//  this ESP32 on the network.
// =====================================================================
const char* MQTT_HOST      = "mqtt.covianhive.tech";   // Broker hostname
const int   MQTT_PORT      = 443;                       // WebSocket Secure (WSS) port
const char* MQTT_CLIENT_ID = "esp32-client";            // Unique client identifier

// =====================================================================
//  MQTT Topics
//  These are the topics used for communication:
//    - TOPIC_TEST_SUB   : the ESP32 listens here for test messages
//    - TOPIC_SENSOR_PUB : the ESP32 publishes sensor data (temperature) here
//    - TOPIC_PING       : the ESP32 sends a "ping" heartbeat to this topic
// =====================================================================
const char* TOPIC_TEST_SUB      = "esp32/command/test";            // test messages
const char* TOPIC_SENSOR_PUB    = "esp32/telemetry/temperature";   // temperature data
const char* TOPIC_BATTERY_PUB   = "esp32/telemetry/battery";       // battery telemetry
const char* TOPIC_SOLAR_PUB     = "esp32/telemetry/solar";         // solar telemetry
const char* TOPIC_ESTIMATOR_PUB = "esp32/telemetry/estimator";     // power estimator telemetry
const char* TOPIC_PING          = "esp32/telemetry/ping";          // heartbeat

// =====================================================================
//  Energy settings
// =====================================================================
const float BATTERY_CAPACITY_AH = 100.0;  // Total battery capacity in amp-hours
const float ESTIMATOR_RUNTIME_HOURS = 1.0; // Default runtime horizon for estimator calculations
const float ESTIMATOR_FORCED_LOADS = 1.0;  // Forced load current in amps for estimator budget

// =====================================================================
//  Time Settings (Zambia)
//  Zambia uses Central Africa Time (CAT), UTC+2, and does not observe
//  Daylight Saving Time (DST).
// =====================================================================
const char* NTP_SERVER = "pool.ntp.org";     // NTP server to synchronise time
const long  GMT_OFFSET_SEC = 2 * 3600;       // 7200 seconds = UTC+2
const int   DAYLIGHT_OFFSET_SEC = 0;         // No daylight saving in Zambia

// =====================================================================
//  Timing Intervals
//  How often the ESP32 publishes sensor data and sends a ping.
//  Values are in milliseconds.
// =====================================================================
// const unsigned long PUBLISH_INTERVAL = 5000;   // Publish temperature every 5 seconds
// const unsigned long PING_INTERVAL    = 15000;  // Send ping every 15 seconds
// // Intervals (ms)
const unsigned long PUBLISH_INTERVAL = 60000;  // Publish temperature every 60 seconds (1 minute)
const unsigned long PING_INTERVAL    = 15000;  // Send ping every 15 seconds (unchanged)
const unsigned long STATUS_LIST_INTERVAL  = 6000;   // Tree list every 6s (new)
const unsigned long OPTIMISE_INTERVAL     = 6000;   // Run optimisation every 6s (new)

// Default available current for automatic optimisation (Amps)
const float       OPTIMISE_CURRENT        = 20.0;   // You can change this

#endif