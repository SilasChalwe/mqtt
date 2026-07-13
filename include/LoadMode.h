#ifndef LOAD_MODE_H
#define LOAD_MODE_H

#include <Arduino.h>
#include <algorithm>
#include <cctype>
#include <string>

inline bool parseLoadModeValue(const char* typeValue, bool fallbackFixed) {
    if (typeValue == nullptr) return fallbackFixed;

    std::string value(typeValue);
    std::transform(value.begin(), value.end(), value.begin(), [](unsigned char c) {
        return static_cast<char>(std::tolower(c));
    });

    if (value == "fixed") return true;
    if (value == "auto") return false;
    return fallbackFixed;
}

inline bool parseLoadModeValue(std::nullptr_t, bool fallbackFixed) {
    return fallbackFixed;
}

template <typename T>
inline bool parseLoadModeValue(const T& typeValue, bool fallbackFixed) {
    return parseLoadModeValue(typeValue.c_str(), fallbackFixed);
}

#endif
