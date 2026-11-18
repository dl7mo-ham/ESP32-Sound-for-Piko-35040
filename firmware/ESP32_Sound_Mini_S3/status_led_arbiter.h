/*
 * status_led_arbiter.h  -  Soundmodul Mini
 *
 * Prioritaetsregel fuer die gemeinsame WS2812: Solange die Flamme an ist,
 * gehoert die LED ihr; sonst zeichnet die RC-Statusanzeige. Im Reed-Betrieb
 * bleibt die LED ohne Flamme dunkel (kein "kein Signal"-Blinken). Damit steht
 * in loop() nur noch ein Aufruf statt der if/else-Verzweigung (v0.15).
 */

#ifndef STATUS_LED_ARBITER_H
#define STATUS_LED_ARBITER_H

// statusUpdate: Callback der Statusanzeige (updateLed() im Sketch). Darf
// nullptr sein - dann bleibt die LED bei ausgeschalteter Flamme unveraendert.
void ledArbiterUpdate(void (*statusUpdate)());

#endif
