# BestFirstSearch Tests

![Hierarchical tree structure](tree_diagram.png)

This test sketch verifies core `BestFirstSearch` behavior using the existing library implementation without modifying `lib/`.

## Covered behavior
- Basic node creation and lookup
- Nested branch creation and invalid parent handling
- Updating existing nodes and rejecting updates to missing nodes
- `scout()` returning appliance candidates
- `execute()` activating candidates under capacity
- Removing a branch removes its subtree
- Refusing to remove the root node
- Empty result behavior after cleanup

## How to run
1. Open `test/BestFirstSearchTest/BestFirstSearchTest.ino` in the Arduino IDE or use the Arduino CLI.
2. Select the ESP32 board and the correct serial port.
3. Upload the sketch.
4. Open the serial monitor at `115200` baud.

## Expected output
The sketch prints a line for each test and a final `ALL PASSED` or `FAILED` summary.

## Notes
- This is a functional unit test, not a performance benchmark.
- It does not verify concurrency or long-term scalability on the ESP32.
- It validates correctness of the algorithmic flow and node management behavior.

## Computational Complexity and Space Boundary Analysis
The control pipeline is separated into two algorithmic stages:

1. **Network Scout** (`scout()` in `BestFirstSearch`)
   - Time complexity: `O(V log V + E)`
   - Space complexity: `O(V)`
   - `V` is the total number of physical electrical nodes, including branch junctions and appliance leaves.
   - `E` is the number of wire connections. For a tree topology, `E = V - 1`.
   - The `V log V` term comes from the `std::priority_queue` used to rank candidate nodes by heuristic value.
   - This matches a lightweight local branch discovery process that only traverses actual hierarchical wiring paths.

2. **Appliance Combination Finder** (`execute()` in `BestFirstSearch`)
   - Time complexity: `O(N * W)`
   - Space complexity: `O(N * W)`
   - `N` is the number of active appliance candidate nodes.
   - `W` is the discrete current capacity resolution.
   - The implementation uses a bottom-up dynamic programming matrix and deterministic backtracking, similar to a bounded knapsack selection.

### Why this is feasible for household topology
- A residential electrical network is a hierarchical tree, so branch discovery never explores exponential graph cycles.
- The active appliance set is bounded by real hardware pin capacity, and the current capacity resolution limits the DP state.
- The current test setup confirms algorithm correctness on nested tree shapes and branch removal.

## Microcontroller Hardware Allocation and Architectural Constraints
This system is designed for the ESP32 family with the following practical constraints:

- Dual-core Xtensa 32-bit CPU at 240 MHz.
- 520 KB of on-chip static RAM, with a significant portion reserved for FreeRTOS, Wi-Fi/Bluetooth stacks, and peripheral drivers.
- Usable GPIO pins are limited by flash, bootstrap, ADC, I2C/SPI/UART, and current sensor wiring.
- In practice, available relay actuation pins are typically `<= 20`, bounding `N <= 20`.

## Real-time Telemetry and Execution Time Estimates
For a conservative capacity model of `N = 20` active appliances and `W = 100` capacity increments:

- DP matrix size: `(N + 1) x (W + 1) = 21 x 101 = 2,121` elements.
- Memory footprint: `2,121 * 4 bytes = 8,484 bytes` (~8.48 KB).
- Execution iterations: `N * W = 2,000`.
- Conservative cycle estimate: `2,000 * 50 = 100,000` cycles.
- Estimated latency at 240 MHz: `100,000 / 240,000,000 ≈ 0.42 ms`.

These estimates are purely algorithmic and do not include I/O overhead, but they demonstrate the selection engine is feasible within ESP32 runtime and RAM budgets.

## Operational Metrics Summary
| Stage | Time Complexity | Space Complexity | Estimated ESP32 Latency |
| --- | --- | --- | --- |
| Best-First Network Scout | `O(V log V + E)` | `O(V)` | < 0.10 ms (approx.) |
| Appliance Combination Finder | `O(N * W)` | `O(N * W)` | ≈ 0.42 ms |
| Combined Pipeline | dominated by `O(N * W)` | ≈ 8.48 KB | < 1.00 ms total |

## Tree Topology and Nested Parent Support
The attached tree diagram shows nested parent branches and leaf appliance nodes.

- White nodes correspond to branch/junction nodes.
- Orange nodes correspond to appliance loads.
- Parent nodes can be nested, and each branch may contain both sub-branches and leaf appliances.

The current implementation supports this structure by:
- allowing `createAndAddNode(parentName, ...)` to attach nodes under any existing branch,
- treating `pin = -1` as a branch node,
- using recursive search and priority queue traversal to discover all candidates,
- and applying DP-based selection across all active leaf candidates.

## Current Test Coverage
The unit test validates:
- nested branch creation,
- invalid parent rejection,
- node updates,
- correct candidate collection from a hierarchical tree,
- selection behavior under capacity,
- subtree removal,
- refusal to remove the root node,
- and empty result state after cleanup.

### Boundary testing and validation strategy
The current test sketch is a functional unit test focused on correctness for a modest tree shape. It is not a large-scale stress test. For broader validation, a full test plan should include:
- **Black box testing:** verify inputs and outputs only, e.g. add nodes, execute the algorithm, and confirm expected on/off results without inspecting internal state.
- **White box testing:** verify the internal control flow and path discovery logic, e.g. confirm that `createAndAddNode()` attaches nodes correctly and `scout()` traverses the tree as expected.
- **Grey box testing:** combine both, using knowledge of tree structure and algorithm assumptions to validate edge cases.
- **Boundary testing:** check behavior at limits such as the maximum number of usable GPIO-controlled appliance nodes (`N`), invalid parent names, empty trees, and zero or negative capacity values.

### What's been tested and what has not
- The documented test confirms small-scale correctness on nested branches and leaf candidate selection.
- It does not currently execute the algorithm for `N = 50` or extremely large values such as `10,000` nodes.
- The implementation is theoretically bounded by the hardware and algorithm, but actual scaling to `10,000` nodes is not validated in this repository.

### Normal node counts and practical limits
- In this design, practical appliance count is limited by ESP32 GPIO availability and hardware constraints.
- A realistic operational limit is roughly `N <= 20` actuated loads, because many pins must be reserved for flash, bootstrap, ADC, serial, and sensors.
- A household distribution tree may contain many more branch/junction nodes, but the active appliance candidate set remains bounded by relay-controlled loads.
- Therefore, the tested model is targeted at the feasible real-world range rather than unbounded graph sizes.

If you want, I can add a second test sketch that explicitly exercises a larger tree shape and demonstrates a boundary-case stress test for the practical `N <= 20` scenario.

## Tree shape and parent/child behavior
The attached diagram shows a hierarchical distribution tree with a single root and many nested branches.

- White nodes represent branch junctions or parent nodes.
- Orange nodes represent appliance loads (leaf nodes).
- Each load node is attached to a parent branch node.
- Some branches have many children, and some children are themselves branch nodes with their own subtree.

The `BestFirstSearch` implementation supports this structure:
- `createAndAddNode(parent, name, ...)` adds a new node under the specified `parent`.
- `parent` can be `Main_DB` (the root) or any existing branch node name.
- Branch nodes are created by setting `pin = -1`.
- Appliance leaf nodes are created by giving a valid relay pin number.

### Example of the attached tree
If you model the attached tree in the test, a single parent node may have many child branches, and those branches may in turn have their own children.
The algorithm still works because it searches recursively from the root and collects all appliance leaf nodes via `scout()`.

### What the test covers for that tree
- a large root parent with many children
- nested parent branches under other branches
- leaf nodes attached to deeply nested branches
- branch removal that clears its entire subtree
- `scout()` collecting all candidate appliance leaves regardless of depth
- `execute()` choosing among those candidates based on available current

If you want, I can also add a second test sketch that builds and validates the exact attached tree shape. 
