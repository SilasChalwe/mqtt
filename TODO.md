# TODO

## Production readiness checklist

### 1. Hardware and wiring
- [ ] Verify hardware wiring and label connections for INA219, ESP32, relays, and power supply.
- [ ] Confirm the actual hardware is connected and powered during validation runs.
- [ ] Record the exact pin mapping in `README.md` or a dedicated wiring diagram section.

### 2. Build and baseline validation
- [ ] Confirm clean build on `esp32:esp32:esp32` using `arduino-cli compile --fqbn esp32:esp32:esp32 mqtt.ino`.
- [ ] Add a build badge to `README.md` if the repository is intended for production deployment.
- [ ] Document the required Arduino CLI version and ESP32 package version.

### 3. Algorithm and scheduler correctness
- [ ] Add unit tests for `ScheduleFeasibility::evaluate()` scenarios, including low battery, solar availability, and reserve conditions.
- [ ] Add tests for `SchedulePlanner::planCandidates()` to ensure forced-on, deferred, and normal actions behave correctly.
- [ ] Review the greedy selection algorithm in `BestFirstSearch::execute()` and document its limitations.
- [ ] Add a production-level note for future algorithm improvements: multi-criteria scoring, horizon planning, and Knapsack optimization.

### 4. Error handling and reliability
- [ ] Add sensor initialization checks and safe fallback behavior when INA219 or battery sensors fail.
- [ ] Add Wi-Fi connection retry and MQTT reconnect handling.
- [ ] Add watchdog-style recovery or safe shutdown behavior for unsupported or invalid states.

### 5. Runtime behavior and telemetry
- [ ] Validate JSON payload size and ensure ArduinoJson capacity is sufficient for `ControlResult` output.
- [ ] Confirm telemetry topics, payload format, and scheduler output are stable and documented.
- [ ] Add a runtime health/status message or topic for observing scheduler state on connected hardware.

### 6. Documentation and release readiness
- [ ] Document how schedule modes are interpreted and what `SchedulePlanner::actionFor()` does.
- [ ] Add versioning, release notes, and a changelog entry for the current production-ready state.
- [ ] Add steps for deployment and hardware setup in `README.md`.
- [ ] Run full end-to-end tests on actual hardware with real power management scenarios and log the results.
