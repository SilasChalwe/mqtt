# WiFi Manager Test

This sketch verifies WiFi connectivity using `WiFiManager` and credentials from `config/Config.h`.

## What it checks
- WiFi connection can be established
- `WiFiManager::isConnected()` returns true after connection
- serial output reports pass/fail status

## How to run
1. Open `test/WiFiManagerTest/WiFiManagerTest.ino` in the Arduino IDE or use the Arduino CLI.
2. Select the ESP32 board and the correct serial port.
3. Upload the sketch.
4. Open the serial monitor at `115200` baud.

## Expected output
- `WiFiManager connectivity test starting...`
- `WiFi connected: PASS`
- `WiFiManager test succeeded.`

## Notes
- This test requires the correct `WIFI_SSID` and `WIFI_PASSWORD` values in `config/Config.h`.
- If the test fails, confirm the network credentials and WiFi coverage.
- This is a functional connectivity test, not a long-duration network stability test.
