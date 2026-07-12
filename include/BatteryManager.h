#ifndef BATTERY_MANAGER_H
#define BATTERY_MANAGER_H

#include <Arduino.h>
#include <Adafruit_INA219.h>

class BatteryManager {
public:
    BatteryManager(Adafruit_INA219& sensor, float voltageMin, float voltageMax, float capacityAh);

    bool begin();
    void update();

    float voltage() const { return _voltage; }
    float soc() const { return _soc; }
    float capacityAh() const { return _capacityAh; }

private:
    Adafruit_INA219& _sensor;
    float _voltageMin;
    float _voltageMax;
    float _capacityAh;
    float _voltage = 0.0f;
    float _soc = 0.0f;
};

#endif
