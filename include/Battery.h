#ifndef BATTERY_H
#define BATTERY_H

#include <Adafruit_INA219.h>

// Hardware configurations
const float VOLTAGE_MAX = 4.20;            
const float VOLTAGE_MIN = 3.40;            

// Shared global instances and properties
extern Adafruit_INA219 ina219;
extern float battery_soc;
extern float battery_voltage;

// Initializes the sensor
inline void initBattery() {
  if (!ina219.begin()) {
    Serial.println("INA219 failed!");
    while (1);
  }
}

// Reads and updates the properties
inline void updateBattery() {
  battery_voltage = ina219.getBusVoltage_V();
  
  // Calculate and constrain SoC
  battery_soc = ((battery_voltage - VOLTAGE_MIN) / (VOLTAGE_MAX - VOLTAGE_MIN)) * 100.0;
  battery_soc = constrain(battery_soc, 0.0, 100.0);
}

#endif // BATTERY_H
