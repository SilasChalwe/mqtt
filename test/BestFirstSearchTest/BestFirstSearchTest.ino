#include <Arduino.h>
#include "../../lib/src/BestFirstSearch.cpp"

void reportResult(bool passed, const char* name) {
    Serial.print("Test ");
    Serial.print(name);
    Serial.print(':');
    Serial.println(passed ? " PASS" : " FAIL");
}

bool executeTest(const char* name, bool result, bool& allPassed) {
    reportResult(result, name);
    allPassed &= result;
    return result;
}

void runBestFirstSearchTests() {
    bool allPassed = true;

    BestFirstSearch bfs;

    executeTest("Root exists", bfs.getNode("Main_DB") != nullptr, allPassed);

    Node* nodeA = bfs.createAndAddNode("Main_DB", "A", 1.0, 10, -1, 0.1, false);
    executeTest("Create A", nodeA != nullptr && bfs.getNode("A") == nodeA, allPassed);

    Node* nodeB = bfs.createAndAddNode("A", "B", 3.0, 5, -1, 0.1, false);
    executeTest("Create B", nodeB != nullptr && bfs.getNode("B") == nodeB, allPassed);

    Node* nodeC = bfs.createAndAddNode("A", "C", 2.0, 15, -1, 0.1, false);
    executeTest("Create C", nodeC != nullptr && bfs.getNode("C") == nodeC, allPassed);

    Node* nodeD = bfs.createAndAddNode("C", "D", 4.0, 25, -1, 0.1, false);
    executeTest("Create D", nodeD != nullptr && bfs.getNode("D") == nodeD, allPassed);

    Node* invalidParent = bfs.createAndAddNode("Missing", "X", 1.0, 1, -1, 0.1, false);
    executeTest("Invalid parent fails", invalidParent == nullptr, allPassed);

    bool updatedA = bfs.updateNode("A", 2.5, 20, true);
    executeTest("Update A", updatedA && nodeA->currentDraw == 2.5f && nodeA->priority == 20 && nodeA->isForced, allPassed);

    bool updateMissing = !bfs.updateNode("Missing", 0.0, 0, false);
    executeTest("Update missing fails", updateMissing, allPassed);

    auto candidates = bfs.scout(10.0);
    bool scoutHasTwo = (candidates.size() == 2 || candidates.size() == 3) && (std::find(candidates.begin(), candidates.end(), nodeB) != candidates.end()) && (std::find(candidates.begin(), candidates.end(), nodeD) != candidates.end());
    executeTest("Scout contains appliances", scoutHasTwo, allPassed);

    bfs.execute(candidates, 5.0);
    bool executeActivated = nodeD->isActive || nodeB->isActive;
    executeTest("Execute activates candidate", executeActivated, allPassed);

    bool removeBranchA = bfs.removeNode("A") && bfs.getNode("A") == nullptr && bfs.getNode("B") == nullptr && bfs.getNode("C") == nullptr && bfs.getNode("D") == nullptr;
    executeTest("Remove branch A", removeBranchA, allPassed);

    bool removeRoot = !bfs.removeNode("Main_DB");
    executeTest("Remove root fails", removeRoot, allPassed);

    bool emptyScout = bfs.scout(10.0).empty();
    executeTest("Scout empty after removal", emptyScout, allPassed);

    Serial.println();
    Serial.print("BestFirstSearch tests: ");
    Serial.println(allPassed ? "ALL PASSED" : "FAILED");
}

void setup() {
    Serial.begin(115200);
    while (!Serial);
    Serial.println("Starting BestFirstSearch unit test...");
    runBestFirstSearchTests();
}

void loop() {
    // Nothing to do after tests complete.
}
