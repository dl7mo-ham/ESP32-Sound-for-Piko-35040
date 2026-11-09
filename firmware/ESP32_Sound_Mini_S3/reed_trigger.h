#ifndef REED_TRIGGER_H
#define REED_TRIGGER_H

/*
 * reed_trigger.h  -  Soundmodul Mini
 *
 * Optionale Erweiterung: IO5 und IO6 als Reed-Kontakt-Eingänge statt CRSF.
 *
 * Wenn REED_TRIGGER_ENABLED definiert ist, werden IO5 und IO6 als
 * INPUT_PULLUP konfiguriert:
 *   IO6  -> Motorsound und LED:
 *             - kurzer Impuls (< 800 ms)   -> Motorsound (start, loop, shut) togglen
 *             - langer Impuls (>= 800 ms)  -> Flammen-LED an- bzw. ausschalten
 *   IO5  -> Zusatzsounds:
 *             - kurzer Impuls (< 800 ms)   -> sound1.wav
 *             - langer Impuls (>= 800 ms)  -> sound2.wav
 *
 * WICHTIG: Wenn REED_TRIGGER_ENABLED aktiv ist, darf Port A (CRSF) NICHT
 * genutzt werden – IO5 und IO6 sind dann GPIO-Eingänge, keine UART-Pins.
 * Die CRSF-Initialisierung in setup() wird in diesem Fall bedingt kompiliert.
 *
 * Pinbelegung (aus pins.h):
 *   IO6  -> Motorsound (war PIN_CRSF_TX)
 *   IO5  -> sound1 / sound2 (war PIN_CRSF_RX)
 *
 * Die drei Zeitparameter sind in der Web-UI unter "Einstellungen ->
 * Reed-Kontakte" einstellbar und werden in config.Reed*Ms gespeichert:
 *   Entprellzeit:                config.ReedDebounceMs    (Standard 50 ms)
 *   Schwellwert langer Impuls:   config.ReedLongPressMs   (Standard 800 ms)
 *   Mindestpause zwischen Triggern: config.ReedRetriggerMs (Standard 200 ms)
 */

// ── Aktivierung ──────────────────────────────────────────────────────────
// Zeile auskommentieren = Reed-Trigger deaktiviert (normaler CRSF-Betrieb)
#define REED_TRIGGER_ENABLED

#ifdef REED_TRIGGER_ENABLED

#include <Arduino.h>
#include "pins.h"   // PIN_STATUS_LED

// ── Konfiguration ────────────────────────────────────────────────────────
static constexpr uint8_t  REED_PIN_MOTOR      = 6;    // IO6 -> Kurz: Motor Toggle / Lang: LED Toggle
static constexpr uint8_t  REED_PIN_SOUNDS     = 5;    // IO5 -> Kurz: sound1.wav / Lang: sound2.wav
static constexpr uint8_t  REED_PIN_LED        = PIN_STATUS_LED;  // RGB-LED (WS2812)
static constexpr uint8_t  REED_PIN_SOUND1     = REED_PIN_MOTOR;  // Kompatibilitaet
static constexpr uint8_t  REED_PIN_SOUND2     = REED_PIN_SOUNDS; // Kompatibilitaet

// Werkseinstellungen; die aktiven Werte kommen aus config.Reed*Ms und sind
// in der Web-UI unter "Einstellungen -> Reed-Kontakte" einstellbar.
static constexpr uint32_t REED_DEBOUNCE_DEFAULT_MS   = 50;   // Entprellzeit
static constexpr uint32_t REED_LONG_PRESS_DEFAULT_MS = 800;  // Schwelle langer Impuls (ms)
static constexpr uint32_t REED_RETRIGGER_DEFAULT_MS  = 200;  // Mindestpause Retrigger

// Flammen-Effekt der Status-LED im Reed-Modus (per langem IO6-Impuls ein/aus)
static constexpr uint8_t  REED_LED_BRIGHTNESS       = 40;  // Maximalhelligkeit
static constexpr uint32_t REED_FLAME_INTERVAL_MS    = 33;  // ~30 fps
static constexpr uint8_t  REED_FLAME_MIN_BRIGHTNESS = 10;  // Mindesthelligkeit
static constexpr uint8_t  REED_FLAME_GREEN_PCT      = 25;  // 0=rot, 100=gruen -> 25 = warmes Orange

// ── Schnittstelle ────────────────────────────────────────────────────────

// Einmalig in setup() aufrufen.
void reedTriggerBegin();

// In loop() aufrufen.
// sound_on_web: Zeiger auf das Sound_on_web[]-Array aus dem Hauptprogramm
// array_size:   Anzahl der Elemente (muss >= 3 sein)
// sound_on_motor: Referenz auf Sound_on_Motor (wird bei kurzem IO6-Impuls getoggelt;
//                 ein langer IO6-Impuls toggelt stattdessen die LED)
void reedTriggerUpdate(bool* sound_on_web, uint8_t array_size, bool& sound_on_motor);

#endif // REED_TRIGGER_ENABLED
#endif // REED_TRIGGER_H
