#ifndef MQTT_MANAGER_H
#define MQTT_MANAGER_H

#include <mqtt_client.h>
// Forward declaration – no need to include the BFS header here
class BestFirstSearch;


class MQTTManager {
public:

    void begin(const char* host, int port, const char* client_id);
    void subscribe(const char* topic, int qos = 0);
    void publish(const char* topic, const char* payload, int qos = 0, bool retain = false);
    void ping();
    void loop();   // not needed for mqtt client but kept for consistency
    void setBFS(BestFirstSearch* engine) { bfs = engine; }


private:
    esp_mqtt_client_handle_t client = nullptr;
    static void eventHandler(void* handler_args, esp_event_base_t base,   int32_t event_id, void* event_data);
    void handleEvent(int32_t event_id, esp_mqtt_event_handle_t event);
    BestFirstSearch* bfs = nullptr;        // pointer to the untouched engine

};

#endif