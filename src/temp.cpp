#include "temp.h"
#include <OneWire.h>
#include <DallasTemperature.h>

static OneWire          _ow(PIN_TEMP_DS18B20);
static DallasTemperature _dt(&_ow);

float        TempSensor::_lastTempC       = NAN;
bool         TempSensor::_conversionPending = false;
unsigned long TempSensor::_convStartMs     = 0;

// DS18B20 at 12-bit resolution takes up to 750 ms per conversion.
static const unsigned long CONV_TIMEOUT_MS = 800;

void TempSensor::begin() {
    _dt.begin();
    _dt.setResolution(12);
    _dt.setWaitForConversion(false); // non-blocking
    _dt.requestTemperatures();
    _conversionPending = true;
    _convStartMs       = millis();
    Serial.printf("[Temp] DS18B20 on GPIO %d — %d sensor(s) found\n",
                  PIN_TEMP_DS18B20, _dt.getDeviceCount());
}

void TempSensor::update() {
    if (!_conversionPending) return;
    if ((millis() - _convStartMs) < CONV_TIMEOUT_MS) return;

    float t = _dt.getTempCByIndex(0);
    if (t != DEVICE_DISCONNECTED_C && t != 85.0f) {
        // 85.0°C is the DS18B20 power-on default — ignore it
        _lastTempC = t;
    }
    // Kick off next conversion immediately
    _dt.requestTemperatures();
    _convStartMs = millis();
}

float TempSensor::readC()   { return _lastTempC; }
bool  TempSensor::isReady() { return !isnan(_lastTempC); }
