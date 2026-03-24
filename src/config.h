#pragma once
#include <Arduino.h>
#include <Preferences.h>

// === GPIO Pin Definitions ===
// Ignition timing inputs (TTL-level signals from conversion circuit)
#define PIN_IGN_CYL1   32   // Cylinder 1 (reference)
#define PIN_IGN_CYL2   33   // Cylinder 2
#define PIN_IGN_CYL3   25   // Cylinder 3
#define PIN_IGN_CYL4   26   // Cylinder 4

// ADC inputs
#define PIN_OIL_PRESS  34   // Oil pressure sensor (ADC)
#define PIN_OIL_TEMP   35   // Oil temperature sensor (ADC)

// === ADC Calibration ===
// Oil pressure: 0–10 bar -> 0.5–4.5V (100–900 in 12-bit ADC range at 3.3V)
#define OIL_PRESS_ADC_MIN   310   // ~0.5V (0 bar)
#define OIL_PRESS_ADC_MAX   2790  // ~4.5V (10 bar)
#define OIL_PRESS_BAR_MIN   0.0f
#define OIL_PRESS_BAR_MAX   10.0f

// Oil temp: thermistor NTC 10k B=3950
#define OIL_TEMP_R_SERIES   10000.0f  // Series resistor (Ohm)
#define OIL_TEMP_R0         10000.0f  // NTC resistance at 25°C
#define OIL_TEMP_T0         298.15f   // 25°C in Kelvin
#define OIL_TEMP_B          3950.0f   // B coefficient

// === Ignition Timing ===
// 4-cylinder engine: cylinders fire every 180° crank
// Without TDC, cyl1 is used as 0° reference
#define FIRE_INTERVAL_DEG   180.0f   // degrees between cylinders (720° / 4)
#define RPM_TIMEOUT_MS      2000     // No signal timeout (engine stopped)

// === WiFi defaults (stored in NVS, overridable via WebUI) ===
#define DEFAULT_AP_SSID     "Flat4Monitor"
#define DEFAULT_AP_PASS     "vwbulli1968"
#define NVS_NAMESPACE       "flat4cfg"
#define NVS_KEY_SSID        "ssid"
#define NVS_KEY_PASS        "pass"

class Config {
public:
    static void load();
    static void save(const String& ssid, const String& pass);
    static String getSSID();
    static String getPass();

private:
    static String _ssid;
    static String _pass;
    static Preferences _prefs;
};
