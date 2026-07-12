#include "BestFirstSearch.h"

#include <Arduino.h>      // required for millis
#include <algorithm>      // required for std::max

BestFirstSearch::BestFirstSearch() {
    // Root is the entry point (the very top circle in your image)
    root = new Node();
    root->id = 0;
    root->name = "Main_DB";
    root->currentDraw = 0.0f;
    root->voltage = 0.0f;
    root->power = 0.0f;
    root->energyWh = 0.0f;
    root->lastActiveUpdateMs = millis();
    root->priority = 0;
    root->relayPin = -1;
    root->isForced = true;
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

Node* BestFirstSearch::createAndAddNode(const String& parentName, const String& name, float amps, int priority, int pin, float friction, bool forced, float voltage) {
    // If parent is not specified or Main_DB, start at root
    Node* parent = (parentName == "Main_DB" || parentName == "") ? root : findByName(root, parentName);

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
    newNode->isForced = forced;
    newNode->isActive = false;

    parent->children.push_back(newNode);
    return newNode;
}

bool BestFirstSearch::updateNode(const String& name, float newAmps, int newPriority, bool forced, float newVoltage) {
    Node* target = findByName(root, name);
    if (target) {
        target->currentDraw = newAmps;
        if (newVoltage > 0.0f) {
            target->voltage = newVoltage;
        }
        target->recalculatePower();
        target->priority = newPriority;
        target->isForced = forced;
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
    if (nodeName == "Main_DB") return false; 

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

std::vector<Node*> BestFirstSearch::scout(float availableCurrent) {
    std::vector<Node*> candidates;
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
    return candidates;
}

void BestFirstSearch::execute(std::vector<Node*> candidates, float C_available, float P_available) {
    int N = candidates.size();
    if (N == 0) return;

    float safeCurrent = (C_available > 0.0f) ? C_available : 0.0f;
    float safePower = (P_available > 0.0f) ? P_available : 0.0f;
    const int POWER_SCALE = 10; // 1 DP power unit = 10 W
    int W = (int)(safePower / POWER_SCALE);
    if (W < 0) W = 0;

    float forcedCurrent = 0.0f;
    float forcedPower = 0.0f;
    for (Node* n : candidates) {
        n->recalculatePower();
        if (n->isForced && n->isActive) {
            forcedCurrent += n->currentDraw;
            forcedPower += n->power;
        }
    }

    float remainingCurrent = safeCurrent - forcedCurrent;
    float remainingPower = safePower - forcedPower;
    if (remainingCurrent < 0.0f) remainingCurrent = 0.0f;
    if (remainingPower < 0.0f) remainingPower = 0.0f;

    int remainingW = (int)(remainingPower / POWER_SCALE);
    if (remainingW < 0) remainingW = 0;

    std::vector<bool> selected(N, false);
    for (int i = 0; i < N; i++) {
        if (candidates[i]->isForced) selected[i] = candidates[i]->isActive;
    }

    std::vector<int> nonForcedIndices;
    for (int i = 0; i < N; i++) {
        if (!candidates[i]->isForced) nonForcedIndices.push_back(i);
    }
    int M = nonForcedIndices.size();

    std::vector<std::vector<int>> K(M + 1, std::vector<int>(remainingW + 1, 0));
    for (int i = 1; i <= M; i++) {
        Node* node = candidates[nonForcedIndices[i - 1]];
        int P_i = (int)(node->power / POWER_SCALE);
        if (P_i < 0) P_i = 0;
        int U_i = node->priority;
        for (int w = 0; w <= remainingW; w++) {
            if (P_i <= w && node->currentDraw <= remainingCurrent && node->power <= remainingPower) {
                K[i][w] = max(U_i + K[i - 1][w - P_i], K[i - 1][w]);
            } else {
                K[i][w] = K[i - 1][w];
            }
        }
    }

    int res = K[M][remainingW];
    int w_limit = remainingW;
    for (int i = M; i > 0 && res > 0; i--) {
        Node* node = candidates[nonForcedIndices[i - 1]];
        int P_i = (int)(node->power / POWER_SCALE);
        if (P_i < 0) P_i = 0;
        if (res == K[i - 1][w_limit]) {
            selected[nonForcedIndices[i - 1]] = false;
        } else {
            selected[nonForcedIndices[i - 1]] = true;
            res -= node->priority;
            w_limit -= P_i;
        }
    }

    float totalCurrent = forcedCurrent;
    for (int i = 0; i < N; i++) {
        if (selected[i] && !candidates[i]->isForced) totalCurrent += candidates[i]->currentDraw;
    }

    if (totalCurrent > safeCurrent) {
        std::vector<int> order;
        for (int i = 0; i < N; i++) {
            if (selected[i] && !candidates[i]->isForced) order.push_back(i);
        }
        std::sort(order.begin(), order.end(), [&](int a, int b) {
            float ratioA = candidates[a]->priority / max(0.1f, candidates[a]->currentDraw);
            float ratioB = candidates[b]->priority / max(0.1f, candidates[b]->currentDraw);
            return ratioA < ratioB;
        });
        for (int idx : order) {
            if (totalCurrent <= safeCurrent) break;
            selected[idx] = false;
            totalCurrent -= candidates[idx]->currentDraw;
        }
    }

    // Physical Actuation and energy accounting
    unsigned long nowMs = millis();
    for (int i = 0; i < N; i++) {
        Node* n = candidates[i];
        if (n->relayPin == -1) continue;
        n->isActive = selected[i];
        n->accumulateEnergy(nowMs);
    }
}