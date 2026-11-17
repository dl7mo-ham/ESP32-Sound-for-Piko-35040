/*
 * flame_led.cpp  -  Soundmodul Mini
 *
 * Flammen-Effekt der Status-LED (WS2812, warmes Orange), nicht blockierend.
 * Nutzt StatusLed::writeRgb(), damit Statusanzeige und Flamme dieselbe
 * LED-Instanz verwenden (kein zweiter Treiber, keine veralteten Farben).
 */

#include "flame_led.h"
#include "src/RC_Common/status_led.h"

static constexpr uint8_t  FLAME_BRIGHTNESS       = 40;
static constexpr uint32_t FLAME_INTERVAL_MS      = 33;  // ~30 fps
static constexpr uint8_t  FLAME_MIN_BRIGHTNESS   = 10;
static constexpr uint8_t  FLAME_GREEN_PCT        = 25;  // 0=rot, 100=gruen -> 25 = warmes Orange

static StatusLed* led = nullptr;
static bool     ledOn       = false;
static uint8_t  flameLevel  = 0;
static uint32_t flameLastMs = 0;
static uint32_t flameRng    = 0x1D2C3B4Au;

static void renderFlame(uint32_t now) {
    if (!led) return;
    if (!ledOn) {
        if (flameLevel) {
            led->writeRgb(0, 0, 0);
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
        (uint16_t)(uint8_t)flameRng * (FLAME_BRIGHTNESS - FLAME_MIN_BRIGHTNESS) / 255;

    // Traegheit: 2/3 alt + 1/3 neu -> organisches Flackern statt Stroboskop.
    flameLevel = (uint8_t)(((uint16_t)flameLevel * 2u + target) / 3u);
    if (flameLevel < FLAME_MIN_BRIGHTNESS) flameLevel = FLAME_MIN_BRIGHTNESS;

    uint8_t g = (uint16_t)flameLevel * FLAME_GREEN_PCT / 100;
    led->writeRgb(flameLevel, g, 0);   // warmes Orange
}

void flameBegin(StatusLed& statusLed) {
    led         = &statusLed;
    ledOn       = false;
    flameLevel  = 0;
    flameLastMs = 0;
}

void flameUpdate() {
    renderFlame(millis());
}

void flameSet(bool on) {
    if (on == ledOn) return;
    ledOn = on;
    if (!on) {
        // Sofort dunkel, nicht erst beim naechsten flameUpdate(); im Loop
        // uebernimmt gleich wieder die Status-LED.
        if (led) led->writeRgb(0, 0, 0);
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
