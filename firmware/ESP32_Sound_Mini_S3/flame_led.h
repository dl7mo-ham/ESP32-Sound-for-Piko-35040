#ifndef FLAME_LED_H
#define FLAME_LED_H

/*
 * flame_led.h  -  Soundmodul Mini
 *
 * Flammen-Effekt der Status-LED (WS2812). Ausgelagert aus reed_trigger.cpp,
 * weil die LED nicht mehr ueber einen langen Reed-Impuls, sondern ueber die
 * Web-UI geschaltet wird. Spaeter koennen auch Taster/Schalter an IO7/IO8
 * (siehe Planung) dieselben Funktionen aufrufen.
 *
 * Der Effekt ist nur im Reed-Modus sinnvoll: dort laeuft die
 * CRSF-Status-LED (status_led.h) nicht, die LED ist also frei.
 * In loop() daher nur bei REED_TRIGGER_ENABLED aufrufen.
 *
 *   flameBegin()   einmalig in setup()
 *   flameUpdate()  in loop(), rendert das Flackern (nicht blockierend)
 *   flameToggle()  schaltet ein/aus (Web-UI, spaeter IO7/IO8)
 *   flameSet(on)   explizit ein/aus
 *   flameIsOn()    aktueller Zustand
 */

#include <Arduino.h>

void flameBegin();
void flameUpdate();
void flameToggle();
void flameSet(bool on);
bool flameIsOn();

#endif // FLAME_LED_H
