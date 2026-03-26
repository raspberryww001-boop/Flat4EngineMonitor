#include "camera.h"
#include "config.h"
#include <esp_camera.h>
#include <WebServer.h>

// ---- AI-Thinker ESP32-CAM pin map ----
// (Freenove ESP32-CAM uses this same layout)
#define CAM_PIN_PWDN    32
#define CAM_PIN_RESET   -1   // not connected
#define CAM_PIN_XCLK     0
#define CAM_PIN_SIOD    26   // SDA
#define CAM_PIN_SIOC    27   // SCL
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

static WebServer _streamSrv(81);

static inline void ledOn()  { digitalWrite(PIN_CAM_LED, HIGH); }
static inline void ledOff() { digitalWrite(PIN_CAM_LED, LOW);  }

// ---- MJPEG multipart stream ----
// LED turns on when a viewer connects, off when they disconnect.
static void handleStream() {
    ledOn();
    Serial.println("[Camera] Stream client connected — LED on");

    WiFiClient client = _streamSrv.client();
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
static void handleSnapshot() {
    camera_fb_t* fb = esp_camera_fb_get();
    if (!fb) {
        _streamSrv.send(503, "text/plain", "Capture failed");
        return;
    }
    _streamSrv.sendHeader("Content-Disposition", "inline; filename=snapshot.jpg");
    _streamSrv.sendHeader("Access-Control-Allow-Origin", "*");
    _streamSrv.send_P(200, "image/jpeg", (const char*)fb->buf, fb->len);
    esp_camera_fb_return(fb);
}

// ---- Manual LED control: GET /led?state=on  or  /led?state=off ----
static void handleLed() {
    _streamSrv.sendHeader("Access-Control-Allow-Origin", "*");
    if (_streamSrv.hasArg("state")) {
        String s = _streamSrv.arg("state");
        if (s == "on")  { ledOn();  _streamSrv.send(200, "text/plain", "on");  return; }
        if (s == "off") { ledOff(); _streamSrv.send(200, "text/plain", "off"); return; }
    }
    // No arg: return current state
    _streamSrv.send(200, "text/plain",
        digitalRead(PIN_CAM_LED) ? "on" : "off");
}

// ---- FreeRTOS task: runs stream server on core 0 ----
static void streamTask(void* arg) {
    _streamSrv.on("/stream",   handleStream);
    _streamSrv.on("/snapshot", handleSnapshot);
    _streamSrv.on("/led",      handleLed);
    _streamSrv.begin();
    Serial.println("[Camera] Stream server started on port 81");
    for (;;) {
        _streamSrv.handleClient();
        delay(1);
    }
}

bool Camera_begin() {
    // LED pin setup
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
        cfg.frame_size   = FRAMESIZE_VGA;   // 640×480
        cfg.jpeg_quality = 12;              // 0–63 (lower = better quality)
        cfg.fb_count     = 2;              // double-buffer for smooth streaming
    } else {
        cfg.frame_size   = FRAMESIZE_QVGA;  // 320×240
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
    // Stack 4 KB, priority 1, pinned to core 0
    // (main loop runs on core 1, so no CPU contention)
    xTaskCreatePinnedToCore(streamTask, "camStream", 4096, nullptr, 1, nullptr, 0);
}
