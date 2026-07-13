#ifndef TIME_MANAGER_H
#define TIME_MANAGER_H

#include <Arduino.h>

class TimeManager {
public:
    // Call once in setup() to start time synchronisation
    static void begin(const char* ntpServer,long gmtOffsetSec,int daylightOffsetSec);

    // Returns true if the time has been synchronised with NTP
    static bool isTimeValid();

    // Returns the current local time as a formatted string
    // format examples: "%Y-%m-%d %H:%M:%S" or "%H:%M:%S"
    static String getFormattedTime(const char* format = "%Y-%m-%d %H:%M:%S");

    // Returns minutes since midnight using the synchronised TimeManager clock, or -1 if unavailable.
    static int getMinutesSinceMidnight();
};

#endif
