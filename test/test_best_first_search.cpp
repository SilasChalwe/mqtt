#include <cassert>
#include <cmath>
#include <iostream>
#include <vector>

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

    std::vector<Node*> candidates{tooCurrentHeavy, optimalA, optimalB};
    bfs.execute(candidates, 6.0f, 110.0f);

    assertWithinBudget(candidates, 6.0f, 110.0f);
    assert(!tooCurrentHeavy->isActive);
    assert(optimalA->isActive);
    assert(optimalB->isActive);
}

int main() {
    cumulative_current_limit_is_enforced();
    fixed_loads_reduce_remaining_budget();
    two_constraint_selection_beats_power_only_greedy_trim();
    std::cout << "BestFirstSearch current-limit tests passed\n";
    return 0;
}
