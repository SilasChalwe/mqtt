#ifndef TREE_STORAGE_H
#define TREE_STORAGE_H

#include <Arduino.h>
#include "../lib/src/BestFirstSearch.h"

namespace TreeStorage {
    static constexpr const char* TreePath = "/tree.txt";

    bool begin();
    bool save(BestFirstSearch& bfs);
    bool load(BestFirstSearch& bfs);
    String toJson(BestFirstSearch& bfs);
}

#endif
