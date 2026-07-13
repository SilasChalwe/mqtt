#ifndef SCHEDULE_PLANNER_H
#define SCHEDULE_PLANNER_H

#include <vector>
#include <Arduino.h>
#include "Node.h"
#include "ScheduleFeasibility.h"

namespace SchedulePlanner {
    std::vector<Node*> planCandidates(const std::vector<Node*>& candidates,
                                      int currentMinute,
                                      float batterySocPercent,
                                      float batteryVoltage,
                                      float batteryCapacityAh,
                                      float solarCurrent,
                                      float solarPower,
                                      float defaultMinimumSocPercent);

    const char* actionFor(const ScheduleFeasibilityResult& result, int scheduleMode);
}

#endif
