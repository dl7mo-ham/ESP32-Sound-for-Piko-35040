/*
 * reed_trigger.cpp  -  Soundmodul Mini
 *
 * Reed-Kontakt-Eingänge (Piko 35040 sendet nur kurze Pulse):
 *   IO6: - kurzer Druck      -> sound1.wav
 *        - doppelter Druck   -> Motorsound (start/loop/shut) togglen
 *   IO5: - kurzer Druck      -> sound2.wav
 *        - doppelter Druck   -> Flammen-LED an- bzw. ausschalten
 *
 * Ein Einzeldruck wird erst nach Ablauf des Doppeldruck-Fensters
 * (config.ReedDoublePressMs) ausgelöst, damit ein unmittelbar folgender
 * zweiter Puls als Doppeldruck erkannt wird.
 *
 * Nur aktiv wenn REED_TRIGGER_ENABLED in reed_trigger.h definiert ist.
 */

#include "reed_trigger.h"
#include "config.h"

#ifdef REED_TRIGGER_ENABLED

// Aktives Doppeldruck-Fenster. Der Wert ist in der Web-UI einstellbar und
// wird ohne Neustart wirksam; bei ungueltigem/fehlendem Wert gilt die
// Werkseinstellung.
static uint32_t reedDoublePressMs() {
    return config.ReedDoublePressMs >= 100
        ? (uint32_t)config.ReedDoublePressMs
        : REED_DOUBLE_PRESS_DEFAULT_MS;
}

// ── Ereignis eines Reed-Kanals ────────────────────────────────────────────
enum ReedEvent : uint8_t {
    REED_EVENT_NONE,
    REED_EVENT_SINGLE,
    REED_EVENT_DOUBLE
};

// ── Interner Zustand eines Reed-Kanals ────────────────────────────────────
struct ReedChannel {
    uint8_t  pin;
    bool     stableState;     // entprellter Pegel
    bool     rawLast;         // letzter Rohpegel
    uint32_t lastChangeMs;    // letzte Rohpegel-Aenderung (Entprellung)
    bool     isPressed;       // Kontakt aktuell geschlossen
    bool     pressConsumed;   // aktueller Druck bereits als Doppeldruck verarbeitet
    uint8_t  pressCount;      // 0 = nichts offen, 1 = wartet auf Doppeldruck
    uint32_t firstPressMs;    // Loslassen des ersten Drucks im Fenster
};

static ReedChannel io6Ch = { REED_PIN_IO6, HIGH, HIGH, 0, false, false, 0, 0 };
static ReedChannel io5Ch = { REED_PIN_IO5, HIGH, HIGH, 0, false, false, 0, 0 };

// LED-Zustand im Reed-Modus (wird per doppeltem IO5-Druck getoggelt).
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

// ── Kanal entprellen und Einzel-/Doppeldruck erkennen ─────────────────────
static ReedEvent updateChannel(ReedChannel& ch, uint32_t now) {
    const uint32_t doublePressMs = reedDoublePressMs();
    bool raw = (bool)digitalRead(ch.pin);
    if (raw != ch.rawLast) {
        ch.lastChangeMs = now;
        ch.rawLast = raw;
    }

    ReedEvent event = REED_EVENT_NONE;

    // Entprellte Flankenerkennung. Der Doppeldruck wird an der Flanke des
    // zweiten Drucks erkannt; die Pulsdauer selbst spielt keine Rolle.
    if ((now - ch.lastChangeMs) >= REED_DEBOUNCE_MS) {
        if (raw != ch.stableState) {
            ch.stableState = raw;
            if (ch.stableState == LOW) {
                // Kontakt schliesst.
                ch.isPressed = true;
                ch.pressConsumed = false;
                if (ch.pressCount == 1) {
                    if ((now - ch.firstPressMs) <= doublePressMs) {
                        // Zweiter Druck innerhalb des Fensters -> Doppeldruck.
                        ch.pressCount = 0;
                        ch.pressConsumed = true;
                        event = REED_EVENT_DOUBLE;
                    } else {
                        // Fenster war abgelaufen, der Einzeldruck wurde noch
                        // nicht ausgeloest: jetzt nachholen und diesen Druck
                        // als neuen Einzeldruck vormerken.
                        ch.firstPressMs = now;
                        event = REED_EVENT_SINGLE;
                    }
                }
            } else {
                // Kontakt oeffnet wieder.
                ch.isPressed = false;
                if (ch.pressConsumed) {
                    ch.pressConsumed = false;   // Doppeldruck war schon erledigt
                } else if (ch.pressCount == 0) {
                    // Erster Druck: Fenster fuer einen moeglichen zweiten startet.
                    ch.pressCount  = 1;
                    ch.firstPressMs = now;
                }
            }
        }
    }

    // Kein zweiter Druck innerhalb des Fensters -> Einzeldruck ausloesen.
    if (event == REED_EVENT_NONE && ch.pressCount == 1 && !ch.isPressed &&
        (now - ch.firstPressMs) >= doublePressMs) {
        ch.pressCount = 0;
        event = REED_EVENT_SINGLE;
    }

    return event;
}

static void channelBegin(ReedChannel& ch) {
    pinMode(ch.pin, INPUT_PULLUP);
    ch.stableState   = HIGH;
    ch.rawLast       = HIGH;
    ch.lastChangeMs  = millis();
    ch.isPressed     = false;
    ch.pressConsumed = false;
    ch.pressCount    = 0;
    ch.firstPressMs  = 0;
}

// ── reedTriggerBegin() ───────────────────────────────────────────────────
void reedTriggerBegin() {
    channelBegin(io6Ch);
    channelBegin(io5Ch);

    ledOn       = false;
    flameLevel  = 0;
    flameLastMs = 0;
    neopixelWrite(REED_PIN_LED, 0, 0, 0);

    Serial.printf("[Reed] IO%d -> kurz=sound1 / doppelt=Motorsound (INPUT_PULLUP)\n",
                  io6Ch.pin);
    Serial.printf("[Reed] IO%d -> kurz=sound2 / doppelt=LED (INPUT_PULLUP, Doppeldruck <= %u ms)\n",
                  io5Ch.pin, (unsigned)reedDoublePressMs());
}

// ── reedTriggerUpdate() ──────────────────────────────────────────────────
void reedTriggerUpdate(bool* sound_on_web, uint8_t array_size, bool& sound_on_motor) {
    uint32_t now = millis();

    // Flammen-Animation der Status-LED (laeuft unabhaengig von den Eingaengen)
    updateFlame(now);

    // ── 1. IO6: kurz = sound1, doppelt = Motorsound Toggle ────────────────
    switch (updateChannel(io6Ch, now)) {
        case REED_EVENT_SINGLE:
            if (array_size >= 2) {
                sound_on_web[1] = true;
                Serial.printf("[Reed] IO%d Einzeldruck -> sound1.wav\n", io6Ch.pin);
            }
            break;
        case REED_EVENT_DOUBLE:
            sound_on_motor = !sound_on_motor;
            Serial.printf("[Reed] IO%d Doppeldruck -> Motorsound %s\n",
                          io6Ch.pin,
                          sound_on_motor ? "EIN (Start->Loop)" : "AUS (Shut)");
            break;
        default:
            break;
    }

    // ── 2. IO5: kurz = sound2, doppelt = LED Toggle ───────────────────────
    switch (updateChannel(io5Ch, now)) {
        case REED_EVENT_SINGLE:
            if (array_size >= 3) {
                sound_on_web[2] = true;
                Serial.printf("[Reed] IO%d Einzeldruck -> sound2.wav\n", io5Ch.pin);
            }
            break;
        case REED_EVENT_DOUBLE:
            ledOn = !ledOn;
            Serial.printf("[Reed] IO%d Doppeldruck -> LED %s\n",
                          io5Ch.pin, ledOn ? "EIN" : "AUS");
            break;
        default:
            break;
    }
}

#endif // REED_TRIGGER_ENABLED
