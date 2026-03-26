#include "camera.h"
#include "config.h"
#include <esp_camera.h>
#include <WiFi.h>

// ---- AI-Thinker ESP32-CAM pin map ----
#define CAM_PIN_PWDN    32
#define CAM_PIN_RESET   -1
#define CAM_PIN_XCLK     0
#define CAM_PIN_SIOD    26
#define CAM_PIN_SIOC    27
#define CAM_PIN_D7      35
#define CAM_PIN_D6      34
#define CAM_PIN_D5      39
#define CAM_PIN_D4      36
#define CAM_PIN_D3      21
#define CAM_PIN_D2      19
#define CAM_PIN_D1      18
#define CAM_PIN_D0       5
#define CAM_PIN_VSYNC   25
#define CAM_PIN_HREF    23
#define CAM_PIN_PCLK    22

static WiFiServer _streamSrv(81);

static inline void ledOn()  { digitalWrite(PIN_CAM_LED, HIGH); }
static inline void ledOff() { digitalWrite(PIN_CAM_LED, LOW);  }

// Read HTTP request line, drain remaining headers
static String readRequestLine(WiFiClient& client) {
    String first = "";
    unsigned long deadline = millis() + 2000;
    while (client.connected() && millis() < deadline) {
        if (!client.available()) { delay(1); continue; }
        String line = client.readStringUntil('\n');
        if (first.isEmpty() && line.length() > 4) first = line;
        if (line == "\r" || line.length() <= 1) break;
    }
    return first;
}

// CORS + keep-alive headers helper
static void sendCORSHeader(WiFiClient& client) {
    client.print("Access-Control-Allow-Origin: *\r\n");
}

// ---- MJPEG stream ----
static void handleStream(WiFiClient& client) {
    ledOn();
    Serial.println("[Camera] Stream client connected — LED on");

    client.print(
        "HTTP/1.1 200 OK\r\n"
        "Content-Type: multipart/x-mixed-replace;boundary=frame\r\n"
        "Connection: keep-alive\r\n"
        "Access-Control-Allow-Origin: *\r\n\r\n"
    );

    while (client.connected()) {
        camera_fb_t* fb = esp_camera_fb_get();
        if (!fb) { delay(10); continue; }

        if (fb->format == PIXFORMAT_JPEG) {
            client.printf(
                "--frame\r\n"
                "Content-Type: image/jpeg\r\n"
                "Content-Length: %u\r\n\r\n",
                fb->len
            );
            client.write(fb->buf, fb->len);
            client.print("\r\n");
        }
        esp_camera_fb_return(fb);
        delay(50);  // ~20 fps cap
    }

    ledOff();
    Serial.println("[Camera] Stream client disconnected — LED off");
}

// ---- Single JPEG snapshot ----
static void handleSnapshot(WiFiClient& client) {
    camera_fb_t* fb = esp_camera_fb_get();
    if (!fb) {
        client.print("HTTP/1.1 503 Service Unavailable\r\nContent-Length: 0\r\n\r\n");
        return;
    }
    client.printf(
        "HTTP/1.1 200 OK\r\n"
        "Content-Type: image/jpeg\r\n"
        "Content-Length: %u\r\n"
        "Content-Disposition: inline; filename=snapshot.jpg\r\n"
        "Access-Control-Allow-Origin: *\r\n\r\n",
        fb->len
    );
    client.write(fb->buf, fb->len);
    esp_camera_fb_return(fb);
}

// ---- LED control: /led?state=on|off ----
static void handleLed(WiFiClient& client, const String& req) {
    if (req.indexOf("state=on")  >= 0) ledOn();
    if (req.indexOf("state=off") >= 0) ledOff();
    const char* state = digitalRead(PIN_CAM_LED) ? "on" : "off";
    client.printf(
        "HTTP/1.1 200 OK\r\n"
        "Content-Type: text/plain\r\n"
        "Content-Length: %u\r\n"
        "Access-Control-Allow-Origin: *\r\n\r\n"
        "%s",
        strlen(state), state
    );
}

// ---- FreeRTOS task ----
static void streamTask(void* arg) {
    _streamSrv.begin();
    Serial.println("[Camera] Stream server started on port 81");

    for (;;) {
        WiFiClient client = _streamSrv.available();
        if (client) {
            String req = readRequestLine(client);
            if      (req.indexOf("/stream")   >= 0) handleStream(client);
            else if (req.indexOf("/snapshot") >= 0) handleSnapshot(client);
            else if (req.indexOf("/led")      >= 0) handleLed(client, req);
            else client.print("HTTP/1.1 404 Not Found\r\nContent-Length: 0\r\n\r\n");
            client.stop();
        }
        delay(1);
    }
}

bool Camera_begin() {
    pinMode(PIN_CAM_LED, OUTPUT);
    ledOff();

    camera_config_t cfg;
    memset(&cfg, 0, sizeof(cfg));

    cfg.ledc_channel = LEDC_CHANNEL_0;
    cfg.ledc_timer   = LEDC_TIMER_0;
    cfg.pin_d0       = CAM_PIN_D0;
    cfg.pin_d1       = CAM_PIN_D1;
    cfg.pin_d2       = CAM_PIN_D2;
    cfg.pin_d3       = CAM_PIN_D3;
    cfg.pin_d4       = CAM_PIN_D4;
    cfg.pin_d5       = CAM_PIN_D5;
    cfg.pin_d6       = CAM_PIN_D6;
    cfg.pin_d7       = CAM_PIN_D7;
    cfg.pin_xclk     = CAM_PIN_XCLK;
    cfg.pin_pclk     = CAM_PIN_PCLK;
    cfg.pin_vsync    = CAM_PIN_VSYNC;
    cfg.pin_href     = CAM_PIN_HREF;
    cfg.pin_sccb_sda = CAM_PIN_SIOD;
    cfg.pin_sccb_scl = CAM_PIN_SIOC;
    cfg.pin_pwdn     = CAM_PIN_PWDN;
    cfg.pin_reset    = CAM_PIN_RESET;
    cfg.xclk_freq_hz = 20000000;
    cfg.pixel_format = PIXFORMAT_JPEG;

    if (psramFound()) {
        cfg.frame_size   = FRAMESIZE_VGA;
        cfg.jpeg_quality = 12;
        cfg.fb_count     = 2;
    } else {
        cfg.frame_size   = FRAMESIZE_QVGA;
        cfg.jpeg_quality = 15;
        cfg.fb_count     = 1;
    }

    esp_err_t err = esp_camera_init(&cfg);
    if (err != ESP_OK) {
        Serial.printf("[Camera] Init failed: 0x%x\n", err);
        return false;
    }
    Serial.println("[Camera] OV2640 initialized OK");
    return true;
}

void Camera_startStreamServer() {
    xTaskCreatePinnedToCore(streamTask, "camStream", 4096, nullptr, 1, nullptr, 0);
}
