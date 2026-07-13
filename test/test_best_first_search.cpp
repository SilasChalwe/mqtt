#include <cassert>
#include <cmath>
#include <iostream>
#include <vector>

#include "../include/TimeManager.h"
#include "../include/ScheduleFeasibility.h"
#include "../include/SchedulePlanner.h"

static int fakeMinutesSinceMidnight = -1;

bool TimeManager::isTimeValid() { return fakeMinutesSinceMidnight >= 0; }
String TimeManager::getFormattedTime(const char*) { return "00:00:00"; }
int TimeManager::getMinutesSinceMidnight() { return fakeMinutesSinceMidnight; }
void TimeManager::begin(const char*, long, int) {}

#include "../lib/src/BestFirstSearch.cpp"

static float selectedCurrent(const std::vector<Node*>& nodes) {
    float total = 0.0f;
    for (Node* node : nodes) {
        if (node->isActive) total += node->currentDraw;
    }
    return total;
}

static float selectedPower(const std::vector<Node*>& nodes) {
    float total = 0.0f;
    for (Node* node : nodes) {
        if (node->isActive) total += node->power;
    }
    return total;
}

static void assertWithinBudget(const std::vector<Node*>& nodes, float current, float power) {
    assert(selectedCurrent(nodes) <= current + 0.0001f);
    assert(selectedPower(nodes) <= power + 0.0001f);
}

static void cumulative_current_limit_is_enforced() {
    BestFirstSearch bfs;
    Node* highA = bfs.createAndAddNode("Main_DB", "highA", 4.0f, 10, 1, 0.0f, false, 10.0f);
    Node* highB = bfs.createAndAddNode("Main_DB", "highB", 4.0f, 9, 2, 0.0f, false, 10.0f);
    Node* medium = bfs.createAndAddNode("Main_DB", "medium", 2.0f, 6, 3, 0.0f, false, 10.0f);

    std::vector<Node*> candidates{highA, highB, medium};
    bfs.execute(candidates, 6.0f, 100.0f);

    assertWithinBudget(candidates, 6.0f, 100.0f);
    assert(highA->isActive);
    assert(!highB->isActive);
    assert(medium->isActive);
}

static void fixed_loads_reduce_remaining_budget() {
    BestFirstSearch bfs;
    Node* fixed = bfs.createAndAddNode("Main_DB", "fixed", 4.0f, 1, 1, 0.0f, true, 10.0f);
    Node* fitsRemaining = bfs.createAndAddNode("Main_DB", "fitsRemaining", 2.0f, 8, 2, 0.0f, false, 10.0f);
    Node* exceedsRemaining = bfs.createAndAddNode("Main_DB", "exceedsRemaining", 3.0f, 20, 3, 0.0f, false, 10.0f);
    fixed->isActive = true;

    std::vector<Node*> candidates{fixed, fitsRemaining, exceedsRemaining};
    bfs.execute(candidates, 6.0f, 60.0f);

    assert(fixed->isActive);
    assert(fitsRemaining->isActive);
    assert(!exceedsRemaining->isActive);
    assertWithinBudget(candidates, 6.0f, 60.0f);
}

static void two_constraint_selection_beats_power_only_greedy_trim() {
    BestFirstSearch bfs;
    Node* tooCurrentHeavy = bfs.createAndAddNode("Main_DB", "tooCurrentHeavy", 5.0f, 10, 1, 0.0f, false, 10.0f);
    Node* optimalA = bfs.createAndAddNode("Main_DB", "optimalA", 3.0f, 9, 2, 0.0f, false, 10.0f);
    Node* optimalB = bfs.createAndAddNode("Main_DB", "optimalB", 3.0f, 9, 3, 0.0f, false, 10.0f);

    std::vector<Node*> candidates{optimalA, optimalB, tooCurrentHeavy};
    bfs.execute(candidates, 6.0f, 110.0f);

    assertWithinBudget(candidates, 6.0f, 110.0f);
    assert(!tooCurrentHeavy->isActive);
    assert(optimalA->isActive);
    assert(optimalB->isActive);
}

static void scout_orders_candidates_by_a_star_evaluation() {
    BestFirstSearch bfs;
    Node* lowerEvaluation = bfs.createAndAddNode("Main_DB", "lowerEvaluation", 1.0f, 10, 1, 0.0f, false, 10.0f);
    Node* higherEvaluation = bfs.createAndAddNode("Main_DB", "higherEvaluation", 5.0f, 8, 2, 0.0f, false, 10.0f);

    std::vector<Node*> candidates = bfs.scout(10.0f);

    assert(candidates.size() == 2);
    assert(candidates[0] == higherEvaluation);
    assert(candidates[1] == lowerEvaluation);
}

static void active_schedule_boosts_candidate_during_user_window() {
    BestFirstSearch bfs;
    Node* normal = bfs.createAndAddNode("Main_DB", "normal", 1.0f, 10, 1, 0.0f, false, 10.0f);
    Node* scheduled = bfs.createAndAddNode("Main_DB", "scheduled", 1.0f, 1, 2, 0.0f, false, 10.0f);
    scheduled->hasSchedule = true;
    scheduled->scheduleStartMinute = 19 * 60;
    scheduled->scheduleEndMinute = 20 * 60;
    scheduled->scheduleBoost = 1000;

    fakeMinutesSinceMidnight = (19 * 60) + 5;
    std::vector<Node*> candidates = bfs.scout(10.0f);

    assert(candidates.size() == 2);
    assert(candidates[0] == scheduled);
    assert(candidates[1] == normal);
    fakeMinutesSinceMidnight = -1;
}

static void schedule_feasibility_reports_energy_and_failure_reason() {
    BestFirstSearch bfs;
    Node* scheduled = bfs.createAndAddNode("Main_DB", "scheduled", 2.0f, 1, 2, 0.0f, false, 12.0f);
    scheduled->hasSchedule = true;
    scheduled->scheduleStartMinute = 19 * 60;
    scheduled->scheduleEndMinute = 21 * 60;
    scheduled->scheduleMode = 2;
    scheduled->scheduleMinimumSoc = 40.0f;
    scheduled->recalculatePower();

    ScheduleFeasibilityResult result = ScheduleFeasibility::evaluate(scheduled,
                                                                     18 * 60,
                                                                     30.0f,
                                                                     12.0f,
                                                                     10.0f,
                                                                     0.0f,
                                                                     0.0f,
                                                                     20.0f);

    assert(result.hasSchedule);
    assert(!result.canRun);
    assert(result.scheduleStatus == "skipped_low_battery");
    assert(result.requiredEnergyWh == 48.0f);
    assert(result.startsInMinutes == 60);
}

static void schedule_planner_defers_at_risk_optional_loads() {
    BestFirstSearch bfs;
    Node* optional = bfs.createAndAddNode("Main_DB", "optional", 10.0f, 1, 2, 0.0f, false, 12.0f);
    optional->hasSchedule = true;
    optional->scheduleStartMinute = 19 * 60;
    optional->scheduleEndMinute = 21 * 60;
    optional->scheduleMode = 0;
    optional->recalculatePower();

    std::vector<Node*> planned = SchedulePlanner::planCandidates({optional},
                                                                 18 * 60,
                                                                 50.0f,
                                                                 12.0f,
                                                                 1.0f,
                                                                 0.0f,
                                                                 0.0f,
                                                                 20.0f);

    assert(planned.empty());
    assert(!optional->isActive);
}

int main() {
    cumulative_current_limit_is_enforced();
    fixed_loads_reduce_remaining_budget();
    two_constraint_selection_beats_power_only_greedy_trim();
    scout_orders_candidates_by_a_star_evaluation();
    active_schedule_boosts_candidate_during_user_window();
    schedule_feasibility_reports_energy_and_failure_reason();
    schedule_planner_defers_at_risk_optional_loads();
    std::cout << "BestFirstSearch current-limit tests passed\n";
    return 0;
}
