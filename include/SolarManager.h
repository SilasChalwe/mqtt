#ifndef SOLAR_MANAGER_H
#define SOLAR_MANAGER_H

#include <Arduino.h>
#include <Adafruit_INA219.h>

class SolarManager {
private:
    Adafruit_INA219& _sensor;
    int _adcPin;
    int _precision;
    float _voltageDividerRatio;

    float _filteredVoltage;
    float _filteredCurrentAmps;
    float _totalEnergyWh;
    unsigned long _lastUpdateTimeMs;

    static constexpr float FILTER_ALPHA = 0.15f;

public:
    SolarManager(Adafruit_INA219& sensor, int adcPin, float voltageDividerRatio, int precision = 3);

    bool begin();
    void update();
    void resetEnergy();

    float getVoltage() const { return _filteredVoltage; }
    float getCurrent() const { return _filteredCurrentAmps; }
    float getPower() const { return (_filteredCurrentAmps <= 0.0f) ? 0.0f : (_filteredVoltage * _filteredCurrentAmps); }
    float getEnergyWh() const { return _totalEnergyWh; }

    void setCurrent(float amps) { _filteredCurrentAmps = amps; }
    void setVoltage(float volts) { _filteredVoltage = volts; }
};

#endif
