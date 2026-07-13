#include "../include/SchedulePlanner.h"

namespace SchedulePlanner {

const char* actionFor(const ScheduleFeasibilityResult& result, int scheduleMode) {
    if (!result.hasSchedule) return "normal";
    if (result.scheduleStatus == "completed") return "completed";
    if (result.scheduleStatus == "skipped_low_battery") return "force_off_low_battery";
    if (result.scheduleStatus == "will_fail") return "force_off_insufficient_energy";
    if (result.scheduleStatus == "at_risk" && scheduleMode != 2) return "defer_at_risk";
    if (result.scheduleStatus == "active" && scheduleMode == 2) return "force_on_required";
    if (result.scheduleStatus == "reserved") return "reserve_energy";
    return "normal";
}

std::vector<Node*> planCandidates(const std::vector<Node*>& candidates,
                                  int currentMinute,
                                  float batterySocPercent,
                                  float batteryVoltage,
                                  float batteryCapacityAh,
                                  float solarCurrent,
                                  float solarPower,
                                  float defaultMinimumSocPercent) {
    std::vector<Node*> forcedOn;
    std::vector<Node*> normal;

    for (Node* node : candidates) {
        ScheduleFeasibilityResult result = ScheduleFeasibility::evaluate(node,
                                                                         currentMinute,
                                                                         batterySocPercent,
                                                                         batteryVoltage,
                                                                         batteryCapacityAh,
                                                                         solarCurrent,
                                                                         solarPower,
                                                                         defaultMinimumSocPercent);
        const char* action = actionFor(result, node ? node->scheduleMode : 1);
        String actionText = action;
        if (actionText == "force_off_low_battery" || actionText == "force_off_insufficient_energy" ||
            actionText == "defer_at_risk" || actionText == "completed") {
            if (node) node->isActive = false;
            continue;
        }
        if (actionText == "force_on_required") {
            forcedOn.push_back(node);
        } else {
            normal.push_back(node);
        }
    }

    forcedOn.insert(forcedOn.end(), normal.begin(), normal.end());
    return forcedOn;
}

}
