#pragma once
#include <Arduino.h>
#include "config.h"

struct SensorData {
    float oilPressureBar;  // 0–10 bar
    float oilTempC;        // °C
};

class Sensors {
public:
    static void begin();
    static SensorData read();

private:
    // Smooth ADC with oversampling
    static int readADCAvg(int pin, int samples = 16);
    static float adcToOilPressure(int raw);
    static float adcToOilTemp(int raw);
};
