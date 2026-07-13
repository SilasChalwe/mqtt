#ifndef TEST_ARDUINO_H
#define TEST_ARDUINO_H

#include <algorithm>
#include <cstdint>
#include <string>

using String = std::string;

template <typename T>
T max(T a, T b) {
    return std::max(a, b);
}

inline unsigned long millis() {
    static unsigned long now = 0;
    now += 100;
    return now;
}

#endif


/*

Neither directly. Friction isn't a physical distance measurement. It's a penalty you assign to a node, and it represents the electrical "badness" of the path from the Main Distribution Board (root) to that specific appliance.

Here’s how to think about it:

The tree starts at Main_DB (the root).

Each child node is a sub-circuit, junction box, or appliance further down the wiring.

If a particular branch has a long cable run, thin wires, or shared conduits that cause significant voltage drop, you give the leaf appliance node a positive friction value to reflect that loss.

Friction is cumulative in the heuristic only for that node – it doesn't propagate upward, it's just a property of the node itself.

So it's relative to the distribution board, in the sense that the whole system is powered from that root, and you're penalising a load that is electrically "further" or "lossier" from the DB.
The microcontroller doesn't measure this; you decide it based on your wiring plan.

Practical example:
Living room lights (priority 10, friction 0) – short cable run, high priority.

Garden shed lights (priority 10, friction 5) – long outdoor cable with voltage drop, same priority but lower effective score because of friction.

In the best‑first search, the living room lights will be chosen before the shed lights when budget is tight, because

Priority → Integer, typically from 0 to 10 (or 0 to 100).

0 = least important, turn off first when power is scarce.

Higher number = more important, keep on if possible.

Friction → Float (can have decimals), always positive or zero.

0.0 = no penalty, circuit is perfect.

5.0 (example) = adds a penalty of 5 to the “desirability” score.

Usually choose a value smaller than the typical priority (e.g., 0 to 5) so it doesn’t completely cancel the priority.


*/