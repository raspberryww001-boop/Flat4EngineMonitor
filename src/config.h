#pragma once
#include <Arduino.h>
#include <Preferences.h>

// === GPIO Pin Definitions ===
// Reassigned for ESP32-CAM (AI-Thinker) compatibility.
// Camera module occupies: GPIO 4,5,18,19,21,22,23,25,26,27,34,35,36,39
//   (Freenove: PWDN=-1, XCLK=21 frees GPIO 0; D0=4 occupies GPIO 4)
// PSRAM occupies: GPIO 16,17
// Available for application: GPIO 0,2,12,13,14,15

// Ignition timing inputs (TTL-level signals from conversion circuit)
// GPIO 12–15 are shared with SD card slot; SD card must NOT be inserted.
#define PIN_IGN_CYL1   12   // Cylinder 1 (reference)
#define PIN_IGN_CYL2   13   // Cylinder 2
#define PIN_IGN_CYL3   14   // Cylinder 3
#define PIN_IGN_CYL4   15   // Cylinder 4

// Camera illumination LED
// GPIO 33 is free on AI-Thinker ESP32-CAM (not used by camera, PSRAM, or SD).
// Connect: GPIO 33 → 100Ω resistor → LED anode → LED cathode → GND
// Active HIGH (HIGH = LED on). On-board red LED also responds to this pin.
#define PIN_CAM_LED    33

// ADC inputs — NOTE: These are ADC2 channels.
// ADC2 is shared with the WiFi RF path and gives unreliable readings while
// WiFi is active. Oil pressure / temperature are slow-changing values so
// brief dropout or jitter is tolerable for this application.
#define PIN_OIL_PRESS   2   // Oil pressure sensor (ADC2_CH2)
#define PIN_OIL_TEMP    0   // Oil temperature sensor (ADC2_CH1)
                            // GPIO 0 freed because Freenove uses GPIO 21 for XCLK (not GPIO 0)
                            // Note: GPIO 0 has internal pull-up; sensor must not pull LOW at boot

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
#define IGN_DEBOUNCE_US     2000     // Ignore re-triggers within 2 ms of last pulse

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
