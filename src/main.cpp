#include <Arduino.h>
#include <WiFi.h>
#include "config.h"
#include "ignition.h"
#include "sensors.h"
#include "webserver.h"

extern "C" void webui_loop();

void setup() {
    Serial.begin(115200);
    Serial.println("\n[Flat4Monitor] Booting...");

    // Load saved WiFi config from NVS
    Config::load();
    Serial.printf("[Config] SSID: %s\n", Config::getSSID().c_str());

    // Start WiFi Access Point
    WiFi.mode(WIFI_AP);
    bool ok = WiFi.softAP(Config::getSSID().c_str(), Config::getPass().c_str());
    if (ok) {
        Serial.printf("[WiFi] AP started: %s  IP: %s\n",
                      Config::getSSID().c_str(),
                      WiFi.softAPIP().toString().c_str());
    } else {
        Serial.println("[WiFi] AP start FAILED, using defaults");
        WiFi.softAP(DEFAULT_AP_SSID, DEFAULT_AP_PASS);
    }

    // Initialize hardware
    Sensors::begin();
    Ignition::begin();
    Serial.println("[Sensors] ADC initialized");
    Serial.println("[Ignition] Interrupts attached");

    // Start web server
    WebUI::begin();
    Serial.println("[System] Ready. Connect to AP and open http://192.168.4.1");
}

void loop() {
    webui_loop(); // Broadcast sensor/ignition data via WebSocket
    delay(10);
}
