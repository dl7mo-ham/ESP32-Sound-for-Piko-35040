/*
 * reed_trigger.cpp  -  Soundmodul Mini
 *
 * Reed-Kontakt-Eingänge:
 *   IO6: Motorsound (start/loop/shut) an- bzw. ausschalten (Toggle per Impuls)
 *   IO5: Zusatzsound:
 *          - Kurzer Impuls (< 800 ms)   -> sound1.wav
 *          - Langer Impuls (>= 800 ms)  -> sound2.wav
 *
 * Nur aktiv wenn REED_TRIGGER_ENABLED in reed_trigger.h definiert ist.
 */

#include "reed_trigger.h"

#ifdef REED_TRIGGER_ENABLED

// ── Interner Zustand IO6 (Motorsound Toggle) ──────────────────────────────
struct ReedMotorChannel {
    uint8_t  pin;
    bool     stableState;
    bool     rawLast;
    uint32_t lastChangeMs;
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

static ReedMotorChannel  motorCh  = { REED_PIN_MOTOR,  HIGH, HIGH, 0, 0 };
static ReedSoundsChannel soundsCh = { REED_PIN_SOUNDS, HIGH, HIGH, 0, 0, false, false, 0 };

// ── reedTriggerBegin() ───────────────────────────────────────────────────
void reedTriggerBegin() {
    pinMode(motorCh.pin, INPUT_PULLUP);
    motorCh.stableState   = HIGH;
    motorCh.rawLast       = HIGH;
    motorCh.lastChangeMs  = millis();
    motorCh.lastTriggerMs = 0;

    pinMode(soundsCh.pin, INPUT_PULLUP);
    soundsCh.stableState   = HIGH;
    soundsCh.rawLast       = HIGH;
    soundsCh.lastChangeMs  = millis();
    soundsCh.pressStartMs  = 0;
    soundsCh.isPressed     = false;
    soundsCh.longTriggered = false;
    soundsCh.lastTriggerMs = 0;

    Serial.printf("[Reed] IO%d -> Motorsound Toggle (INPUT_PULLUP)\n", motorCh.pin);
    Serial.printf("[Reed] IO%d -> Kurz=sound1 / Lang(>=%u ms)=sound2 (INPUT_PULLUP)\n",
                  soundsCh.pin, (unsigned)REED_LONG_PRESS_MS);
}

// ── reedTriggerUpdate() ──────────────────────────────────────────────────
void reedTriggerUpdate(bool* sound_on_web, uint8_t array_size, bool& sound_on_motor) {
    uint32_t now = millis();

    // ── 1. IO6: Motorsound Toggle ─────────────────────────────────────────
    bool rawM = (bool)digitalRead(motorCh.pin);
    if (rawM != motorCh.rawLast) {
        motorCh.lastChangeMs = now;
        motorCh.rawLast = rawM;
    }

    if ((now - motorCh.lastChangeMs) >= REED_DEBOUNCE_MS) {
        if (rawM != motorCh.stableState) {
            motorCh.stableState = rawM;
            // Fallende Flanke (HIGH -> LOW): Reed-Kontakt geschlossen / Impuls
            if (motorCh.stableState == LOW) {
                if ((now - motorCh.lastTriggerMs) >= REED_RETRIGGER_MS) {
                    sound_on_motor = !sound_on_motor;
                    motorCh.lastTriggerMs = now;
                    Serial.printf("[Reed] IO%d Impuls -> Motorsound %s\n",
                                  motorCh.pin, sound_on_motor ? "EIN (Start->Loop)" : "AUS (Shut)");
                }
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
