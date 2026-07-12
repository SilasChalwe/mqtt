# MQTT Manager Test

This sketch verifies the MQTT broker connection and basic MQTT client lifecycle.

## What it checks
- WiFi connection succeeds using `config/Config.h` credentials
- MQTT client `begin()` is invoked with broker settings
- broker connection events are handled by `MQTTManager`
- periodic `ping()` messages are sent so broker activity is visible

## How to run
1. Open `test/MQTTManagerTest/MQTTManagerTest.ino` in the Arduino IDE or use the Arduino CLI.
2. Select the ESP32 board and the correct serial port.
3. Upload the sketch.
4. Open the serial monitor at `115200` baud.

## Expected output
- `WiFi connected: yes`
- `MQTT begin() called; waiting for broker connection event...`
- `MQTT connected!` once the broker handshake completes
- `PING sent: ...` every 5 seconds

## Notes
- This test requires an active MQTT broker as configured in `config/Config.h`.
- If the broker is unreachable, verify the host, port, and network connectivity.
- It is a functional connectivity test, not a full stress test.
