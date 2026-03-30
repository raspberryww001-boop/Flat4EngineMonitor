#include "ignition.h"

volatile uint64_t Ignition::_lastTime[4]  = {0, 0, 0, 0};
volatile uint64_t Ignition::_cyl1Period   = 0;
volatile uint64_t Ignition::_cyl1Ref      = 0;
volatile bool     Ignition::_fired[4]     = {false, false, false, false};

void Ignition::begin() {
    // Ignition signals are active-LOW pulses after TTL conversion (falling = spark)
    // Use RISING to detect the leading edge of the TTL pulse
    pinMode(PIN_IGN_CYL1, INPUT);
    pinMode(PIN_IGN_CYL2, INPUT);
    pinMode(PIN_IGN_CYL3, INPUT);
    pinMode(PIN_IGN_CYL4, INPUT);

    attachInterrupt(digitalPinToInterrupt(PIN_IGN_CYL1), isr_cyl1, RISING);
    attachInterrupt(digitalPinToInterrupt(PIN_IGN_CYL2), isr_cyl2, RISING);
    attachInterrupt(digitalPinToInterrupt(PIN_IGN_CYL3), isr_cyl3, RISING);
    attachInterrupt(digitalPinToInterrupt(PIN_IGN_CYL4), isr_cyl4, RISING);
}

void IRAM_ATTR Ignition::isr_cyl1() {
    uint64_t now = esp_timer_get_time(); // microseconds
    if (_lastTime[0] != 0) {
        if ((now - _lastTime[0]) < IGN_DEBOUNCE_US) return; // debounce
        _cyl1Period = now - _lastTime[0];
    }
    _lastTime[0] = now;
    _cyl1Ref     = now;
    _fired[0]    = true;
}

void IRAM_ATTR Ignition::isr_cyl2() {
    uint64_t now = esp_timer_get_time();
    if (_lastTime[1] != 0 && (now - _lastTime[1]) < IGN_DEBOUNCE_US) return;
    _lastTime[1] = now;
    _fired[1]    = true;
}

void IRAM_ATTR Ignition::isr_cyl3() {
    uint64_t now = esp_timer_get_time();
    if (_lastTime[2] != 0 && (now - _lastTime[2]) < IGN_DEBOUNCE_US) return;
    _lastTime[2] = now;
    _fired[2]    = true;
}

void IRAM_ATTR Ignition::isr_cyl4() {
    uint64_t now = esp_timer_get_time();
    if (_lastTime[3] != 0 && (now - _lastTime[3]) < IGN_DEBOUNCE_US) return;
    _lastTime[3] = now;
    _fired[3]    = true;
}

IgnitionData Ignition::getData() {
    // Snapshot with interrupts briefly disabled for consistency
    noInterrupts();
    uint64_t lastTime[4];
    uint64_t cyl1Period = _cyl1Period;
    uint64_t cyl1Ref    = _cyl1Ref;
    bool     fired[4];
    for (int i = 0; i < 4; i++) {
        lastTime[i] = _lastTime[i];
        fired[i]    = _fired[i];
        _fired[i]   = false; // clear
    }
    interrupts();

    IgnitionData d;
    uint64_t now = esp_timer_get_time();

    // Engine running check: cyl1 must have fired within timeout
    bool running = (cyl1Ref > 0) &&
                   ((now - cyl1Ref) < (uint64_t)RPM_TIMEOUT_MS * 1000ULL);
    d.engineRunning = running;

    // RPM from cyl1 period
    // Period is ONE full crank revolution (720°) covered in 2 ignition cycles for 4-cyl
    // Actually: cyl1Period = time between consecutive cyl1 sparks = 720° of crank
    if (running && cyl1Period > 0) {
        // cyl1 fires once per 2 crank revolutions (4-stroke)
        // Period in µs -> RPM = 60e6 / period (since period = 1 cycle = 2 rev for 4-stroke)
        // For 4-stroke: period between consecutive sparks for same cylinder = 2 crank revs
        // RPM (crank) = 60s / (period_us / 1e6) / 2  ... wait:
        // RPM = revolutions per minute.  period_us is time for 2 crank revolutions.
        // So RPM = (60 * 1e6) / period_us * ... no:
        //   2 crank revs per spark cycle for same cyl
        //   period_us = time for 2 crank revs
        //   crank RPM = (60e6 / period_us) * 2   <- multiply by 2 not divide
        // Simplification: RPM = 120,000,000 / period_us
        d.rpm = 120000000.0f / (float)cyl1Period;
    } else {
        d.rpm = 0.0f;
    }

    // Timing offsets: degrees relative to cyl1
    // cyl1 = 0° by definition
    d.timingDeg[0] = 0.0f;
    d.firing[0]    = fired[0];
    // Misfire: cylinder silent for >1.5x the expected firing period
    for (int i = 0; i < 4; i++) {
        d.misfire[i] = running && cyl1Period > 0 && lastTime[i] > 0 &&
                       (now - lastTime[i]) > (cyl1Period * 3 / 2);
    }

    for (int i = 1; i < 4; i++) {
        d.firing[i] = fired[i];
        if (running && cyl1Period > 0 && lastTime[i] > 0 && cyl1Ref > 0) {
            // Time since cyl1 last fired
            // We want the offset within the last cyl1 cycle
            int64_t dt_us = (int64_t)(lastTime[i] - cyl1Ref);
            // Wrap into [-period/2 .. +period/2] window
            int64_t period = (int64_t)cyl1Period;
            while (dt_us >  period / 2) dt_us -= period;
            while (dt_us < -period / 2) dt_us += period;
            // Convert to degrees (full cycle = 720°)
            float deg = ((float)dt_us / (float)cyl1Period) * 720.0f;
            // Clamp to ±360°
            if (deg >  360.0f) deg -= 720.0f;
            if (deg < -360.0f) deg += 720.0f;
            d.timingDeg[i] = deg;
        } else {
            d.timingDeg[i] = (float)i * FIRE_INTERVAL_DEG; // default
        }
    }

    return d;
}
