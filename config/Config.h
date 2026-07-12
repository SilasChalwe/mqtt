#ifndef CONFIG_H
#define CONFIG_H

// =====================================================================
//  WiFi Settings
//  Replace with your own network credentials.
// =====================================================================
static constexpr const char* WIFI_SSID     = "covianhive";   // WiFi network name
static constexpr const char* WIFI_PASSWORD = "********";     // WiFi password

// =====================================================================
//  MQTT Broker Settings
//  The MQTT broker address, port, and the client ID that identifies
//  this ESP32 on the network.
// =====================================================================
static constexpr const char* MQTT_HOST      = "mqtt.covianhive.tech";   // Broker hostname
static constexpr int MQTT_PORT      = 443;                       // WebSocket Secure (WSS) port
static constexpr const char* MQTT_CLIENT_ID = "esp32-client";            // Unique client identifier

// =====================================================================
//  MQTT Topics
//  These are the topics used for communication:
//    - TOPIC_TEST_SUB   : the ESP32 listens here for test messages
//    - TOPIC_SENSOR_PUB : the ESP32 publishes sensor data (temperature) here
//    - TOPIC_PING       : the ESP32 sends a "ping" heartbeat to this topic
// =====================================================================
static constexpr const char* TOPIC_TEST_SUB      = "esp32/command/test";            // test messages
static constexpr const char* TOPIC_SENSOR_PUB    = "esp32/telemetry/temperature";   // temperature data
static constexpr const char* TOPIC_BATTERY_PUB   = "esp32/telemetry/battery";       // battery telemetry
static constexpr const char* TOPIC_SOLAR_PUB     = "esp32/telemetry/solar";         // solar telemetry
static constexpr const char* TOPIC_ESTIMATOR_PUB = "esp32/telemetry/estimator";     // power estimator telemetry
static constexpr const char* TOPIC_PING          = "esp32/telemetry/ping";          // heartbeat


// =====================================================================
//  Relay Settings
//  Set RELAY_ACTIVE_HIGH to false for active-low relay modules.
// =====================================================================
static constexpr bool RELAY_ACTIVE_HIGH = true;

// =====================================================================
//  Energy settings
// =====================================================================
static constexpr float BATTERY_CAPACITY_AH = 100.0;  // Total battery capacity in amp-hours

static constexpr float BATTERY_VOLTAGE_MAX = 4.20f;
static constexpr float BATTERY_VOLTAGE_MIN = 3.40f;
static constexpr float INVERTER_MAX_AMPS = 15.0f;
static constexpr int SOLAR_VOLTAGE_ADC_PIN = 35;
static constexpr float SOLAR_VOLTAGE_DIVIDER_RATIO = 11.0f;
static constexpr float ESTIMATOR_RUNTIME_HOURS = 1.0; // Default runtime horizon for estimator calculations
static constexpr float ESTIMATOR_FORCED_LOADS = 1.0;  // Forced load current in amps for estimator budget

// =====================================================================
//  Time Settings (Zambia)
//  Zambia uses Central Africa Time (CAT), UTC+2, and does not observe
//  Daylight Saving Time (DST).
// =====================================================================
static constexpr const char* NTP_SERVER = "pool.ntp.org";     // NTP server to synchronise time
static constexpr long GMT_OFFSET_SEC = 2 * 3600;       // 7200 seconds = UTC+2
static constexpr int DAYLIGHT_OFFSET_SEC = 0;         // No daylight saving in Zambia

// =====================================================================
//  Timing Intervals
//  How often the ESP32 publishes sensor data and sends a ping.
//  Values are in milliseconds.
// =====================================================================
// static constexpr unsigned long PUBLISH_INTERVAL = 5000;   // Publish temperature every 5 seconds
// static constexpr unsigned long PING_INTERVAL    = 15000;  // Send ping every 15 seconds
// // Intervals (ms)
static constexpr unsigned long PUBLISH_INTERVAL = 60000;  // Publish temperature every 60 seconds (1 minute)
static constexpr unsigned long PING_INTERVAL    = 15000;  // Send ping every 15 seconds (unchanged)
static constexpr unsigned long STATUS_LIST_INTERVAL  = 6000;   // Tree list every 6s (new)
static constexpr unsigned long OPTIMISE_INTERVAL     = 6000;   // Run optimisation every 6s (new)

// Default available current for automatic optimisation (Amps)
static constexpr float OPTIMISE_CURRENT        = 20.0;   // You can change this

#endif
