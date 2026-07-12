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
