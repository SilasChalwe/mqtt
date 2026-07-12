# PowerEstimatorTest

This test sketch validates the solar/battery power estimator logic in the project.

## Covered behavior
- `SolarManager` current, voltage, and power calculations.
- `PowerEstimator::getAvailableCurrent()` budget calculation under battery SoC and solar input.
- `PowerEstimator::getEstimatedRuntimeHours()` for both net solar-supported and net discharge scenarios.
- Safety threshold behavior that prevents negative available current.

## How to run
1. Open `test/PowerEstimatorTest/PowerEstimatorTest.ino` in the Arduino IDE or use the Arduino CLI.
2. Select the ESP32 board and correct serial port.
3. Upload the sketch.
4. Open the serial monitor at `115200` baud.

## Expected output
The sketch prints a pass/fail result for each test and a final `=== Test Complete ===` banner.

## Notes
- This is a unit test for estimator logic only; it does not exercise the full Best-First Search or knapsack load scheduling pipeline.
- Sensor hardware is mocked in the sketch using a local `Adafruit_INA219 ina219` instance, while battery and solar values are passed into `PowerEstimator::update(...)`.
