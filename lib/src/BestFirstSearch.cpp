#include "BestFirstSearch.h"

#include <Arduino.h>      // required for millis
#include <algorithm>      // required for std::max
#include <cmath>          // required for ceil/floor in resource scaling

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
    const int CURRENT_SCALE = 10;   // 1 DP current unit = 0.1 A
    const int POWER_SCALE = 10;     // 1 DP power unit = 10 W

    float fixedCurrent = 0.0f;
    float fixedPower = 0.0f;
    for (Node* n : candidates) {
        n->recalculatePower();
        if (n->isFixed() && n->isActive) {
            fixedCurrent += n->currentDraw;
            fixedPower += n->power;
        }
    }

    float remainingCurrent = safeCurrent - fixedCurrent;
    float remainingPower = safePower - fixedPower;
    if (remainingCurrent < 0.0f) remainingCurrent = 0.0f;
    if (remainingPower < 0.0f) remainingPower = 0.0f;

    int remainingC = (int)std::floor(remainingCurrent * CURRENT_SCALE + 0.0001f);
    int remainingW = (int)std::floor(remainingPower / POWER_SCALE + 0.0001f);
    if (remainingC < 0) remainingC = 0;
    if (remainingW < 0) remainingW = 0;

    std::vector<bool> selected(N, false);
    for (int i = 0; i < N; i++) {
        if (candidates[i]->isFixed()) selected[i] = candidates[i]->isActive;
    }

    std::vector<int> autoIndices;
    for (int i = 0; i < N; i++) {
        if (!candidates[i]->isFixed()) autoIndices.push_back(i);
    }
    int M = autoIndices.size();

    const size_t cells = (size_t)(M + 1) * (size_t)(remainingC + 1) * (size_t)(remainingW + 1);
    const size_t bytes = cells * sizeof(int);
    const size_t MAX_DP_BYTES = 120 * 1024;
    if (bytes > MAX_DP_BYTES) {
#ifdef ARDUINO
        Serial.printf("Optimisation skipped: DP table too large (%u bytes)\n", (unsigned)bytes);
#endif
        unsigned long nowMs = millis();
        for (Node* n : candidates) {
            if (n && n->relayPin != -1) n->accumulateEnergy(nowMs);
        }
        return;
    }

    auto index = [&](int i, int c, int w) {
        return ((i * (remainingC + 1)) + c) * (remainingW + 1) + w;
    };

    std::vector<int> K((M + 1) * (remainingC + 1) * (remainingW + 1), 0);
    for (int i = 1; i <= M; i++) {
        Node* node = candidates[autoIndices[i - 1]];
        int C_i = (int)std::ceil(node->currentDraw * CURRENT_SCALE - 0.0001f);
        int P_i = (int)std::ceil(node->power / POWER_SCALE - 0.0001f);
        if (C_i < 0) C_i = 0;
        if (P_i < 0) P_i = 0;
        int U_i = node->priority;

        for (int c = 0; c <= remainingC; c++) {
            for (int w = 0; w <= remainingW; w++) {
                int skip = K[index(i - 1, c, w)];
                int take = skip;
                if (C_i <= c && P_i <= w && node->currentDraw <= remainingCurrent && node->power <= remainingPower) {
                    take = U_i + K[index(i - 1, c - C_i, w - P_i)];
                }
                K[index(i, c, w)] = max(take, skip);
            }
        }
    }

    int c_limit = remainingC;
    int w_limit = remainingW;
    for (int i = M; i > 0; i--) {
        Node* node = candidates[autoIndices[i - 1]];
        int C_i = (int)std::ceil(node->currentDraw * CURRENT_SCALE - 0.0001f);
        int P_i = (int)std::ceil(node->power / POWER_SCALE - 0.0001f);
        if (C_i < 0) C_i = 0;
        if (P_i < 0) P_i = 0;

        if (C_i <= c_limit && P_i <= w_limit &&
            K[index(i, c_limit, w_limit)] == node->priority + K[index(i - 1, c_limit - C_i, w_limit - P_i)]) {
            selected[autoIndices[i - 1]] = true;
            c_limit -= C_i;
            w_limit -= P_i;
        } else {
            selected[autoIndices[i - 1]] = false;
        }
    }

    // The selector only needs the DP result above; no historical state
    // chain is tracked in this implementation, so nothing else is applied here.

    // Energy accounting must use the state that was active during the
    // elapsed interval.  Accumulate before applying the newly selected state
    // so transitions from ON->OFF do not drop the just-finished runtime and
    // transitions from OFF->ON do not backfill energy for idle time.
    unsigned long nowMs = millis();
    for (int i = 0; i < N; i++) {
        Node* n = candidates[i];
        if (n->relayPin == -1) continue;
        n->accumulateEnergy(nowMs);
        n->isActive = selected[i];
    }
}
