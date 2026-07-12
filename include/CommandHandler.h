#ifndef COMMAND_HANDLER_H
#define COMMAND_HANDLER_H

#include <Arduino.h>
#include "MQTTManager.h"   // for the publish() method – we need an MQTTManager reference

class BestFirstSearch;     // forward declaration

extern SemaphoreHandle_t bfsMutex;

void processCommand(const String& topic, const String& payload,
                    BestFirstSearch* bfs, MQTTManager* mqtt);

// Command queue API (Option B)
typedef struct CommandMessage {
    String topic;
    String payload;
} CommandMessage;

extern QueueHandle_t commandQueue;

// Initialize the command queue and processor task. Call once from setup().
void initCommandQueue(BestFirstSearch* bfs, MQTTManager* mqtt);

// Enqueue an incoming MQTT command for processing by the command task.
bool enqueueCommand(const String& topic, const String& payload);

#endif