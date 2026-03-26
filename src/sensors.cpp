#include "sensors.h"
#include <math.h>

void Sensors::begin() {
    // ESP32 ADC: 12-bit, 0–3.3V (attenuation 11dB for ~0–3.6V range)
    analogSetPinAttenuation(PIN_OIL_PRESS, ADC_11db);
    analogSetPinAttenuation(PIN_OIL_TEMP,  ADC_11db);
    analogReadResolution(12);
}

int Sensors::readADCAvg(int pin, int samples) {
    long sum = 0;
    for (int i = 0; i < samples; i++) {
        sum += analogRead(pin);
    }
    return (int)(sum / samples);
}

float Sensors::adcToOilPressure(int raw) {
    // Linear mapping from ADC range to bar range
    float bar = ((float)(raw - OIL_PRESS_ADC_MIN) /
                 (float)(OIL_PRESS_ADC_MAX - OIL_PRESS_ADC_MIN)) *
                (OIL_PRESS_BAR_MAX - OIL_PRESS_BAR_MIN) + OIL_PRESS_BAR_MIN;
    return constrain(bar, OIL_PRESS_BAR_MIN, OIL_PRESS_BAR_MAX);
}

float Sensors::adcToOilTemp(int raw) {
    if (raw <= 0) return -273.15f; // invalid
    // Voltage divider: Vout = Vcc * Rntc / (Rseries + Rntc)
    // ADC 12-bit max = 4095 (at 3.3V)
    float vcc    = 3300.0f; // mV (we work in ratio, so value doesn't matter)
    float ratio  = (float)raw / 4095.0f;       // Vout/Vcc
    float rNtc   = OIL_TEMP_R_SERIES * ratio / (1.0f - ratio);
    // Steinhart-Hart simplified (B parameter equation)
    float tempK  = 1.0f / (1.0f / OIL_TEMP_T0 + log(rNtc / OIL_TEMP_R0) / OIL_TEMP_B);
    return tempK - 273.15f;
}

SensorData Sensors::read() {
    static float cachedPressure = 0.0f;
    static float cachedTemp     = -99.0f;
    int rawP = readADCAvg(PIN_OIL_PRESS);
    int rawT = readADCAvg(PIN_OIL_TEMP);
    // ADC2 returns 0 when blocked by WiFi — keep last valid reading
    if (rawP > 0) cachedPressure = adcToOilPressure(rawP);
    if (rawT > 0) cachedTemp     = adcToOilTemp(rawT);
    SensorData d;
    d.oilPressureBar = cachedPressure;
    d.oilTempC       = cachedTemp;
    return d;
}
