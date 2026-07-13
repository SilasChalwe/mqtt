#include "BestFirstSearch.h"

#include <Arduino.h>      // required for millis
#include <algorithm>      // required for std::max (retained for compatibility)
#include <cmath>          // required for ceil/floor in resource scaling (no longer used)

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
 * Pure best-first search (greedy): collects all appliance nodes (leaf nodes with a relay)
 * in descending order of heuristic value (priority - wireFriction).
 * The priority queue expands the most promising node first – no path cost is used.
 * This produces a candidate list ready for a greedy knapsack fill.
 */
std::vector<Node*> BestFirstSearch::scout(float availableCurrent) {
    std::vector<Node*> candidates;
    // Heuristic evaluation: the higher (priority - friction), the better.
    auto compare = [](Node* a, Node* b) {
        return (a->priority - a->wireFriction) < (b->priority - b->wireFriction);
    };
    std::priority_queue<Node*, std::vector<Node*>, decltype(compare)> pq(compare);

    pq.push(root);
    while (!pq.empty()) {
        Node* current = pq.top();
        pq.pop();
        
        // Collect appliances (relayPin != -1)
        if (current->relayPin != -1) {
            candidates.push_back(current);
        }
        
        for (Node* child : current->children) {
            if (child) pq.push(child);
        }
    }
    // candidates is now ordered best-first (highest heuristic first)
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
     * Pure best-first greedy knapsack selection:
     * The candidates are already ordered by descending heuristic (priority - friction)
     * because scout() performed a best-first traversal.
     * We now greedily activate each automatic load if it fits within the remaining budget.
     * This is a true greedy algorithm – no backtracking, no DP.
     */
    // TOLERANCE avoids floating-point comparison issues; renamed from EPS to avoid ESP32 macro conflict
    const float TOLERANCE = 1e-6f;
    for (int i = 0; i < N; i++) {
        Node* node = candidates[i];
        if (node->isFixed()) continue;   // already handled

        if ((node->currentDraw <= remainingCurrent + TOLERANCE) &&
            (node->power       <= remainingPower   + TOLERANCE)) {
            selected[i] = true;
            remainingCurrent -= node->currentDraw;
            remainingPower   -= node->power;
        } else {
            selected[i] = false;
        }
    }

    // --- Energy accounting and state application ---
    unsigned long nowMs = millis();
    for (int i = 0; i < N; i++) {
        Node* n = candidates[i];
        if (n->relayPin == -1) continue;
        n->accumulateEnergy(nowMs);
        n->isActive = selected[i];
    }
}