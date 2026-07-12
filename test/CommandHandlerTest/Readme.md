# CommandHandler Test

This sketch validates the `processCommand()` logic in `src/CommandHandler.cpp`.

## What it checks
- adding single branch and leaf nodes
- bulk add via JSON array
- updating node properties
- executing the optimization command
- relay state toggles via JSON
- deleting a node
- handling invalid JSON payloads

## How to run
1. Open `test/CommandHandlerTest/CommandHandlerTest.ino` in the Arduino IDE or use the Arduino CLI.
2. Select the ESP32 board and the correct serial port.
3. Upload the sketch.
4. Open the serial monitor at `115200` baud.

## Expected output
The sketch prints pass/fail results for each command handler scenario and a final summary.

## Notes
- This test is a functional command parser and control-flow test.
- It uses the existing `BestFirstSearch` implementation directly.
- It does not exercise actual MQTT broker traffic.
- `processCommand()` is invoked with command topics directly to validate behavior.
