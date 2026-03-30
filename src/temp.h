#pragma once
#include <Arduino.h>
#include "config.h"

class TempSensor {
public:
    static void  begin();
    static void  update();   // call from loop() — non-blocking, ~750ms conversion
    static float readC();    // latest reading in °C, or NAN if no sensor
    static bool  isReady();  // true once first valid reading is available

private:
    static float _lastTempC;
    static bool  _conversionPending;
    static unsigned long _convStartMs;
};
