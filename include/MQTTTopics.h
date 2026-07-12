#ifndef MQTT_TOPICS_H
#define MQTT_TOPICS_H

namespace MQTTTopics {
    static constexpr const char* CommandWildcard = "esp32/#";

    static constexpr const char* StatusError = "esp32/status/error";

    static constexpr const char* ConfigNodeAdded = "esp32/status/config/node_added";
    static constexpr const char* ConfigBulkAdded = "esp32/status/config/bulk_added";
    static constexpr const char* ConfigUpdateDone = "esp32/status/config/update_done";
    static constexpr const char* ConfigDeleteDone = "esp32/status/config/delete_done";
    static constexpr const char* ConfigTreeSaved = "esp32/status/config/tree_saved";
    static constexpr const char* ConfigTree = "esp32/status/config/tree";
    static constexpr const char* ConfigList = "esp32/status/config/list";
    static constexpr const char* ConfigGetNode = "esp32/status/config/get_node";
    static constexpr const char* ConfigPinStatus = "esp32/status/config/pin_status";

    static constexpr const char* ControlExecuteReceived = "esp32/status/control/execute_received";
    static constexpr const char* ControlResult = "esp32/status/control/result";
    static constexpr const char* ControlRelayChanged = "esp32/status/control/relay_changed";
}

#endif
