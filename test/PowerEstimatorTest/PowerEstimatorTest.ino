/*
  PowerEstimatorTest.ino
  Unit test for SolarManager and PowerEstimator behavior.

  This sketch validates:
  - SolarManager filtering and power calculations
  - PowerEstimator available current budget
  - PowerEstimator runtime estimate under load
  - Safety threshold and inverter limit behavior
*/

#include "../../config/Config.h"
#include "../../include/SolarManager.h"
#include "../../include/PowerEstimator.h"
#include "../../include/Battery.h"

// Include implementation files so the sketch links standalone.
#include "../../src/SolarManager.cpp"
#include "../../src/PowerEstimator.cpp"

// Mock sensor globals expected by Battery.h and SolarManager.
Adafruit_INA219 ina219;
float battery_soc = 0.0;
float battery_voltage = 0.0;

void printResult(bool ok, const char* testName) {
  Serial.printf("%s: %s\n", testName, ok ? "PASSED" : "FAILED");
}

bool approxEqual(float a, float b, float tol = 0.01f) {
  return fabs(a - b) <= tol;
}

void setup() {
  Serial.begin(115200);
  delay(1000);
  Serial.println("=== PowerEstimator Unit Test ===");

  SolarManager solar(35);
  PowerEstimator estimator(solar, 0.2);

  // Test 1: SolarManager direct state and power math.
  solar.setCurrent(2.0f);
  solar.setVoltage(12.0f);
  bool ok1 = approxEqual(solar.getCurrent(), 2.0f) && approxEqual(solar.getVoltage(), 12.0f) && approxEqual(solar.getPower(), 24.0f);
  printResult(ok1, "SolarManager power calculation");

  // Provide battery state for estimator.
  battery_voltage = 12.0f;
  battery_soc = 85.0f;
  estimator.setFilteredSoC(0.85f);

  // Test 2: Available current with battery + solar.
  float available = estimator.get_CAvailable(100.0f, 5.0f, 1.0f);
  bool ok2 = available > 0.0f;
  printResult(ok2, "PowerEstimator available current > 0");

  // Expected approximate budget: usableAh = 100 * (0.85 - 0.2) = 65
  // budget = min(INVERTER_MAX_AMPS, (65 + 2.0) / 5) = min(15, 13.4) = 13.4
  // result = 13.4 - 1.0 = 12.4
  bool ok3 = approxEqual(available, 12.4f, 0.2f);
  printResult(ok3, "PowerEstimator budget calc");

  // Test 3: Estimated runtime with load less than solar.
  float runtime1 = estimator.get_EstimatedRuntime(100.0f, 1.0f);
  bool ok4 = approxEqual(runtime1, 65.0f / (1.0f - 2.0f), 0.2f);
  // Because net load is negative, the method returns 99.9 as unlimited runtime.
  ok4 = approxEqual(runtime1, 99.9f, 0.1f);
  printResult(ok4, "PowerEstimator unlimited runtime when solar covers load");

  // Test 4: Estimated runtime with net discharge.
  float runtime2 = estimator.get_EstimatedRuntime(100.0f, 5.0f);
  float expectedRuntime2 = 65.0f / 3.0f; // net = 5 - 2
  bool ok5 = approxEqual(runtime2, expectedRuntime2, 0.2f);
  printResult(ok5, "PowerEstimator runtime under normal load");

  // Test 5: Safety threshold prevents negative usable Ah.
  estimator.setFilteredSoC(0.1f);
  float available2 = estimator.get_CAvailable(100.0f, 5.0f, 0.0f);
  bool ok6 = approxEqual(available2, 0.0f, 0.01f);
  printResult(ok6, "PowerEstimator zero available when SoC below threshold");

  Serial.println("=== Test Complete ===");
}

void loop() {
  // Nothing to do.
  vTaskDelay(pdMS_TO_TICKS(1000));
}
