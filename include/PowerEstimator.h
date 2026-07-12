#ifndef POWER_ESTIMATOR_H
#define POWER_ESTIMATOR_H

#include <Arduino.h>
#include "Battery.h"
#include "SolarManager.h"

extern float INVERTER_MAX_AMPS;

class PowerEstimator {
private:
    SolarManager& solar;
    float safetyThreshold;
    float _filteredSoC;
    const float _filterAlpha = 0.15;

public:
    // This now correctly expects 2 arguments: (SolarManager, threshold)
    PowerEstimator(SolarManager& solarInstance, float threshold = 0.2);

    void begin();
    void updateSensors(); // Named to match loop()

    float get_CAvailable(float batteryAhTotal, float targetRuntimeHours, float forcedLoadsCurrent);
    float get_EstimatedRuntime(float batteryAh, float totalLoadCurrent);

    float get_SolarCurrent() { return solar.getCurrent(); }
    float get_BatterySoC() { return _filteredSoC; }
    float get_BatteryVoltage() { return battery_voltage; }

    // Test helpers
    void setFilteredSoC(float soc) { _filteredSoC = soc; }
};

#endif