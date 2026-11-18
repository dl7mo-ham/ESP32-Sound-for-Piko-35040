#ifndef REED_TRIGGER_H
#define REED_TRIGGER_H

/*
 * reed_trigger.h  -  Soundmodul Mini
 *
 * Reed-/Tastereingaenge pro Port (v0.14). Ist ein Port in der Rolle
 * APP_UROLE_REED (projektlokal, Wert 100 - siehe reed_roles.h), werden beide
 * Pins als INPUT_PULLUP betrieben (kein UART).
 * Ein Impuls (schliessende Flanke HIGH -> LOW) legt nach der Entprellzeit
 * die in config.ReedAction[port][pin] gewaehlte Aktion in die Kommando-Queue
 * des Hauptsketchs. Die Queue wird in loop() ueber denselben Pfad
 * ausgefuehrt wie Web-Testbuttons und die uebrigen Trigger.
 *
 * Ports: 0 = A (RX/TX aus PIN_CRSF_*), 1 = B (PIN_PORTB_*),
 *        2 = C (PIN_PORTC_*, nur wenn HAS_PORT_C).
 * Pins:  0 = RX, 1 = TX.
 *
 * Die Port-Rolle wirkt erst nach einem Neustart (Boot-Snapshot). Aktionen
 * und Entprellzeit werden dagegen bei jedem Impuls frisch aus config gelesen
 * und wirken sofort.
 */

#include <Arduino.h>
#include "config.h"
#include "command_queue.h"   // enqueueCommand(): Header statt extern-Deklaration

// Einmalig in setup() nach dem Setzen der Pins/des UARTs aufrufen.
void reedBegin(uint8_t roleA, uint8_t roleB, uint8_t roleC);

// Jeden Loop aufrufen. Ohne aktiven Reed-Port ohne Wirkung.
void reedUpdate();

// Ist der Port auf diesem Board vorhanden bzw. laeuft er in diesem Boot als
// Reed-Port? Port C fehlt beim ESP32-C3.
bool reedPortPresent(uint8_t port);
bool reedPortActive(uint8_t port);

// GPIO-Nummer eines Reed-Pins fuer Anzeige/Diagnose (z. B. 5 = IO5).
// 0xFF, wenn Port oder Pin auf diesem Board nicht vorhanden ist.
uint8_t reedPinNumber(uint8_t port, uint8_t pin);

// Anzahl der in diesem Boot aktiven Reed-Ports (0..3).
uint8_t reedActiveCount();

// Anzeige/Validierung der Aktionen (IDs siehe config.h).
const char* reedActionName(uint8_t action);
const char* reedActionOptions();   // "Keine;Sound 1;...;Flamme Ein/Aus"
uint8_t     reedActionClamp(int raw);

// Impulszaehler seit dem Start (Diagnose/Web-UI).
uint32_t reedTriggerCount(uint8_t port, uint8_t pin);

// enqueueCommand() (command_queue.h) wird von reedUpdate() und vom
// Web-Testbutton genutzt.

#endif // REED_TRIGGER_H
