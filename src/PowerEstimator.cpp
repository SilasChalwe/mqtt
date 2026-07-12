#include "../include/PowerEstimator.h"
#include "../config/Config.h"

PowerEstimator::PowerEstimator(float threshold)
    : safetyThreshold(threshold), _filteredSoC(0.0f), _batteryVoltage(0.0f), _solarCurrent(0.0f), _solarPower(0.0f) {}

void PowerEstimator::begin() {
    _filteredSoC = 0.0f;
    _batteryVoltage = 0.0f;
    _solarCurrent = 0.0f;
    _solarPower = 0.0f;
}

void PowerEstimator::update(float batterySocPercent, float batteryVoltage, float solarCurrent, float solarPower) {
    float currentSoC = batterySocPercent / 100.0f;
    _filteredSoC = (currentSoC * _filterAlpha) + (_filteredSoC * (1.0f - _filterAlpha));
    _batteryVoltage = batteryVoltage;
    _solarCurrent = solarCurrent;
    _solarPower = solarPower;
}

float PowerEstimator::getAvailableCurrent(float batteryAhTotal,
                                          float targetRuntimeHours,
                                          float forcedLoadsCurrent) const {
    if (targetRuntimeHours <= 0) targetRuntimeHours = 1.0f;

    float usableSoC = _filteredSoC - safetyThreshold;
    if (usableSoC < 0.0f) usableSoC = 0.0f;
    float batteryAhBuffer = batteryAhTotal * usableSoC;

    float capacityRate = (batteryAhBuffer + _solarCurrent) / targetRuntimeHours;
    float budget = (capacityRate < INVERTER_MAX_AMPS) ? capacityRate : INVERTER_MAX_AMPS;
    float result = budget - forcedLoadsCurrent;

    return (result > 0.0f) ? result : 0.0f;
}

float PowerEstimator::getEstimatedRuntimeHours(float batteryAh,
                                               float totalLoadCurrent) const {
    float net = totalLoadCurrent - _solarCurrent;
    if (net <= 0.0f) return 99.9f;

    float usableAh = batteryAh * (_filteredSoC - safetyThreshold);
    if (usableAh <= 0.0f) return 0.0f;

    return usableAh / net;
}
