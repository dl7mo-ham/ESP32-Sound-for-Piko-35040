#ifndef FLAME_LED_H
#define FLAME_LED_H

/*
 * flame_led.h  -  Soundmodul Mini
 *
 * Nicht blockierender Flammen-Effekt auf der Status-LED. Der Effekt nutzt
 * dasselbe StatusLed-Objekt wie die RC-Statusanzeige, damit keine zwei
 * Treiber dieselbe WS2812 ansprechen. Solange die Flamme an ist, ueberlagert
 * sie die Status-LED (in loop() wird dann statusLed.update() uebersprungen).
 *
 * Geschaltet wird die Flamme ueber die Web-UI/Reed-Aktion (Toggle).
 */

#include <Arduino.h>

class StatusLed;

void flameBegin(StatusLed& led);
void flameUpdate();
void flameToggle();
void flameSet(bool on);
bool flameIsOn();

#endif // FLAME_LED_H
