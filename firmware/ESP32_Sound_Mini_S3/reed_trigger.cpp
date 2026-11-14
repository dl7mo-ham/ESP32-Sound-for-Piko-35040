/*
 * reed_trigger.cpp  -  Soundmodul Mini
 *
 * Reed-Kontakt-Eingaenge: beide reagieren nur auf einfache Impulse.
 *   IO6 -> sound1.wav
 *   IO5 -> sound2.wav
 *
 * Ein Trigger wird nur bei der schliessenden Flanke (HIGH -> LOW) nach
 * abgelaufener Entprellzeit ausgeloest.
 *
 * Nur aktiv wenn REED_TRIGGER_ENABLED in reed_trigger.h definiert ist.
 */

#include "reed_trigger.h"
#include "config.h"

#ifdef REED_TRIGGER_ENABLED

// Aktive Entprellzeit aus der Konfiguration (Web-UI). Bei ungueltigen
// Werten gilt die Werkseinstellung.
static uint32_t reedDebounceMs() {
    return config.ReedDebounceMs >= 5
        ? (uint32_t)config.ReedDebounceMs
        : REED_DEBOUNCE_DEFAULT_MS;
}

// ── Interner Zustand eines Reed-Kanals ───────────────────────────────────
struct ReedChannel {
    uint8_t  pin;
    uint8_t  soundSlot;    // Index in Sound_on_web[] (1 = sound1.wav, 2 = sound2.wav)
    bool     stableState;
    bool     rawLast;
    uint32_t lastChangeMs;
};

static ReedChannel channels[] = {
    { REED_PIN_SOUND1, 1, HIGH, HIGH, 0 },
    { REED_PIN_SOUND2, 2, HIGH, HIGH, 0 },
};
static constexpr uint8_t REED_CHANNEL_COUNT = sizeof(channels) / sizeof(channels[0]);

// ── reedTriggerBegin() ───────────────────────────────────────────────────
void reedTriggerBegin() {
    for (uint8_t i = 0; i < REED_CHANNEL_COUNT; i++) {
        ReedChannel& ch = channels[i];
        pinMode(ch.pin, INPUT_PULLUP);
        ch.stableState   = HIGH;
        ch.rawLast       = HIGH;
        ch.lastChangeMs  = millis();
        Serial.printf("[Reed] IO%d -> sound%d.wav (INPUT_PULLUP, Entprellung %u ms)\n",
                      ch.pin, (unsigned)ch.soundSlot, (unsigned)reedDebounceMs());
    }
}

// ── reedTriggerUpdate() ──────────────────────────────────────────────────
void reedTriggerUpdate(bool* sound_on_web, uint8_t array_size) {
    uint32_t now = millis();
    const uint32_t debounceMs = reedDebounceMs();

    for (uint8_t i = 0; i < REED_CHANNEL_COUNT; i++) {
        ReedChannel& ch = channels[i];
        if (ch.soundSlot >= array_size) continue;   // Array zu klein -> Kanal aus

        bool raw = (bool)digitalRead(ch.pin);
        if (raw != ch.rawLast) {
            ch.lastChangeMs = now;
            ch.rawLast = raw;
        }

        // Erst nach der Entprellzeit gilt der Pegel als stabil.
        if ((now - ch.lastChangeMs) < debounceMs) continue;
        if (raw == ch.stableState) continue;

        ch.stableState = raw;

        // Flanke HIGH -> LOW (Kontakt schliesst): Impuls auswerten.
        // Kein Loslass-Ereignis noetig, dadurch spielt der Sound schon
        // beim Schliessen und nicht erst beim Oeffnen des Kontakts.
        if (ch.stableState == LOW) {
            sound_on_web[ch.soundSlot] = true;
            Serial.printf("[Reed] IO%d Impuls -> sound%d.wav\n",
                          ch.pin, (unsigned)ch.soundSlot);
        }
    }
}

#endif // REED_TRIGGER_ENABLED
