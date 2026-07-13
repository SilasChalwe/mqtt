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

The firmware subscribes to the wildcard topic esp32/# and handles the following command topics.

#### Quick payload reference

| Topic | Payload type | Required fields | Optional fields |
| --- | --- | --- | --- |
| esp32/command/config/add | JSON object | name, amps | parent, voltage, priority, pin, friction, type |
| esp32/command/config/add_bulk | JSON array of objects | each object needs name and amps | parent, voltage, priority, pin, friction, type |
| esp32/command/config/update | JSON object | name | amps, voltage, priority, type |
| esp32/command/config/delete | JSON object | name | none |
| esp32/command/config/get_node | JSON object | name | none |
| esp32/command/config/list | empty payload | none | none |
| esp32/command/config/pin_status | empty payload | none | none |
| esp32/command/config/save_tree | empty payload | none | none |
| esp32/command/config/tree | empty payload | none | none |
| esp32/command/control/execute | JSON object or plain text | none | available_current |
| esp32/command/control/relay/<node_name> | plain text or JSON object | state | none |

#### Configuration commands

- Topic: esp32/command/config/add
  - Purpose: Add one load node to the tree.
  - Exact payload shape:
    ```json
    {
      "parent": "Main_DB",
      "name": "Fridge",
      "amps": 1.2,
      "voltage": 230,
      "priority": 5,
      "pin": 17,
      "friction": 0.1,
      "type": "auto"
    }
    ```
  - Field notes:
    - name must be unique across the whole tree.
    - amps is the load current draw in amps.
    - pin is optional; use -1 if the node should not control a relay.
    - type accepts "auto", "fixed", true, or false.
    - parent defaults to Main_DB if omitted.

- Topic: esp32/command/config/add_bulk
  - Purpose: Add several load nodes in one request.
  - Exact payload shape:
    ```json
    [
      {
        "parent": "Main_DB",
        "name": "Fridge",
        "amps": 1.2,
        "voltage": 230,
        "priority": 5,
        "pin": 17,
        "friction": 0.1,
        "type": "auto"
      },
      {
        "parent": "Main_DB",
        "name": "Lights",
        "amps": 0.8,
        "voltage": 230,
        "priority": 3,
        "pin": 18,
        "friction": 0.05,
        "type": "fixed"
      }
    ]
    ```
  - Each array item uses the same fields as the single-add payload.

- Topic: esp32/command/config/update
  - Purpose: Update an existing node.
  - Exact payload shape:
    ```json
    {
      "name": "Fridge",
      "amps": 1.5,
      "voltage": 230,
      "priority": 7,
      "type": "fixed"
    }
    ```
  - The node identified by name must already exist.

- Topic: esp32/command/config/delete
  - Purpose: Delete a load node by name.
  - Exact payload shape:
    ```json
    {
      "name": "Fridge"
    }
    ```
  - The root node Main_DB cannot be deleted.

- Topic: esp32/command/config/list
  - Purpose: Return the full list of nodes.
  - Payload: send an empty message, or no payload at all.

- Topic: esp32/command/config/pin_status
  - Purpose: Return the used and available relay pins.
  - Payload: send an empty message, or no payload at all.

- Topic: esp32/command/config/get_node
  - Purpose: Return details for one node.
  - Exact payload shape:
    ```json
    {
      "name": "Fridge"
    }
    ```

- Topic: esp32/command/config/save_tree
  - Purpose: Save the current tree to LittleFS as /tree.txt.
  - Payload: send an empty message, or no payload at all.

- Topic: esp32/command/config/tree
  - Purpose: Return the full nested tree as JSON.
  - Payload: send an empty message, or no payload at all.

#### Control commands

- Topic: esp32/command/control/execute
  - Purpose: Run optimisation immediately.
  - Exact payload shape (JSON):
    ```json
    {
      "available_current": 10
    }
    ```
  - Plain-text alternative:
    ```text
    10
    ```
  - If the payload is missing or invalid, the firmware falls back to a default value.

- Topic: esp32/command/control/relay/<node_name>
  - Purpose: Toggle a relay-controlled node manually.
  - Exact payload shape (plain text):
    ```text
    on
    ```
    ```text
    off
    ```
    ```text
    auto
    ```
  - JSON alternative:
    ```json
    {
      "state": "on"
    }
    ```
  - Replace <node_name> with the actual node name, for example:
    ```text
    esp32/command/control/relay/Fridge
    ```

### Publication topics (data the device sends)

#### Telemetry topics

- Topic: esp32/telemetry/temperature
  - Payload example:
    ```json
    {
      "temp": 24.5,
      "time": "2026-07-13 12:34:56"
    }
    ```

- Topic: esp32/telemetry/battery
  - Payload example:
    ```json
    {
      "voltage": 3.92,
      "soc": 82.5,
      "battery_capacity_ah": 100,
      "time": "2026-07-13 12:34:56"
    }
    ```

- Topic: esp32/telemetry/solar
  - Payload example:
    ```json
    {
      "voltage": 18.4,
      "current": 2.1,
      "power": 38.6,
      "energy_wh": 12.8,
      "time": "2026-07-13 12:34:56"
    }
    ```

- Topic: esp32/telemetry/estimator
  - Payload example:
    ```json
    {
      "battery_soc": 82.5,
      "battery_voltage": 3.92,
      "solar_current": 2.1,
      "solar_power_w": 38.6,
      "battery_power_w": 8.2,
      "available_current": 10,
      "available_power_w": 39.2,
      "estimated_runtime_h": 1,
      "energy_wh": 12.8,
      "time": "2026-07-13 12:34:56"
    }
    ```

#### Status topics

- Topic: esp32/status/error
  - Payload example:
    ```json
    {
      "error": "Missing 'name'",
      "request_topic": "esp32/command/config/delete"
    }
    ```

- Topic: esp32/status/config/node_added
  - Payload example:
    ```text
    Fridge
    ```

- Topic: esp32/status/config/bulk_added
  - Payload example:
    ```text
    Added 2 loads, skipped 0 invalid entries
    ```

- Topic: esp32/status/config/update_done
  - Payload example:
    ```text
    Fridge
    ```

- Topic: esp32/status/config/delete_done
  - Payload example:
    ```text
    Fridge
    ```

- Topic: esp32/status/config/tree_saved
  - Payload example:
    ```text
    OK
    ```

- Topic: esp32/status/config/tree
  - Payload example:
    ```json
    {
      "name": "Main_DB",
      "children": []
    }
    ```

- Topic: esp32/status/config/list
  - Payload example:
    ```json
    [
      {
        "name": "Fridge",
        "amps": 1.2,
        "pin": 17,
        "type": "auto"
      }
    ]
    ```

- Topic: esp32/status/config/get_node
  - Payload example:
    ```json
    {
      "name": "Fridge",
      "amps": 1.2,
      "pin": 17,
      "type": "auto"
    }
    ```

- Topic: esp32/status/config/pin_status
  - Payload example:
    ```json
    {
      "used": [17, 18],
      "available": [2, 4, 5, 12, 13, 14, 15, 16, 19, 21, 22, 23, 25, 26, 27, 32, 33]
    }
    ```

- Topic: esp32/status/control/execute_received
  - Payload example:
    ```text
    Running optimisation...
    ```

- Topic: esp32/status/control/result
  - Payload example:
    ```json
    {
      "timestamp": "2026-07-13 12:34:56",
      "available_current": 10,
      "loads": [
        {
          "name": "Fridge",
          "parent": "Main_DB",
          "active": true,
          "amps": 1.2,
          "pin": 17
        }
      ]
    }
    ```

- Topic: esp32/status/control/relay_changed
  - Payload example:
    ```json
    {
      "topic": "esp32/command/control/relay/Fridge",
      "node": "Fridge",
      "state": "on"
    }
    ```

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

This repository is licensed under the MIT License.

Copyright (c) 2026 Silas Chalwe, CEO and Founder of Covian Hive

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
