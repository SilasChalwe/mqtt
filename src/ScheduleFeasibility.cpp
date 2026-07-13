#include "../include/ScheduleFeasibility.h"

namespace {
constexpr int MINUTES_PER_DAY = 24 * 60;
constexpr int SUNRISE_MINUTE = 6 * 60;
constexpr int SUNSET_MINUTE = 18 * 60;

float clampNonNegative(float value) {
    return value > 0.0f ? value : 0.0f;
}

float daylightWeight(int minute) {
    if (minute < SUNRISE_MINUTE || minute >= SUNSET_MINUTE) return 0.0f;
    int noon = 12 * 60;
    int distanceFromNoon = abs(minute - noon);
    float weight = 1.0f - (static_cast<float>(distanceFromNoon) / (6.0f * 60.0f));
    return weight > 0.0f ? weight : 0.0f;
}

float predictedSolarWhForWindow(int startMinute, int durationMinutes, float solarPower) {
    if (startMinute < 0 || durationMinutes <= 0 || solarPower <= 0.0f) return 0.0f;

    float weightedMinutes = 0.0f;
    for (int i = 0; i < durationMinutes; ++i) {
        weightedMinutes += daylightWeight((startMinute + i) % MINUTES_PER_DAY);
    }
    return solarPower * (weightedMinutes / 60.0f);
}
}

namespace ScheduleFeasibility {

int scheduleDurationMinutes(const Node* node) {
    if (!node || !node->hasSchedule || node->scheduleStartMinute < 0 || node->scheduleEndMinute < 0) return 0;
    if (node->scheduleStartMinute == node->scheduleEndMinute) return 1;
    int duration = node->scheduleEndMinute - node->scheduleStartMinute;
    if (duration < 0) duration += MINUTES_PER_DAY;
    return duration;
}

bool isActive(const Node* node, int currentMinute) {
    if (!node || !node->hasSchedule || currentMinute < 0 ||
        node->scheduleStartMinute < 0 || node->scheduleEndMinute < 0) {
        return false;
    }
    if (node->scheduleStartMinute == node->scheduleEndMinute) {
        return currentMinute == node->scheduleStartMinute;
    }
    if (node->scheduleStartMinute < node->scheduleEndMinute) {
        return currentMinute >= node->scheduleStartMinute && currentMinute < node->scheduleEndMinute;
    }
    return currentMinute >= node->scheduleStartMinute || currentMinute < node->scheduleEndMinute;
}

int minutesUntilStart(const Node* node, int currentMinute) {
    if (!node || !node->hasSchedule || currentMinute < 0 || node->scheduleStartMinute < 0) return -1;
    if (isActive(node, currentMinute)) return 0;
    int delta = node->scheduleStartMinute - currentMinute;
    if (delta < 0) delta += MINUTES_PER_DAY;
    return delta;
}

const char* modeString(int scheduleMode) {
    switch (scheduleMode) {
        case 2: return "required";
        case 1: return "preferred";
        default: return "optional";
    }
}

ScheduleFeasibilityResult evaluate(const Node* node,
                                   int currentMinute,
                                   float batterySocPercent,
                                   float batteryVoltage,
                                   float batteryCapacityAh,
                                   float solarCurrent,
                                   float solarPower,
                                   float defaultMinimumSocPercent) {
    (void)solarCurrent;
    ScheduleFeasibilityResult result;
    if (!node || !node->hasSchedule) return result;

    result.hasSchedule = true;
    result.startsInMinutes = minutesUntilStart(node, currentMinute);
    result.requiredRuntimeMinutes = node->scheduleRequiredRuntimeMinutes > 0
                                    ? node->scheduleRequiredRuntimeMinutes
                                    : scheduleDurationMinutes(node);
    result.remainingRuntimeMinutes = result.requiredRuntimeMinutes - node->scheduleRuntimeTodayMinutes;
    if (result.remainingRuntimeMinutes < 0) result.remainingRuntimeMinutes = 0;
    result.scheduleDurationHours = result.remainingRuntimeMinutes / 60.0f;
    result.requiredCurrent = node->currentDraw;
    result.requiredPowerW = node->power > 0.0f ? node->power : (node->currentDraw * node->voltage);
    result.requiredEnergyWh = result.requiredPowerW * result.scheduleDurationHours;
    result.reservedEnergyWh = result.requiredEnergyWh;
    result.minimumSoc = node->scheduleMinimumSoc > 0.0f ? node->scheduleMinimumSoc : defaultMinimumSocPercent;

    float usableSoc = (batterySocPercent - result.minimumSoc) / 100.0f;
    result.availableBatteryWh = batteryCapacityAh * batteryVoltage * clampNonNegative(usableSoc);
    result.predictedSolarWh = predictedSolarWhForWindow(node->scheduleStartMinute,
                                                        scheduleDurationMinutes(node),
                                                        solarPower);

    float availableEnergyWh = result.availableBatteryWh + result.predictedSolarWh;
    result.canRun = availableEnergyWh >= result.requiredEnergyWh;

    if (result.remainingRuntimeMinutes == 0) {
        result.canRun = true;
        result.scheduleStatus = "completed";
    } else if (batterySocPercent < result.minimumSoc) {
        result.canRun = false;
        result.scheduleStatus = "skipped_low_battery";
        result.failureReason = "Battery SoC is below the schedule minimum reserve";
    } else if (result.canRun && result.startsInMinutes == 0) {
        result.scheduleStatus = "active";
    } else if (result.canRun && result.startsInMinutes > 0) {
        result.scheduleStatus = "reserved";
    } else if (!result.canRun && node->scheduleMode == 2) {
        result.scheduleStatus = "will_fail";
        result.failureReason = "Not enough predicted battery plus solar energy for required schedule";
    } else if (!result.canRun) {
        result.scheduleStatus = "at_risk";
        result.failureReason = "Predicted energy is below the scheduled load requirement";
    } else {
        result.scheduleStatus = "scheduled";
    }

    return result;
}

}
