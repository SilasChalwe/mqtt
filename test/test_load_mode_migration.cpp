#include "Arduino.h"
#include "../include/LoadMode.h"
#include <cassert>

static void fixed_type_is_parsed_as_fixed() {
    assert(parseLoadModeValue("fixed", false));
}

static void auto_type_is_parsed_as_auto() {
    assert(!parseLoadModeValue("auto", true));
}

static void unknown_type_uses_fallback() {
    assert(parseLoadModeValue("custom", true));
    assert(!parseLoadModeValue("custom", false));
}

int main() {
    fixed_type_is_parsed_as_fixed();
    auto_type_is_parsed_as_auto();
    unknown_type_uses_fallback();
    return 0;
}
