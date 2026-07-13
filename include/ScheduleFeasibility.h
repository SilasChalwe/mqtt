#ifndef SCHEDULE_FEASIBILITY_H
#define SCHEDULE_FEASIBILITY_H

#include <Arduino.h>
#include "Node.h"

struct ScheduleFeasibilityResult {
    bool hasSchedule = false;
    bool canRun = false;
    String scheduleStatus = "unscheduled";
    String failureReason = "";
    int startsInMinutes = -1;
    float scheduleDurationHours = 0.0f;
    float requiredCurrent = 0.0f;
    float requiredPowerW = 0.0f;
    float requiredEnergyWh = 0.0f;
    float availableBatteryWh = 0.0f;
    float predictedSolarWh = 0.0f;
    float reservedEnergyWh = 0.0f;
    float minimumSoc = 20.0f;
    int requiredRuntimeMinutes = 0;
    int remainingRuntimeMinutes = 0;
};

namespace ScheduleFeasibility {
    int scheduleDurationMinutes(const Node* node);
    int minutesUntilStart(const Node* node, int currentMinute);
    bool isActive(const Node* node, int currentMinute);
    const char* modeString(int scheduleMode);

    ScheduleFeasibilityResult evaluate(const Node* node,
                                       int currentMinute,
                                       float batterySocPercent,
                                       float batteryVoltage,
                                       float batteryCapacityAh,
                                       float solarCurrent,
                                       float solarPower,
                                       float defaultMinimumSocPercent);
}

#endif
