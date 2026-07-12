#include "../include/TimeManager.h"
#include <time.h>

static bool timeInitialised = false;

void TimeManager::begin(const char* ntpServer,
                         long gmtOffsetSec,
                         int daylightOffsetSec) {
    // Configure NTP
    configTime(gmtOffsetSec, daylightOffsetSec, ntpServer);

    // Wait until the time is set (timeout after 10 seconds)
    struct tm timeInfo;
    int retries = 0;
    while (!getLocalTime(&timeInfo) && retries < 20) {
        delay(500);
        retries++;
    }

    if (getLocalTime(&timeInfo)) {
        timeInitialised = true;
        Serial.println("Time synchronised");
    } else {
        Serial.println("Failed to obtain time from NTP");
        timeInitialised = false;
    }
}

bool TimeManager::isTimeValid() {
    return timeInitialised;
}

String TimeManager::getFormattedTime(const char* format) {
    struct tm timeInfo;
    if (!getLocalTime(&timeInfo)) {
        return "Time not available";
    }
    char buffer[64];
    strftime(buffer, sizeof(buffer), format, &timeInfo);
    return String(buffer);
}