#include "../include/SolarManager.h"

SolarManager::SolarManager(Adafruit_INA219& sensor, int adcPin, float voltageDividerRatio, int precision)
    : _sensor(sensor), _adcPin(adcPin), _precision(precision), _voltageDividerRatio(voltageDividerRatio),
      _filteredVoltage(0), _filteredCurrentAmps(0), _totalEnergyWh(0), _lastUpdateTimeMs(0) {}

bool SolarManager::begin() {
    _filteredVoltage = 0.0f;
    _filteredCurrentAmps = 0.0f;
    _totalEnergyWh = 0.0f;
    _lastUpdateTimeMs = millis();
    return true;
}

void SolarManager::update() {
    float rawAmps = _sensor.getCurrent_mA() / 1000.0f;
    if (rawAmps < 0) rawAmps = 0;

    _filteredCurrentAmps = (rawAmps * FILTER_ALPHA) + (_filteredCurrentAmps * (1.0f - FILTER_ALPHA));

    int rawAdc = analogRead(_adcPin);
    float rawVolts = (rawAdc / 4095.0f) * 3.3f * _voltageDividerRatio;
    _filteredVoltage = (rawVolts * FILTER_ALPHA) + (_filteredVoltage * (1.0f - FILTER_ALPHA));

    unsigned long now = millis();
    unsigned long deltaMs = (now >= _lastUpdateTimeMs) ? (now - _lastUpdateTimeMs) : 0;
    _totalEnergyWh += getPower() * (deltaMs / 3600000.0f);
    _lastUpdateTimeMs = now;
}

void SolarManager::resetEnergy() {
    _totalEnergyWh = 0.0f;
    _lastUpdateTimeMs = millis();
}
