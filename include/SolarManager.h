#ifndef SOLAR_MANAGER_H
#define SOLAR_MANAGER_H

#include <Arduino.h>
#include <Adafruit_INA219.h>

// Shared global instance from Battery.h 
extern Adafruit_INA219 ina219;

class SolarManager {
private:
    // Core parameters
    int _adcPin;
    int _precision;

    // Signal filtering state variables
    float _filteredVoltage;
    float _filteredCurrentAmps;

    // Energy tracking
    float _totalEnergyWh;
    unsigned long _lastUpdateTimeMs;

    static constexpr float FILTER_ALPHA = 0.15;

public:
    SolarManager(int adcPin, int precision = 3);

    bool begin(); 
    void update(); // Uses the shared global 'ina219'
    void resetEnergy();

    float getVoltage() const { return _filteredVoltage; }
    float getCurrent() const { return _filteredCurrentAmps; }
    float getPower() const { return (_filteredCurrentAmps <= 0.0f) ? 0.0f : (_filteredVoltage * _filteredCurrentAmps); }
    float getEnergyWh() const { return _totalEnergyWh; }

    // Test helpers
    void setCurrent(float amps) { _filteredCurrentAmps = amps; }
    void setVoltage(float volts) { _filteredVoltage = volts; }
};

#endif