/*
  CommandHandlerTest.ino
  Functional test for the command queue and update handling (Option B).

  This sketch runs on the ESP32 and verifies that enqueueing an update
  command is processed by the command processor task and that the
  BestFirstSearch node is updated accordingly.
*/

#include "../../config/Config.h"
#include "../../include/CommandHandler.h"
#include "../../include/MQTTManager.h"
#include "../../lib/src/BestFirstSearch.h"

// Include implementations so the test sketch links standalone
#include "../../lib/src/BestFirstSearch.cpp"
#include "../../src/CommandHandler.cpp"
#include "../../src/MQTTManager.cpp"
#include "../../src/TimeManager.cpp"
#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>

// Provide a bfsMutex instance expected by CommandHandler.cpp
SemaphoreHandle_t bfsMutex = NULL;

MQTTManager mqtt;
BestFirstSearch bfs;

void setup() {
  Serial.begin(115200);
  delay(1000);

  // Create a sample node to update
  bfs.createAndAddNode("Main_DB", "Lamp", 1.0, 1, -1, 0.1, false);
  Serial.println("Created node 'Lamp' with amps=1.0");

  // Initialize command queue (uses global bfs + mqtt references)
  initCommandQueue(&bfs, &mqtt);

  // Enqueue an update command to change Lamp's amps to 2.5
  const char* topic = "esp32/command/config/update";
  String payload = "{\"name\":\"Lamp\", \"amps\":2.5, \"priority\":5, \"forced\":false}";
  bool enqueued = enqueueCommand(topic, payload);
  Serial.printf("Enqueue update command: %s\n", enqueued ? "OK" : "FAILED");

  // Give some time for the command processor task to run
  delay(2000);

  // Check the node state
  Node* n = bfs.getNode("Lamp");
  if (n) {
    Serial.printf("Lamp after update: amps=%.2f, priority=%d, forced=%s\n",
                  n->currentDraw, n->priority, n->isForced ? "true" : "false");
    if (abs(n->currentDraw - 2.5) < 0.001 && n->priority == 5) {
      Serial.println("TEST PASSED: update applied");
    } else {
      Serial.println("TEST FAILED: update not applied correctly");
    }
  } else {
    Serial.println("TEST FAILED: node 'Lamp' missing");
  }
}

void loop() {
  // nothing to do
  vTaskDelay(pdMS_TO_TICKS(1000));
}
