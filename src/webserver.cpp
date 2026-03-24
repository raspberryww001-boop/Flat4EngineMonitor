#include "webserver.h"
#include <LittleFS.h>
#include <ArduinoJson.h>

AsyncWebServer WebUI::_server(80);
AsyncWebSocket WebUI::_ws("/ws");
unsigned long  WebUI::_lastBroadcast = 0;

void WebUI::begin() {
    // Mount LittleFS
    if (!LittleFS.begin(true)) {
        Serial.println("[WebUI] LittleFS mount failed");
    }

    // WebSocket
    _ws.onEvent(onWsEvent);
    _server.addHandler(&_ws);

    // Serve static files from LittleFS
    _server.serveStatic("/", LittleFS, "/").setDefaultFile("index.html");

    // API: GET current data (JSON)
    _server.on("/api/data", HTTP_GET, [](AsyncWebServerRequest* req) {
        String json = WebUI::buildJsonData();
        req->send(200, "application/json", json);
    });

    // API: GET current WiFi config
    _server.on("/api/config", HTTP_GET, [](AsyncWebServerRequest* req) {
        StaticJsonDocument<256> doc;
        doc["ssid"] = Config::getSSID();
        doc["pass"] = Config::getPass();
        String out;
        serializeJson(doc, out);
        req->send(200, "application/json", out);
    });

    // API: POST new WiFi config
    _server.on("/api/config", HTTP_POST,
        [](AsyncWebServerRequest* req) {},
        nullptr,
        [](AsyncWebServerRequest* req, uint8_t* data, size_t len, size_t, size_t) {
            StaticJsonDocument<256> doc;
            if (deserializeJson(doc, data, len) == DeserializationError::Ok) {
                String ssid = doc["ssid"] | "";
                String pass = doc["pass"] | "";
                if (ssid.length() >= 2) {
                    Config::save(ssid, pass);
                    req->send(200, "application/json", "{\"ok\":true,\"restart\":true}");
                    delay(500);
                    ESP.restart();
                    return;
                }
            }
            req->send(400, "application/json", "{\"ok\":false}");
        }
    );

    // 404
    _server.onNotFound([](AsyncWebServerRequest* req) {
        req->send(404, "text/plain", "Not found");
    });

    _server.begin();
    Serial.println("[WebUI] HTTP server started");
}

void WebUI::onWsEvent(AsyncWebSocket* server, AsyncWebSocketClient* client,
                      AwsEventType type, void* arg, uint8_t* data, size_t len) {
    if (type == WS_EVT_CONNECT) {
        Serial.printf("[WS] Client #%u connected\n", client->id());
    } else if (type == WS_EVT_DISCONNECT) {
        Serial.printf("[WS] Client #%u disconnected\n", client->id());
    }
}

String WebUI::buildJsonData() {
    IgnitionData ign = Ignition::getData();
    SensorData   sen = Sensors::read();

    StaticJsonDocument<512> doc;
    doc["rpm"]         = (int)ign.rpm;
    doc["running"]     = ign.engineRunning;
    doc["oilPressBar"] = round(sen.oilPressureBar * 10.0f) / 10.0f;
    doc["oilTempC"]    = round(sen.oilTempC * 10.0f) / 10.0f;

    JsonArray timing = doc.createNestedArray("timing");
    JsonArray firing = doc.createNestedArray("firing");
    for (int i = 0; i < 4; i++) {
        timing.add(round(ign.timingDeg[i] * 10.0f) / 10.0f);
        firing.add(ign.firing[i]);
    }

    String out;
    serializeJson(doc, out);
    return out;
}

void WebUI::broadcastData() {
    if (millis() - _lastBroadcast < BROADCAST_INTERVAL_MS) return;
    _lastBroadcast = millis();
    if (_ws.count() == 0) return;
    _ws.textAll(buildJsonData());
    _ws.cleanupClients();
}

// Call this from loop()
extern "C" void webui_loop() {
    WebUI::broadcastData();
}
