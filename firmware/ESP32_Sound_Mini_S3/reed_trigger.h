#ifndef REED_TRIGGER_H
#define REED_TRIGGER_H

/*
 * reed_trigger.h  -  Soundmodul Mini
 *
 * Optionale Erweiterung: IO5 und IO6 als Reed-Kontakt-Eingänge statt CRSF.
 *
 * Der Piko 35040 kann keine langen Reed-Impulse senden, nur kurze Pulse.
 * Deshalb wird zwischen Einzel- und Doppeldruck unterschieden:
 *   IO6  -> - kurzer Druck                -> sound1.wav
 *           - doppelter Druck             -> Motorsound ein-/ausschalten
 *   IO5  -> - kurzer Druck                -> sound2.wav
 *           - doppelter Druck             -> Flammen-LED ein-/ausschalten
 *
 * Ein Einzeldruck wird erst nach Ablauf des Doppeldruck-Fensters
 * (config.ReedDoublePressMs, per Web-UI einstellbar) ausgelöst, damit ein
 * zweiter Puls noch als Doppeldruck erkannt werden kann. Die Reaktion auf
 * einen Einzeldruck ist daher um dieses Fenster verzögert.
 *
 * WICHTIG: Wenn REED_TRIGGER_ENABLED aktiv ist, darf Port A (CRSF) NICHT
 * genutzt werden – IO5 und IO6 sind dann GPIO-Eingänge, keine UART-Pins.
 * Die CRSF-Initialisierung in setup() wird in diesem Fall bedingt kompiliert.
 *
 * Pinbelegung (aus pins.h):
 *   IO6  -> sound1 / Motorsound (war PIN_CRSF_TX)
 *   IO5  -> sound2 / LED        (war PIN_CRSF_RX)
 *
 * Entprellzeit: REED_DEBOUNCE_MS (Standard 20 ms)
 * Doppeldruck-Fenster: config.ReedDoublePressMs (Standard 600 ms,
 *                      einstellbar in der Web-UI unter Einstellungen)
 */

// ── Aktivierung ──────────────────────────────────────────────────────────
// Zeile auskommentieren = Reed-Trigger deaktiviert (normaler CRSF-Betrieb)
#define REED_TRIGGER_ENABLED

#ifdef REED_TRIGGER_ENABLED

#include <Arduino.h>
#include "pins.h"   // PIN_STATUS_LED

// ── Konfiguration ────────────────────────────────────────────────────────
static constexpr uint8_t  REED_PIN_IO6        = 6;    // Kurz: sound1 / Doppelt: Motor Toggle
static constexpr uint8_t  REED_PIN_IO5        = 5;    // Kurz: sound2 / Doppelt: LED Toggle
static constexpr uint8_t  REED_PIN_LED        = PIN_STATUS_LED;  // RGB-LED (WS2812)
static constexpr uint8_t  REED_PIN_SOUND1     = REED_PIN_IO6;    // Kompatibilitaet
static constexpr uint8_t  REED_PIN_SOUND2     = REED_PIN_IO5;    // Kompatibilitaet
static constexpr uint8_t  REED_PIN_MOTOR      = REED_PIN_IO6;    // Kompatibilitaet
static constexpr uint8_t  REED_PIN_SOUNDS     = REED_PIN_IO5;    // Kompatibilitaet

static constexpr uint32_t REED_DEBOUNCE_MS          = 20;   // Entprellzeit
// Werkseinstellung; der aktive Wert kommt aus config.ReedDoublePressMs und
// ist in der Web-UI unter "Einstellungen -> Reed-Kontakte" einstellbar.
static constexpr uint32_t REED_DOUBLE_PRESS_DEFAULT_MS = 600;

// Flammen-Effekt der Status-LED im Reed-Modus (per doppeltem IO5-Druck ein/aus)
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
// sound_on_motor: Referenz auf Sound_on_Motor (wird bei doppeltem IO6-Druck
//                 getoggelt; ein kurzer IO6-Druck spielt sound1)
void reedTriggerUpdate(bool* sound_on_web, uint8_t array_size, bool& sound_on_motor);

#endif // REED_TRIGGER_ENABLED
#endif // REED_TRIGGER_H
