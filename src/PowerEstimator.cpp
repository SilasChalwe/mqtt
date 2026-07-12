#include "../include/PowerEstimator.h"

float INVERTER_MAX_AMPS = 15.0; 

PowerEstimator::PowerEstimator(SolarManager& solarInstance, float threshold)
    : solar(solarInstance),
      safetyThreshold(threshold),
      _filteredSoC(0.0) {
}

void PowerEstimator::begin() {
    // Default the estimator to zero until the first valid sensor update occurs.
    _filteredSoC = 0.0f;
}

void PowerEstimator::updateSensors() {
    solar.update();
    updateBattery();

    float currentSoC = battery_soc / 100.0;
    _filteredSoC = (currentSoC * _filterAlpha) + (_filteredSoC * (1.0 - _filterAlpha));
}

float PowerEstimator::get_CAvailable(float batteryAhTotal,
                                     float targetRuntimeHours,
                                     float forcedLoadsCurrent) {

    if (targetRuntimeHours <= 0) targetRuntimeHours = 1.0;

    // 1. Calculate usable Battery SoC in Amp-Hours
    float usableSoC = _filteredSoC - safetyThreshold;
    if (usableSoC < 0.0) usableSoC = 0.0;
    float battery_Ah_buffer = batteryAhTotal * usableSoC;

    // 2. YOUR IMAGE FORMULA: (Battery_SoC + Live_Solar) / Target_Runtime
    float capacityRate = (battery_Ah_buffer + solar.getCurrent()) / targetRuntimeHours;

    // 3. Apply min(Inverter_Max_Output, capacityRate)
    float budget = (capacityRate < INVERTER_MAX_AMPS) ? capacityRate : INVERTER_MAX_AMPS;

    // 4. Subtract Forced Loads
    float result = budget - forcedLoadsCurrent;

    return (result > 0.0) ? result : 0.0;
}

float PowerEstimator::get_EstimatedRuntime(float batteryAh,
                                           float totalLoadCurrent) {
    float net = totalLoadCurrent - solar.getCurrent();
    if (net <= 0.0) return 99.9; 

    float usableAh = batteryAh * (_filteredSoC - safetyThreshold);
    if (usableAh <= 0.0) return 0.0;

    return usableAh / net;
}
