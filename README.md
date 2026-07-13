# MQTT Load Optimisation Firmware for ESP32

This project is an ESP32-based firmware that manages a tree of electrical loads, publishes telemetry over MQTT, and can automatically switch relays to optimise power usage based on available current and battery/solar conditions.

It is designed for a small energy-management system where loads are organised in a hierarchical tree, each node can represent a branch or a relay-controlled appliance, and the firmware decides which loads to keep on or off with a best-first-search strategy.

## What this firmware does

- Connects to Wi-Fi and an MQTT broker over WSS.
- Publishes battery, solar, estimator, and temperature telemetry.
- Accepts MQTT commands to add, update, delete, inspect, and list loads.
- Persists the load tree to LittleFS as JSON so the configuration survives reboots.
- Applies relay switching for supported loads.
- Runs automatic optimisation to keep the system within a power budget.

## Main features

- Hierarchical load tree with parent/child relationships.
- Per-load settings such as current draw, priority, relay pin, friction, and fixed/auto mode.
- Relay pin reuse protection and unique load-name validation.
- Telemetry reporting for battery voltage, state of charge, solar voltage/current/power/energy, and estimator output.
- Manual relay control via MQTT topics.
- Automatic optimisation using a best-first-search over the load tree.

## Project structure

- mqtt.ino: main sketch and task orchestration.
- config/Config.h: configuration declarations.
- src/Config.cpp: runtime configuration values such as Wi-Fi credentials, MQTT broker settings, topics, relay options, and timing intervals.
- include/: public headers for battery, solar, MQTT, command handling, relay control, tree storage, and node modelling.
- src/: implementation of the firmware modules.
- lib/src/: BestFirstSearch implementation used by the optimiser.
- test/: unit tests for the search and load-mode migration logic.

## Hardware assumptions

This firmware expects an ESP32 board connected to:

- An INA219 current/voltage sensor for battery and solar measurements.
- One or more relay modules for switching loads.
- A battery and a solar input source, depending on your deployment.

The code is written around the Arduino ESP32 environment and uses libraries such as ArduinoJson, Adafruit INA219, and the ESP-IDF MQTT client stack.

## Configuration

Before flashing the firmware, review and update the values in src/Config.cpp:

- Wi-Fi SSID and password.
- MQTT host, port, and client ID.
- Topic names.
- Relay active-high/active-low behaviour.
- Battery capacity, voltage thresholds, and estimator defaults.
- Publish and optimisation intervals.

Important: the current repository contains example values and should be adjusted for your own network and hardware.

## Build and upload

1. Install the Arduino IDE or PlatformIO with ESP32 support.
2. Install the required libraries:
   - ArduinoJson
   - Adafruit INA219
3. Open mqtt.ino in the Arduino IDE.
4. Select your ESP32 board and COM port.
5. Compile and upload the sketch.

If you use PlatformIO, the project layout is still compatible with the sketch-based structure, but the Arduino IDE workflow is the most direct fit for this repository.

## Runtime behaviour

On boot the firmware:

1. Starts Wi-Fi.
2. Synchronises time using NTP.
3. Initialises the load tree from persisted storage if available.
4. Starts the MQTT client and subscribes to the command wildcard topic.
5. Begins sensor reads and starts background tasks for telemetry and optimisation.

The firmware publishes telemetry at the configured interval and may run optimisation automatically based on the available current estimate.

## MQTT interface

The device subscribes to command topics under the esp32/command namespace and publishes telemetry and status updates under the esp32/telemetry and esp32/status namespaces.

### Subscription topics (commands the device listens for)

The firmware subscribes to the wildcard topic esp32/# and handles the following command topics:

#### Configuration commands

- esp32/command/config/add
  - Add a single load.
  - Payload example:
    {
      "name": "Fridge",
      "amps": 1.2,
      "pin": 17,
      "priority": 5,
      "type": "auto"
    }

- esp32/command/config/add_bulk
  - Add multiple loads in a JSON array.

- esp32/command/config/update
  - Update an existing load's parameters.

- esp32/command/config/delete
  - Remove a load by name.

- esp32/command/config/list
  - Return the current load tree.

- esp32/command/config/pin_status
  - Report used and available relay pins.

- esp32/command/config/get_node
  - Retrieve details for a specific node.

- esp32/command/config/save_tree
  - Persist the current tree to LittleFS.

- esp32/command/config/tree
  - Return the full nested tree as JSON.

#### Control commands

- esp32/command/control/execute
  - Run optimisation now.
  - Optional payload example:
    {"available_current": 10}

- esp32/command/control/relay/<node_name>
  - Manually force a relay node to on, off, or auto.
  - Payload examples:
    - on
    - off
    - auto

### Publication topics (data the device sends)

#### Telemetry topics

- esp32/telemetry/temperature
- esp32/telemetry/battery
- esp32/telemetry/solar
- esp32/telemetry/estimator

#### Status topics

- esp32/status/error
- esp32/status/config/node_added
- esp32/status/config/bulk_added
- esp32/status/config/update_done
- esp32/status/config/delete_done
- esp32/status/config/tree_saved
- esp32/status/config/tree
- esp32/status/config/list
- esp32/status/config/get_node
- esp32/status/config/pin_status
- esp32/status/control/execute_received
- esp32/status/control/result
- esp32/status/control/relay_changed

## Optimisation logic

The optimisation engine builds a load tree and uses a best-first-search approach to determine which loads should be enabled given the available current budget. It considers:

- load current draw
- load priority
- relay state
- wire friction / cost
- current available current from the estimator

The resulting relay actions are then applied and published back over MQTT.

## Persistence

The load tree can be saved to LittleFS as JSON. The storage implementation is in src/TreeStorage.cpp and writes a file named /tree.txt when the filesystem is available.

## Testing

The repository includes basic tests under the test/ folder for the search logic and load-mode migration behaviour.

## Notes

- This firmware is intended for embedded and experimental energy-management use.
- Relay pin assignments and load topology should be validated carefully before deployment.
- Always verify the hardware wiring and power limits before operating real appliances.

## License

This repository is private and proprietary. All code, design, and associated materials are the intellectual property of the owner and may not be copied, distributed, modified, or used outside the authorized scope without explicit written permission.
