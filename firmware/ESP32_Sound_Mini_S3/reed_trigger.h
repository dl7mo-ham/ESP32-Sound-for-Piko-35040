#ifndef REED_TRIGGER_H
#define REED_TRIGGER_H

/*
 * reed_trigger.h  -  Soundmodul Mini
 *
 * Optionale Erweiterung: IO5 und IO6 als Reed-Kontakt-Eingänge statt CRSF.
 *
 * Wenn REED_TRIGGER_ENABLED definiert ist, werden IO5 und IO6 als
 * INPUT_PULLUP konfiguriert. Beide reagieren nur auf einfache Impulse
 * (keine Kurz-/Lang-Unterscheidung):
 *   IO6  -> sound1.wav
 *   IO5  -> sound2.wav
 *
 * Grund: Der Piko 35040 liefert keine zuverlaessig unterscheidbaren langen
 * Tastendruecke. Motorsound und Flammen-LED werden deshalb nicht mehr ueber
 * Reed-Impulse geschaltet, sondern ueber die Web-UI (Tab "Motor") und
 * spaeter optional ueber Taster/Schalter an IO7/IO8.
 *
 * WICHTIG: Wenn REED_TRIGGER_ENABLED aktiv ist, darf Port A (CRSF) NICHT
 * genutzt werden – IO5 und IO6 sind dann GPIO-Eingänge, keine UART-Pins.
 * Die CRSF-Initialisierung in setup() wird in diesem Fall bedingt kompiliert.
 *
 * Pinbelegung (aus pins.h):
 *   IO6  -> sound1.wav (war PIN_CRSF_TX)
 *   IO5  -> sound2.wav (war PIN_CRSF_RX)
 *
 * Die Entprellzeit ist in der Web-UI unter "Einstellungen ->
 * Reed-Kontakte" einstellbar und wird in config.ReedDebounceMs gespeichert
 * (Standard 50 ms).
 */

// ── Aktivierung ──────────────────────────────────────────────────────────
// Zeile auskommentieren = Reed-Trigger deaktiviert (normaler CRSF-Betrieb)
#define REED_TRIGGER_ENABLED

#ifdef REED_TRIGGER_ENABLED

#include <Arduino.h>

// ── Konfiguration ────────────────────────────────────────────────────────
static constexpr uint8_t  REED_PIN_SOUND1 = 6;    // IO6 -> sound1.wav
static constexpr uint8_t  REED_PIN_SOUND2 = 5;    // IO5 -> sound2.wav

// Werkseinstellung; der aktive Wert kommt aus config.ReedDebounceMs und ist
// in der Web-UI unter "Einstellungen -> Reed-Kontakte" einstellbar.
static constexpr uint32_t REED_DEBOUNCE_DEFAULT_MS = 50;   // Entprellzeit

// ── Schnittstelle ────────────────────────────────────────────────────────

// Einmalig in setup() aufrufen.
void reedTriggerBegin();

// In loop() aufrufen.
// sound_on_web: Zeiger auf das Sound_on_web[]-Array aus dem Hauptprogramm
// array_size:   Anzahl der Elemente (muss >= 3 sein)
void reedTriggerUpdate(bool* sound_on_web, uint8_t array_size);

#endif // REED_TRIGGER_ENABLED
#endif // REED_TRIGGER_H
