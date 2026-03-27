#pragma once
#include <Arduino.h>
#include "config.h"

struct IgnitionData {
    float  rpm;                  // Engine RPM (from cyl1 period)
    float  timingDeg[4];         // Timing offset in degrees rel. to cyl1 (cyl1=0)
    bool   firing[4];            // Pulse detected recently
    bool   misfire[4];           // Cylinder silent for >1.5x expected period
    bool   engineRunning;
};

class Ignition {
public:
    static void begin();
    static IgnitionData getData();

private:
    // Interrupt handlers (must be IRAM_ATTR)
    static void IRAM_ATTR isr_cyl1();
    static void IRAM_ATTR isr_cyl2();
    static void IRAM_ATTR isr_cyl3();
    static void IRAM_ATTR isr_cyl4();

    // Volatile timestamps (microseconds) of last rising edge per cylinder
    static volatile uint64_t _lastTime[4];
    // Period of cyl1 (for RPM)
    static volatile uint64_t _cyl1Period;
    // Reference timestamp of cyl1 rising edge
    static volatile uint64_t _cyl1Ref;
    // Firing flag (set by ISR, cleared by getData)
    static volatile bool _fired[4];
};
