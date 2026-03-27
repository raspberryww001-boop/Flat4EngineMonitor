#pragma once
#include <Arduino.h>
#include <ESPAsyncWebServer.h>
#include "ignition.h"
#include "config.h"

class WebUI {
public:
    static void begin();
    static void broadcastData();  // called from webui_loop() in main

private:
    static AsyncWebServer _server;
    static AsyncWebSocket _ws;

    static void onWsEvent(AsyncWebSocket* server, AsyncWebSocketClient* client,
                          AwsEventType type, void* arg, uint8_t* data, size_t len);
    static String buildJsonData();

    static unsigned long _lastBroadcast;
    static const unsigned long BROADCAST_INTERVAL_MS = 100; // 10 Hz
};
