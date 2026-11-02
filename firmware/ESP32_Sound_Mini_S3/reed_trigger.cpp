/*
 * reed_trigger.cpp  -  Soundmodul Mini
 *
 * Reed-Kontakt-Eingänge auf IO5 (sound2) und IO6 (sound1).
 * Nur aktiv wenn REED_TRIGGER_ENABLED in reed_trigger.h definiert ist.
 * Keinerlei Abhängigkeit von CRSF oder anderen Modulen.
 */

#include "reed_trigger.h"

#ifdef REED_TRIGGER_ENABLED

// ── Interner Zustand (je Kanal) ──────────────────────────────────────────
struct ReedChannel {
    uint8_t  pin;
    uint8_t  soundIdx;       // Index in Sound_on_web[] (1 oder 2)
    bool     lastState;      // letzter entprellter Pegel
    bool     rawLast;        // vorheriger Rohpegel (für Flankendetekt)
    uint32_t lastChangeMs;   // Zeitpunkt des letzten Pegelwechsels (Entprellung)
    uint32_t lastTriggerMs;  // Zeitpunkt des letzten Triggers (Retrigger-Schutz)
};

static ReedChannel channels[2] = {
    { REED_PIN_SOUND1, 1, HIGH, HIGH, 0, 0 },
    { REED_PIN_SOUND2, 2, HIGH, HIGH, 0, 0 },
};

// ── reedTriggerBegin() ───────────────────────────────────────────────────
void reedTriggerBegin() {
    for (auto& ch : channels) {
        pinMode(ch.pin, INPUT_PULLUP);
        ch.lastState    = HIGH;
        ch.rawLast      = HIGH;
        ch.lastChangeMs = millis();
        ch.lastTriggerMs = 0;
        Serial.printf("[Reed] IO%d -> sound%d.wav (INPUT_PULLUP)\n",
                      ch.pin, ch.soundIdx);
    }
}

// ── reedTriggerUpdate() ──────────────────────────────────────────────────
// Flankenbasierte Entprellung: ein Trigger wird ausgelöst, wenn der Pegel
// nach REED_DEBOUNCE_MS stabil LOW ist (fallende Flanke nach Entprellzeit).
void reedTriggerUpdate(bool* sound_on_web, uint8_t array_size) {
    uint32_t now = millis();

    for (auto& ch : channels) {
        if (ch.soundIdx >= array_size) continue;   // Sicherheitscheck

        bool raw = (bool)digitalRead(ch.pin);      // HIGH = offen, LOW = Kontakt

        // Flankendetekt: nur bei Pegelwechsel den Entprell-Timer neu starten
        if (raw != ch.rawLast) {
            ch.lastChangeMs = now;
            ch.rawLast = raw;
        }

        // Entprellzeit abgewartet?
        if ((now - ch.lastChangeMs) >= REED_DEBOUNCE_MS) {
            bool stable = raw;

            // Fallende Flanke (HIGH -> LOW) nach Entprellung = Reed-Impuls
            if (stable == LOW && ch.lastState == HIGH) {
                // Retrigger-Schutz: nicht schneller als REED_RETRIGGER_MS
                if ((now - ch.lastTriggerMs) >= REED_RETRIGGER_MS) {
                    sound_on_web[ch.soundIdx] = true;
                    ch.lastTriggerMs = now;
                    Serial.printf("[Reed] IO%d ausgeloest -> sound%d\n",
                                  ch.pin, ch.soundIdx);
                }
            }
            ch.lastState = stable;
        }
    }
}

#endif // REED_TRIGGER_ENABLED
