#ifndef POWER_ESTIMATOR_H
#define POWER_ESTIMATOR_H

#include <Arduino.h>

class PowerEstimator {
private:
    float safetyThreshold;
    float _filteredSoC;
    float _batteryVoltage;
    float _solarCurrent;
    float _solarPower;
    const float _filterAlpha = 0.15f;

public:
    PowerEstimator(float threshold = 0.2f);

    void begin();
    void update(float batterySocPercent, float batteryVoltage, float solarCurrent, float solarPower = 0.0f);

    float getAvailableCurrent(float batteryAhTotal, float targetRuntimeHours, float forcedLoadsCurrent) const;
    float getEstimatedRuntimeHours(float batteryAh, float totalLoadCurrent) const;

    // Backwards-compatible wrappers for existing sketches/tests.
    float get_CAvailable(float batteryAhTotal, float targetRuntimeHours, float forcedLoadsCurrent) const {
        return getAvailableCurrent(batteryAhTotal, targetRuntimeHours, forcedLoadsCurrent);
    }
    float get_EstimatedRuntime(float batteryAh, float totalLoadCurrent) const {
        return getEstimatedRuntimeHours(batteryAh, totalLoadCurrent);
    }

    float get_SolarCurrent() const { return _solarCurrent; }
    float get_SolarPower() const { return _solarPower; }
    float get_BatterySoC() const { return _filteredSoC; }
    float get_BatteryVoltage() const { return _batteryVoltage; }

    void setFilteredSoC(float soc) { _filteredSoC = soc; }
    void setSolarCurrent(float amps) { _solarCurrent = amps; }
};

#endif
