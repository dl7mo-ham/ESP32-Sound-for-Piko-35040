#ifndef REED_TRIGGER_H
#define REED_TRIGGER_H

/*
 * reed_trigger.h  -  Soundmodul Mini
 *
 * Optionale Erweiterung: IO5 und IO6 als Reed-Kontakt-Eingänge statt CRSF.
 *
 * Wenn REED_TRIGGER_ENABLED definiert ist, werden IO5 (sound2) und IO6
 * (sound1) als INPUT_PULLUP konfiguriert. Jeder Reed-Impuls (LOW-Flanke)
 * triggert den zugehörigen Zusatzsound über Sound_on_web[] – denselben
 * Mechanismus, den auch der Web-Testbutton und das Lua-"Test Sound"-Feld
 * benutzen.
 *
 * WICHTIG: Wenn REED_TRIGGER_ENABLED aktiv ist, darf Port A (CRSF) NICHT
 * genutzt werden – IO5 und IO6 sind dann GPIO-Eingänge, keine UART-Pins.
 * Die CRSF-Initialisierung in setup() muss in diesem Fall entfernt oder
 * bedingt kompiliert werden (siehe Kommentar in ESP32_Sound_Mini_S3.ino).
 *
 * Pinbelegung (aus pins.h):
 *   IO6  -> sound1.wav  (war PIN_CRSF_TX)
 *   IO5  -> sound2.wav  (war PIN_CRSF_RX)
 *
 * Entprellzeit: REED_DEBOUNCE_MS (Standard 50 ms)
 * Mindestpause zwischen zwei Triggern desselben Sounds: REED_RETRIGGER_MS
 */

// ── Aktivierung ──────────────────────────────────────────────────────────
// Zeile auskommentieren = Reed-Trigger deaktiviert (normaler CRSF-Betrieb)
#define REED_TRIGGER_ENABLED

#ifdef REED_TRIGGER_ENABLED

#include <Arduino.h>

// ── Konfiguration ────────────────────────────────────────────────────────
static constexpr uint8_t  REED_PIN_SOUND1     = 6;    // IO6 -> sound1.wav
static constexpr uint8_t  REED_PIN_SOUND2     = 5;    // IO5 -> sound2.wav
static constexpr uint32_t REED_DEBOUNCE_MS    = 50;   // Entprellzeit
static constexpr uint32_t REED_RETRIGGER_MS   = 200;  // Mindestpause Retrigger

// ── Schnittstelle ────────────────────────────────────────────────────────

// Einmalig in setup() aufrufen (vor CRSF-Init ersetzen oder danach – aber
// CRSF darf in diesem Modus nicht initialisiert werden).
void reedTriggerBegin();

// In loop() aufrufen. Setzt Sound_on_web[1] bzw. Sound_on_web[2] bei einem
// Reed-Impuls. Das Array wird direkt beschrieben (extern-Deklaration in
// ESP32_Sound_Mini_S3.ino).
//
// sound_on_web: Zeiger auf das Sound_on_web[]-Array aus dem Hauptprogramm
// array_size:   Anzahl der Elemente (muss >= 3 sein)
void reedTriggerUpdate(bool* sound_on_web, uint8_t array_size);

#endif // REED_TRIGGER_ENABLED
#endif // REED_TRIGGER_H
