#include <Arduino.h>
#include <WiFi.h>
#include "config.h"
#include "ignition.h"
#include "sensors.h"
#include "webserver.h"
#include "camera.h"

extern "C" void webui_loop();

void setup() {
    Serial.begin(115200);
    Serial.println("\n[Flat4Monitor] Booting...");

    // Load saved WiFi config from NVS
    Config::load();
    Serial.printf("[Config] SSID: %s\n", Config::getSSID().c_str());

    // 1. WiFi first — lwIP allocates its mailboxes (mbox) from DRAM here.
    //    Camera must come AFTER so it doesn't exhaust DRAM before lwIP starts.
    WiFi.mode(WIFI_AP);
    if (WiFi.softAP(Config::getSSID().c_str(), Config::getPass().c_str())) {
        Serial.printf("[WiFi] AP started: %s  IP: %s\n",
                      Config::getSSID().c_str(),
                      WiFi.softAPIP().toString().c_str());
    } else {
        Serial.println("[WiFi] AP start FAILED, using defaults");
        WiFi.softAP(DEFAULT_AP_SSID, DEFAULT_AP_PASS);
    }

    // 2. Sensors & ignition — attachInterrupt() installs GPIO ISR service
    //    before camera driver, preventing the "already installed" log.
    Sensors::begin();
    Ignition::begin();
    Serial.println("[Sensors] ADC initialized");
    Serial.println("[Ignition] Interrupts attached");

    // 3. Camera last — frame buffers go to PSRAM, no DRAM competition.
    if (!Camera_begin()) {
        Serial.println("[Camera] WARNING: Camera not available");
    }

    // 4. Web servers
    WebUI::begin();
    Camera_startStreamServer();
    Serial.println("[System] Ready. Connect to AP and open http://192.168.4.1");
    Serial.println("[System] Camera stream: http://192.168.4.1:81/stream");
}

void loop() {
    webui_loop(); // Broadcast sensor/ignition data via WebSocket
    delay(10);
}
