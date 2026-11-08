/*
 * reed_trigger.cpp  -  Soundmodul Mini
 *
 * Reed-Kontakt-Eingänge:
 *   IO6: - Kurzer Impuls (< 800 ms)   -> Motorsound (start/loop/shut) togglen
 *        - Langer Impuls (>= 800 ms)  -> Flammen-LED an- bzw. ausschalten
 *   IO5: Zusatzsound:
 *          - Kurzer Impuls (< 800 ms)   -> sound1.wav
 *          - Langer Impuls (>= 800 ms)  -> sound2.wav
 *
 * Nur aktiv wenn REED_TRIGGER_ENABLED in reed_trigger.h definiert ist.
 */

#include "reed_trigger.h"

#ifdef REED_TRIGGER_ENABLED

// ── Interner Zustand IO6 (kurz: Motor Toggle / lang: LED Toggle) ──────────
struct ReedMotorChannel {
    uint8_t  pin;
    bool     stableState;
    bool     rawLast;
    uint32_t lastChangeMs;
    uint32_t pressStartMs;
    bool     isPressed;
    bool     longTriggered;
    uint32_t lastTriggerMs;
};

// ── Interner Zustand IO5 (Sound 1 kurz / Sound 2 lang) ────────────────────
struct ReedSoundsChannel {
    uint8_t  pin;
    bool     stableState;
    bool     rawLast;
    uint32_t lastChangeMs;
    uint32_t pressStartMs;
    bool     isPressed;
    bool     longTriggered;
    uint32_t lastTriggerMs;
};

static ReedMotorChannel  motorCh  = { REED_PIN_MOTOR,  HIGH, HIGH, 0, 0, false, false, 0 };
static ReedSoundsChannel soundsCh = { REED_PIN_SOUNDS, HIGH, HIGH, 0, 0, false, false, 0 };

// LED-Zustand im Reed-Modus (wird per langem IO6-Impuls getoggelt).
// Statt statischem Gruen wird eine nicht-blockierende Flamme gerendert.
static bool     ledOn       = false;
static uint8_t  flameLevel  = 0;      // zuletzt geschriebene Helligkeit, 0 = aus
static uint32_t flameLastMs = 0;
static uint32_t flameRng    = 0x1D2C3B4Au;

static void updateFlame(uint32_t now) {
    if (!ledOn) {
        if (flameLevel) {
            neopixelWrite(REED_PIN_LED, 0, 0, 0);
            flameLevel = 0;
        }
        return;
    }

    if (flameLastMs && (now - flameLastMs) < REED_FLAME_INTERVAL_MS) return;
    flameLastMs = now;

    // Billiger xorshift-PRNG, kein random()/Seed noetig.
    flameRng ^= flameRng << 13;
    flameRng ^= flameRng >> 17;
    flameRng ^= flameRng << 5;

    uint8_t target = REED_FLAME_MIN_BRIGHTNESS +
        (uint16_t)(uint8_t)flameRng *
        (REED_LED_BRIGHTNESS - REED_FLAME_MIN_BRIGHTNESS) / 255;

    // Traegheit: 2/3 alt + 1/3 neu -> organisches Flackern statt Stroboskop.
    flameLevel = (uint8_t)(((uint16_t)flameLevel * 2u + target) / 3u);
    if (flameLevel < REED_FLAME_MIN_BRIGHTNESS)
        flameLevel = REED_FLAME_MIN_BRIGHTNESS;

    uint8_t g = (uint16_t)flameLevel * REED_FLAME_GREEN_PCT / 100;
    neopixelWrite(REED_PIN_LED, flameLevel, g, 0);   // warmes Orange
}

// ── reedTriggerBegin() ───────────────────────────────────────────────────
void reedTriggerBegin() {
    pinMode(motorCh.pin, INPUT_PULLUP);
    motorCh.stableState   = HIGH;
    motorCh.rawLast       = HIGH;
    motorCh.lastChangeMs  = millis();
    motorCh.pressStartMs  = 0;
    motorCh.isPressed     = false;
    motorCh.longTriggered = false;
    motorCh.lastTriggerMs = 0;

    ledOn       = false;
    flameLevel  = 0;
    flameLastMs = 0;
    neopixelWrite(REED_PIN_LED, 0, 0, 0);

    pinMode(soundsCh.pin, INPUT_PULLUP);
    soundsCh.stableState   = HIGH;
    soundsCh.rawLast       = HIGH;
    soundsCh.lastChangeMs  = millis();
    soundsCh.pressStartMs  = 0;
    soundsCh.isPressed     = false;
    soundsCh.longTriggered = false;
    soundsCh.lastTriggerMs = 0;

    Serial.printf("[Reed] IO%d -> Kurz=Motor Toggle / Lang(>=%u ms)=LED Toggle (INPUT_PULLUP)\n",
                  motorCh.pin, (unsigned)REED_LONG_PRESS_MS);
    Serial.printf("[Reed] IO%d -> Kurz=sound1 / Lang(>=%u ms)=sound2 (INPUT_PULLUP)\n",
                  soundsCh.pin, (unsigned)REED_LONG_PRESS_MS);
}

// ── reedTriggerUpdate() ──────────────────────────────────────────────────
void reedTriggerUpdate(bool* sound_on_web, uint8_t array_size, bool& sound_on_motor) {
    uint32_t now = millis();

    // Flammen-Animation der Status-LED (laeuft unabhaengig von den Eingaengen)
    updateFlame(now);

    // ── 1. IO6: kurz = Motorsound Toggle, lang = LED Toggle ───────────────
    bool rawM = (bool)digitalRead(motorCh.pin);
    if (rawM != motorCh.rawLast) {
        motorCh.lastChangeMs = now;
        motorCh.rawLast = rawM;
    }

    if ((now - motorCh.lastChangeMs) >= REED_DEBOUNCE_MS) {
        if (rawM != motorCh.stableState) {
            motorCh.stableState = rawM;
            if (motorCh.stableState == LOW) {
                // Flanke HIGH -> LOW (Kontakt schliesst): Impuls beginnt
                motorCh.isPressed     = true;
                motorCh.pressStartMs  = motorCh.lastChangeMs;
                motorCh.longTriggered = false;
            } else {
                // Flanke LOW -> HIGH (Kontakt oeffnet wieder)
                if (motorCh.isPressed) {
                    motorCh.isPressed = false;
                    if (!motorCh.longTriggered) {
                        // Losgelassen vor Ablauf von REED_LONG_PRESS_MS -> kurzer Puls!
                        uint32_t pressDuration = motorCh.lastChangeMs - motorCh.pressStartMs;
                        if ((now - motorCh.lastTriggerMs) >= REED_RETRIGGER_MS) {
                            sound_on_motor = !sound_on_motor;
                            motorCh.lastTriggerMs = now;
                            Serial.printf("[Reed] IO%d kurzer Puls (%u ms) -> Motorsound %s\n",
                                          motorCh.pin, (unsigned)pressDuration,
                                          sound_on_motor ? "EIN (Start->Loop)" : "AUS (Shut)");
                        }
                    }
                }
            }
        }
    }

    // Waehrend der Kontakt gehalten wird: langer Puls -> LED togglen
    if (motorCh.isPressed && !motorCh.longTriggered) {
        if ((now - motorCh.pressStartMs) >= REED_LONG_PRESS_MS) {
            if ((now - motorCh.lastTriggerMs) >= REED_RETRIGGER_MS) {
                ledOn = !ledOn;
                motorCh.lastTriggerMs = now;
                motorCh.longTriggered = true;
                Serial.printf("[Reed] IO%d langer Puls (>= %u ms) -> LED %s\n",
                              motorCh.pin, (unsigned)REED_LONG_PRESS_MS,
                              ledOn ? "EIN" : "AUS");
            }
        }
    }

    // ── 2. IO5: sound1 (kurzer Puls) bzw. sound2 (langer Puls >= 800 ms) ──
    if (array_size >= 3) {
        bool rawS = (bool)digitalRead(soundsCh.pin);
        if (rawS != soundsCh.rawLast) {
            soundsCh.lastChangeMs = now;
            soundsCh.rawLast = rawS;
        }

        if ((now - soundsCh.lastChangeMs) >= REED_DEBOUNCE_MS) {
            if (rawS != soundsCh.stableState) {
                soundsCh.stableState = rawS;
                if (soundsCh.stableState == LOW) {
                    // Flanke HIGH -> LOW (Kontakt schließt)
                    soundsCh.isPressed = true;
                    soundsCh.pressStartMs = soundsCh.lastChangeMs;
                    soundsCh.longTriggered = false;
                } else {
                    // Flanke LOW -> HIGH (Kontakt öffnet wieder)
                    if (soundsCh.isPressed) {
                        soundsCh.isPressed = false;
                        if (!soundsCh.longTriggered) {
                            // Losgelassen vor Ablauf von REED_LONG_PRESS_MS -> Kurzer Puls!
                            uint32_t pressDuration = soundsCh.lastChangeMs - soundsCh.pressStartMs;
                            if ((now - soundsCh.lastTriggerMs) >= REED_RETRIGGER_MS) {
                                sound_on_web[1] = true;
                                soundsCh.lastTriggerMs = now;
                                Serial.printf("[Reed] IO%d kurzer Puls (%u ms) -> sound1.wav\n",
                                              soundsCh.pin, (unsigned)pressDuration);
                            }
                        }
                    }
                }
            }
        }

        // Während Kontakt gehalten wird: Prüfung auf langen Puls (>= 800 ms)
        if (soundsCh.isPressed && !soundsCh.longTriggered) {
            if ((now - soundsCh.pressStartMs) >= REED_LONG_PRESS_MS) {
                if ((now - soundsCh.lastTriggerMs) >= REED_RETRIGGER_MS) {
                    sound_on_web[2] = true;
                    soundsCh.lastTriggerMs = now;
                    soundsCh.longTriggered = true;
                    Serial.printf("[Reed] IO%d langer Puls (>= %u ms) -> sound2.wav\n",
                                  soundsCh.pin, (unsigned)REED_LONG_PRESS_MS);
                }
            }
        }
    }
}

#endif // REED_TRIGGER_ENABLED
