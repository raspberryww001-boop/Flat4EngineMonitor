#pragma once
#include <Arduino.h>

// Initialize OV2640 camera.
// Returns true on success, false if camera hardware not found.
bool Camera_begin();

// Start MJPEG stream server on port 81 in a dedicated FreeRTOS task.
// Endpoints:
//   GET http://<host>:81/stream   — continuous MJPEG stream
//   GET http://<host>:81/snapshot — single JPEG frame
void Camera_startStreamServer();
