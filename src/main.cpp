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

    // Initialize ignition/sensors first so attachInterrupt() installs
    // the GPIO ISR service before the camera driver does — prevents the
    // "isr service already installed" error log from esp32-camera.
    Sensors::begin();
    Ignition::begin();
    Serial.println("[Sensors] ADC initialized");
    Serial.println("[Ignition] Interrupts attached");

    // Initialize camera (uses LEDC timers; after ISR service is up)
    if (!Camera_begin()) {
        Serial.println("[Camera] WARNING: Camera not available");
    }

    // Start WiFi Access Point

    // Start web server (port 80) and camera stream server (port 81)
    WebUI::begin();
    Camera_startStreamServer();
    Serial.println("[System] Ready. Connect to AP and open http://192.168.4.1");
    Serial.println("[System] Camera stream: http://192.168.4.1:81/stream");
}

void loop() {
    webui_loop(); // Broadcast sensor/ignition data via WebSocket
    delay(10);
}
