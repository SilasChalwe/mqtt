#include "BestFirstSearch.h"
#include "../../include/TimeManager.h"

#include <Arduino.h>      // required for millis
#include <algorithm>      // required for std::max (retained for compatibility)
#include <cmath>          // required for ceil/floor in resource scaling (no longer used)

namespace {
bool isScheduleActive(const Node* node, int currentMinute) {
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

float traversalCost(const Node* node) {
    if (!node) return 0.0f;
    float base = node->wireFriction + node->currentDraw * 0.01f;
    float priorityPenalty = (100 - node->priority) * 0.01f;
    return base + priorityPenalty;
}

float heuristicCost(const Node* node, int currentMinute) {
    if (!node) return 0.0f;
    float score = node->wireFriction + node->currentDraw * 0.02f;
    if (isScheduleActive(node, currentMinute)) {
        score -= node->scheduleBoost * 0.005f;
    }
    score -= node->priority * 0.01f;
    return score > 0.0f ? score : 0.0f;
}
}

BestFirstSearch::BestFirstSearch() {
    // Root is the entry point (the very top circle in your image)
    rootName = "Main_DB";
    root = new Node();
    root->id = 0;
    root->name = rootName;
    root->currentDraw = 0.0f;
    root->voltage = 0.0f;
    root->power = 0.0f;
    root->energyWh = 0.0f;
    root->lastActiveUpdateMs = millis();
    root->priority = 0;
    root->relayPin = -1;
    root->mode = LoadMode::Fixed;
    root->wireFriction = 0.0f;
    root->hasSchedule = false;
    root->scheduleStartMinute = -1;
    root->scheduleEndMinute = -1;
    root->scheduleBoost = 1000;
    root->scheduleMode = 1;
    root->scheduleMinimumSoc = 20.0f;
    root->scheduleRequiredRuntimeMinutes = 0;
    root->scheduleRuntimeTodayMinutes = 0;
    root->scheduleLastRuntimeMinute = -1;
    root->isActive = true;
}

void BestFirstSearch::clear() {
    if (!root) return;
    for (Node* child : root->children) {
        recursiveDelete(child);
    }
    root->children.clear();
    root->name = rootName;
    root->currentDraw = 0.0f;
    root->voltage = 0.0f;
    root->power = 0.0f;
    root->energyWh = 0.0f;
    root->lastActiveUpdateMs = millis();
    root->priority = 0;
    root->relayPin = -1;
    root->mode = LoadMode::Fixed;
    root->wireFriction = 0.0f;
    root->hasSchedule = false;
    root->scheduleStartMinute = -1;
    root->scheduleEndMinute = -1;
    root->scheduleBoost = 1000;
    root->scheduleMode = 1;
    root->scheduleMinimumSoc = 20.0f;
    root->scheduleRequiredRuntimeMinutes = 0;
    root->scheduleRuntimeTodayMinutes = 0;
    root->scheduleLastRuntimeMinute = -1;
    root->isActive = true;
}

Node* BestFirstSearch::findByName(Node* current, const String& name) {
    if (!current) return nullptr;
    if (current->name == name) return current;
    
    for (Node* child : current->children) {
        Node* result = findByName(child, name);
        if (result) return result;
    }
    return nullptr;
}

Node* BestFirstSearch::createAndAddNode(const String& parentName, const String& name, float amps, int priority, int pin, float friction, bool fixed, float voltage) {
    // If parent is not specified or the configured root name, start at root
    Node* parent = (parentName == rootName || parentName == "") ? root : findByName(root, parentName);

    if (!parent) return nullptr; 

    Node* newNode = new Node();
    newNode->name = name;
    newNode->currentDraw = amps;
    newNode->voltage = voltage;
    newNode->recalculatePower();
    newNode->energyWh = 0.0f;
    newNode->lastActiveUpdateMs = millis();
    newNode->priority = priority;
    newNode->relayPin = pin;
    newNode->wireFriction = friction;
    newNode->hasSchedule = false;
    newNode->scheduleStartMinute = -1;
    newNode->scheduleEndMinute = -1;
    newNode->scheduleBoost = 1000;
    newNode->scheduleMode = 1;
    newNode->scheduleMinimumSoc = 20.0f;
    newNode->scheduleRequiredRuntimeMinutes = 0;
    newNode->scheduleRuntimeTodayMinutes = 0;
    newNode->scheduleLastRuntimeMinute = -1;
    newNode->mode = fixed ? LoadMode::Fixed : LoadMode::Auto;
    newNode->isActive = false;

    parent->children.push_back(newNode);
    return newNode;
}

bool BestFirstSearch::updateNode(const String& name, float newAmps, int newPriority, bool fixed, float newVoltage) {
    Node* target = findByName(root, name);
    if (target) {
        target->currentDraw = newAmps;
        if (newVoltage > 0.0f) {
            target->voltage = newVoltage;
        }
        target->recalculatePower();
        target->priority = newPriority;
        target->mode = fixed ? LoadMode::Fixed : LoadMode::Auto;
        return true;
    }
    return false;
}

void BestFirstSearch::recursiveDelete(Node* node) {
    if (!node) return;
    for (Node* child : node->children) {
        recursiveDelete(child);
    }
    delete node; 
}

bool BestFirstSearch::removeNode(const String& nodeName) {
    if (nodeName == rootName) return false; 

    // Internal helper to find the parent of a target node
    std::function<Node*(Node*, const String&)> findParentOf = [&](Node* current, const String& target) -> Node* {
        for (Node* child : current->children) {
            if (child->name == target) return current;
            Node* p = findParentOf(child, target);
            if (p) return p;
        }
        return nullptr;
    };

    Node* parent = findParentOf(root, nodeName);
    if (!parent) return false;

    for (auto it = parent->children.begin(); it != parent->children.end(); ++it) {
        if ((*it)->name == nodeName) {
            recursiveDelete(*it);
            parent->children.erase(it);
            return true;
        }
    }
    return false;
}

/**
 * A* search scout: collects all appliance nodes (leaf nodes with a relay)
 * in descending order of evaluation value f(n) = g(n) + h(n).
 * g(n) is the node current draw and h(n) is the existing heuristic
 * priority - wireFriction.
 */
std::vector<Node*> BestFirstSearch::scout(float availableCurrent) {
    std::vector<Node*> candidates;
    (void)availableCurrent;
    const int currentMinute = TimeManager::getMinutesSinceMidnight();

    struct AStarNode {
        Node* node;
        float gCost;
        float fCost;
    };

    auto compare = [](const AStarNode& a, const AStarNode& b) {
        return a.fCost > b.fCost;
    };
    std::priority_queue<AStarNode, std::vector<AStarNode>, decltype(compare)> openSet(compare);

    openSet.push(AStarNode{root, 0.0f, heuristicCost(root, currentMinute)});
    while (!openSet.empty()) {
        AStarNode current = openSet.top();
        openSet.pop();

        if (current.node->relayPin != -1) {
            candidates.push_back(current.node);
        }

        for (Node* child : current.node->children) {
            if (!child) continue;
            float g = current.gCost + traversalCost(child);
            float h = heuristicCost(child, currentMinute);
            float f = g + h;
            openSet.push(AStarNode{child, g, f});
        }
    }

    return candidates;
}

void BestFirstSearch::execute(std::vector<Node*> candidates, float C_available, float P_available) {
    int N = candidates.size();
    if (N == 0) return;

    // Ensure all power values are up to date
    for (Node* n : candidates) {
        n->recalculatePower();
    }

    // --- Fixed loads ---
    float fixedCurrent = 0.0f;
    float fixedPower = 0.0f;
    std::vector<bool> selected(N, false);

    for (int i = 0; i < N; i++) {
        if (candidates[i]->isFixed()) {
            selected[i] = candidates[i]->isActive;   // keep existing state
            if (selected[i]) {
                fixedCurrent += candidates[i]->currentDraw;
                fixedPower   += candidates[i]->power;
            }
        }
    }

    // --- Remaining budget after fixed loads ---
    float remainingCurrent = C_available - fixedCurrent;
    float remainingPower   = P_available   - fixedPower;
    if (remainingCurrent < 0.0f) remainingCurrent = 0.0f;
    if (remainingPower   < 0.0f) remainingPower   = 0.0f;

    /**
     * A* ranked knapsack selection:
     * The candidates are already ordered by descending f(n) = g(n) + h(n)
     * because scout() performed an A* traversal. We now activate each automatic
     * load in that order if it fits within the remaining budget.
     */
    // // TOLERANCE avoids floating-point comparison issues; renamed from EPS to avoid ESP32 macro conflict
    // const float TOLERANCE = 1e-6f;
    // for (int i = 0; i < N; i++) {
    //     Node* node = candidates[i];
    //     if (node->isFixed()) continue;   // already handled

    //     if ((node->currentDraw <= remainingCurrent + TOLERANCE) &&
    //         (node->power       <= remainingPower   + TOLERANCE)) {
    //         selected[i] = true;
    //         remainingCurrent -= node->currentDraw;
    //         remainingPower   -= node->power;
    //     } else {
    //         selected[i] = false;
    //     }
    // }
 // ---------- 0/1 Knapsack Selection ----------
        const int SCALE = 100;                         // 0.01 A resolution
        int capacity = (int)(remainingCurrent * SCALE + 0.5f);

        std::vector<float> dp(capacity + 1, -1.0f);
        std::vector<std::vector<bool>> keep(N, std::vector<bool>(capacity + 1, false));

        dp[0] = 0.0f;

        for (int i = 0; i < N; i++) {

            Node* node = candidates[i];

            if (node->isFixed())
                continue;

            int weight = (int)(node->currentDraw * SCALE + 0.5f);
            float value = (float)node->priority;

            for (int w = capacity; w >= weight; w--) {

                if (dp[w - weight] < 0.0f)
                    continue;

                if (node->power > remainingPower)
                    continue;

                if (dp[w] < dp[w - weight] + value) {

                    dp[w] = dp[w - weight] + value;
                    keep[i][w] = true;
                }
            }
        }

        int w = capacity;

        for (int i = N - 1; i >= 0; i--) {

            if (candidates[i]->isFixed())
                continue;

            int weight = (int)(candidates[i]->currentDraw * SCALE + 0.5f);

            if (w >= weight && keep[i][w]) {

                selected[i] = true;
                remainingCurrent -= candidates[i]->currentDraw;
                remainingPower   -= candidates[i]->power;
                w -= weight;
            }
            else {

                selected[i] = false;
            }
        }
   
   
   
   
    // --- Energy accounting and state application ---
    unsigned long nowMs = millis();
    const int currentMinute = TimeManager::getMinutesSinceMidnight();
    for (int i = 0; i < N; i++) {
        Node* n = candidates[i];
        if (n->relayPin == -1) continue;
        n->accumulateEnergy(nowMs);
        if (n->hasSchedule && currentMinute >= 0) {
            if (n->scheduleLastRuntimeMinute > currentMinute) {
                n->scheduleRuntimeTodayMinutes = 0;
            }
            if (selected[i] && isScheduleActive(n, currentMinute) && n->scheduleLastRuntimeMinute >= 0) {
                int deltaMinutes = currentMinute - n->scheduleLastRuntimeMinute;
                if (deltaMinutes > 0) {
                    n->scheduleRuntimeTodayMinutes += deltaMinutes;
                }
            }
            n->scheduleLastRuntimeMinute = currentMinute;
        }
        n->isActive = selected[i];
    }
}