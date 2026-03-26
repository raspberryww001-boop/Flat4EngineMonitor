#include "sensors.h"
#include <math.h>
#include <driver/adc.h>

// Map GPIO pin number to ADC2 channel (ESP32 AI-Thinker)
static adc2_channel_t pinToAdc2Ch(int pin) {
    switch (pin) {
        case  0: return ADC2_CHANNEL_1;
        case  2: return ADC2_CHANNEL_2;
        case  4: return ADC2_CHANNEL_0;
        case 12: return ADC2_CHANNEL_5;
        case 13: return ADC2_CHANNEL_4;
        case 14: return ADC2_CHANNEL_6;
        case 15: return ADC2_CHANNEL_3;
        case 25: return ADC2_CHANNEL_8;
        case 26: return ADC2_CHANNEL_9;
        case 27: return ADC2_CHANNEL_7;
        default: return (adc2_channel_t)-1;
    }
}

void Sensors::begin() {
    // Use IDF ADC2 API directly to avoid Arduino HAL error-log spam when WiFi
    // is active (ADC2 shared with Wi-Fi RF path).
    adc2_config_channel_atten(pinToAdc2Ch(PIN_OIL_PRESS), ADC_ATTEN_DB_11);
    adc2_config_channel_atten(pinToAdc2Ch(PIN_OIL_TEMP),  ADC_ATTEN_DB_11);
}

// Returns -1 when ADC2 is blocked by Wi-Fi (no error log emitted).
int Sensors::readADCAvg(int pin, int samples) {
    adc2_channel_t ch = pinToAdc2Ch(pin);
    long sum = 0;
    int valid = 0;
    for (int i = 0; i < samples; i++) {
        int raw = -1;
        if (adc2_get_raw(ch, ADC_WIDTH_BIT_12, &raw) == ESP_OK && raw >= 0) {
            sum += raw;
            valid++;
        }
    }
    return (valid > 0) ? (int)(sum / valid) : -1;
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
    // -1 means ADC2 blocked by WiFi — keep last valid reading
    if (rawP >= 0) cachedPressure = adcToOilPressure(rawP);
    if (rawT >= 0) cachedTemp     = adcToOilTemp(rawT);
    SensorData d;
    d.oilPressureBar = cachedPressure;
    d.oilTempC       = cachedTemp;
    return d;
}
