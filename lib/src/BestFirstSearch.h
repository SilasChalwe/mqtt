#ifndef BEST_FIRST_SEARCH_H
#define BEST_FIRST_SEARCH_H

#include <Arduino.h>
#include <vector>
#include <queue>
#include <algorithm>
#include <functional>
#include "../../include/Node.h"

class BestFirstSearch {
private:
    Node* root;
    String rootName;
    Node* findByName(Node* current, const String& name);
    void recursiveDelete(Node* node);

public:
    BestFirstSearch();
    
    Node* getRoot() { return root; }
    void clear();

    // [CREATE] Define new nodes and link them automatically to a parent
    Node* createAndAddNode(const String& parentName, const String& name, float amps, int priority, int pin, float friction = 0.1, bool forced = false, float voltage = 230.0f);
    
    // [READ] Search for a node anywhere in the tree
    Node* getNode(const String& name) { return findByName(root, name); }

    // [UPDATE] Dynamically change node properties
    bool updateNode(const String& name, float newAmps, int newPriority, bool forced = false, float newVoltage = -1.0f);

    // [DELETE] Remove a load or a whole branch and free memory
    bool removeNode(const String& nodeName);

    // [ALGORITHM] Stage 2: BFS Path Optimization (Scout)
    std::vector<Node*> scout(float availableCurrent);

    // [OPTIMISE] Select active states and update energy accounting. Hardware actuation is handled by RelayControl.
    void execute(std::vector<Node*> candidates, float C_available, float P_available);
};

#endif