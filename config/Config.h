#ifndef CONFIG_H
#define CONFIG_H

// =====================================================================
//  WiFi Settings
//  Replace with your own network credentials.
// =====================================================================
extern const char* WIFI_SSID;      // WiFi network name
extern const char* WIFI_PASSWORD;  // WiFi password

// =====================================================================
//  MQTT Broker Settings
//  The MQTT broker address, port, and the client ID that identifies
//  this ESP32 on the network.
// =====================================================================
extern const char* MQTT_HOST;      // Broker hostname
extern const int   MQTT_PORT;      // WebSocket Secure (WSS) port
extern const char* MQTT_CLIENT_ID; // Unique client identifier

// =====================================================================
//  MQTT Topics
//  These are the topics used for communication:
//    - TOPIC_TEST_SUB   : the ESP32 listens here for test messages
//    - TOPIC_SENSOR_PUB : the ESP32 publishes sensor data (temperature) here
//    - TOPIC_PING       : the ESP32 sends a "ping" heartbeat to this topic
// =====================================================================
extern const char* TOPIC_TEST_SUB;      // test messages
extern const char* TOPIC_SENSOR_PUB;    // temperature data
extern const char* TOPIC_BATTERY_PUB;   // battery telemetry
extern const char* TOPIC_SOLAR_PUB;     // solar telemetry
extern const char* TOPIC_ESTIMATOR_PUB; // power estimator telemetry
extern const char* TOPIC_PING;          // heartbeat

// =====================================================================
//  Relay Settings
//  Set RELAY_ACTIVE_HIGH to false for active-low relay modules.
// =====================================================================
extern const bool RELAY_ACTIVE_HIGH;

// =====================================================================
//  Energy settings
// =====================================================================
extern const float BATTERY_CAPACITY_AH;  // Total battery capacity in amp-hours

extern const float BATTERY_VOLTAGE_MAX;
extern const float BATTERY_VOLTAGE_MIN;
extern const float INVERTER_MAX_AMPS;
extern const int   SOLAR_VOLTAGE_ADC_PIN;
extern const float SOLAR_VOLTAGE_DIVIDER_RATIO;
extern const float ESTIMATOR_RUNTIME_HOURS; // Default runtime horizon for estimator calculations
extern const float ESTIMATOR_FORCED_LOADS;  // Forced load current in amps for estimator budget

// =====================================================================
//  Time Settings (Zambia)
//  Zambia uses Central Africa Time (CAT), UTC+2, and does not observe
//  Daylight Saving Time (DST).
// =====================================================================
extern const char* NTP_SERVER;     // NTP server to synchronise time
extern const long  GMT_OFFSET_SEC; // 7200 seconds = UTC+2
extern const int   DAYLIGHT_OFFSET_SEC; // No daylight saving in Zambia

// =====================================================================
//  Timing Intervals
//  How often the ESP32 publishes sensor data and sends a ping.
//  Values are in milliseconds.
// =====================================================================
// extern const unsigned long PUBLISH_INTERVAL = 5000;   // Publish temperature every 5 seconds
// extern const unsigned long PING_INTERVAL    = 15000;  // Send ping every 15 seconds
// // Intervals (ms)
extern const unsigned long PUBLISH_INTERVAL;  // Publish temperature every 60 seconds (1 minute)
extern const unsigned long PING_INTERVAL;     // Send ping every 15 seconds (unchanged)
extern const unsigned long STATUS_LIST_INTERVAL; // Tree list every 6s (new)
extern const unsigned long OPTIMISE_INTERVAL;    // Run optimisation every 6s (new)

// Default available current for automatic optimisation (Amps)
extern const float OPTIMISE_CURRENT;  // You can change this

#endif