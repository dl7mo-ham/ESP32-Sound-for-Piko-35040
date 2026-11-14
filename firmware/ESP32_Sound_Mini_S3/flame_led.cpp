/*
 * flame_led.cpp  -  Soundmodul Mini
 *
 * Nicht blockierender Flammen-Effekt der Status-LED (WS2812, warmes Orange).
 * Der Zustand wird per Web-UI (und spaeter IO7/IO8) geschaltet, siehe
 * flame_led.h.
 */

#include "flame_led.h"
#include "pins.h"

// Maximalhelligkeit und Flacker-Parameter.
static constexpr uint8_t  FLAME_BRIGHTNESS       = 40;
static constexpr uint32_t FLAME_INTERVAL_MS      = 33;  // ~30 fps
static constexpr uint8_t  FLAME_MIN_BRIGHTNESS   = 10;
static constexpr uint8_t  FLAME_GREEN_PCT        = 25;  // 0=rot, 100=gruen -> 25 = warmes Orange

static bool     ledOn       = false;
static uint8_t  flameLevel  = 0;      // zuletzt geschriebene Helligkeit, 0 = aus
static uint32_t flameLastMs = 0;
static uint32_t flameRng    = 0x1D2C3B4Au;

static void renderFlame(uint32_t now) {
    if (!ledOn) {
        if (flameLevel) {
            neopixelWrite(PIN_STATUS_LED, 0, 0, 0);
            flameLevel = 0;
        }
        return;
    }

    if (flameLastMs && (now - flameLastMs) < FLAME_INTERVAL_MS) return;
    flameLastMs = now;

    // Billiger xorshift-PRNG, kein random()/Seed noetig.
    flameRng ^= flameRng << 13;
    flameRng ^= flameRng >> 17;
    flameRng ^= flameRng << 5;

    uint8_t target = FLAME_MIN_BRIGHTNESS +
        (uint16_t)(uint8_t)flameRng *
        (FLAME_BRIGHTNESS - FLAME_MIN_BRIGHTNESS) / 255;

    // Traegheit: 2/3 alt + 1/3 neu -> organisches Flackern statt Stroboskop.
    flameLevel = (uint8_t)(((uint16_t)flameLevel * 2u + target) / 3u);
    if (flameLevel < FLAME_MIN_BRIGHTNESS)
        flameLevel = FLAME_MIN_BRIGHTNESS;

    uint8_t g = (uint16_t)flameLevel * FLAME_GREEN_PCT / 100;
    neopixelWrite(PIN_STATUS_LED, flameLevel, g, 0);   // warmes Orange
}

void flameBegin() {
    ledOn       = false;
    flameLevel  = 0;
    flameLastMs = 0;
    neopixelWrite(PIN_STATUS_LED, 0, 0, 0);
}

void flameUpdate() {
    renderFlame(millis());
}

void flameSet(bool on) {
    ledOn = on;
    if (!on) {
        // Sofort dunkel, nicht erst beim naechsten flameUpdate().
        neopixelWrite(PIN_STATUS_LED, 0, 0, 0);
        flameLevel = 0;
    }
    Serial.printf("[LED] Flamme %s\n", on ? "EIN" : "AUS");
}

void flameToggle() {
    flameSet(!ledOn);
}

bool flameIsOn() {
    return ledOn;
}
