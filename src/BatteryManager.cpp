#include "../include/BatteryManager.h"

BatteryManager::BatteryManager(Adafruit_INA219& sensor, float voltageMin, float voltageMax, float capacityAh)
    : _sensor(sensor), _voltageMin(voltageMin), _voltageMax(voltageMax), _capacityAh(capacityAh) {}

bool BatteryManager::begin() {
    if (!_sensor.begin()) {
        Serial.println("INA219 failed!");
        return false;
    }
    update();
    return true;
}

void BatteryManager::update() {
    _voltage = _sensor.getBusVoltage_V();
    _soc = ((_voltage - _voltageMin) / (_voltageMax - _voltageMin)) * 100.0f;
    _soc = constrain(_soc, 0.0f, 100.0f);
}
