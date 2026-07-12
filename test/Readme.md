# BestFirstSearch Tests

This test sketch verifies core `BestFirstSearch` behavior using the existing library implementation without modifying `lib/`.

## Covered behavior
- Basic node creation and lookup
- Nested branch creation and invalid parent handling
- Updating existing nodes and rejecting updates to missing nodes
- `scout()` returning appliance candidates
- `execute()` activating candidates under capacity
- Removing a branch removes its subtree
- Refusing to remove the root node
- Empty result behavior after cleanup

## How to run
1. Open `test/BestFirstSearchTest.ino` in the Arduino IDE or use the Arduino CLI.
2. Select the ESP32 board and the correct serial port.
3. Upload the sketch.
4. Open the serial monitor at `115200` baud.

Additional test:
1. Open `test/CommandHandlerTest.ino` to verify the command queue and update handling.
2. Upload and monitor serial output for `TEST PASSED` or `TEST FAILED`.

## Expected output
The sketch prints a line for each test and a final `ALL PASSED` or `FAILED` summary.

## Notes
- This is a functional unit test, not a performance benchmark.
- It does not verify concurrency or long-term scalability on the ESP32.
- It validates correctness of the algorithmic flow and node management behavior.
