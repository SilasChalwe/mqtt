#include "../include/SolarManager.h"

SolarManager::SolarManager(int adcPin, int precision) 
    : _adcPin(adcPin), _precision(precision), _filteredVoltage(0), 
      _filteredCurrentAmps(0), _totalEnergyWh(0), _lastUpdateTimeMs(0) {}

bool SolarManager::begin() {
    // Initialize sensor state to zero until valid readings arrive
    _filteredVoltage = 0.0f;
    _filteredCurrentAmps = 0.0f;
    _totalEnergyWh = 0.0f;
    _lastUpdateTimeMs = millis();
    return true; 
}

void SolarManager::update() {
    // 1. Read Current from the SHARED sensor
    float rawAmps = ina219.getCurrent_mA() / 1000.0f;
    if (rawAmps < 0) rawAmps = 0; // Solar contribution only

    _filteredCurrentAmps = (rawAmps * FILTER_ALPHA) + (_filteredCurrentAmps * (1.0f - FILTER_ALPHA));

    // 2. Read Voltage from ADC pin (Pin 35)
    int rawAdc = analogRead(_adcPin);
    // Adjust 11.0 to match your resistor divider ratio
    float rawVolts = (rawAdc / 4095.0f) * 3.3f * 11.0f; 
    _filteredVoltage = (rawVolts * FILTER_ALPHA) + (_filteredVoltage * (1.0f - FILTER_ALPHA));

    // 3. Energy Accumulation
    unsigned long now = millis();
    float hoursPassed = (now - _lastUpdateTimeMs) / 3600000.0f;
    _totalEnergyWh += getPower() * hoursPassed;
    _lastUpdateTimeMs = now;
}
