#ifndef BEST_FIRST_SEARCH_H
#define BEST_FIRST_SEARCH_H

#include <Arduino.h>
#include <vector>
#include <queue>
#include <algorithm>
#include <functional>

// Represents a node in the Distribution Tree (Branch or Leaf)
struct Node {
    int id;
    String name;          // Using String to prevent memory corruption/static errors
    float currentDraw;    // Amps
    float voltage;        // Volts for this appliance (default system voltage)
    float power;          // Watts, computed as voltage * current
    float energyWh;       // Watt-hours consumed while active
    unsigned long lastActiveUpdateMs; // Last time energy was accumulated
    int priority;         // Utility score
    int relayPin;         // -1 = Branch (White Node), >= 0 = Appliance (Orange Node)
    bool isForced;        // Critical load
    float wireFriction;   // Heuristic cost g(n)
    bool isActive;        // Current relay state
    
    std::vector<Node*> children; 
};

class BestFirstSearch {
private:
    Node* root;
    Node* findByName(Node* current, const String& name);
    void recursiveDelete(Node* node);

public:
    BestFirstSearch();
    
    Node* getRoot() { return root; }

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

    // [ACTUATE] Relay Actuation logic (The Knapsack Engine)
    void execute(std::vector<Node*> candidates, float C_available, float P_available);
};

#endif