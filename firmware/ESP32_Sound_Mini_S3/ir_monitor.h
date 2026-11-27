#ifndef IR_MONITOR_H
#define IR_MONITOR_H

#include <Arduino.h>
#include "pins.h"   // BOARD_C3_PRO_MINI

/*
 * ir_monitor.h - projektlokal (nicht upstream)
 *
 * Liest einen IR-Empfaenger (z. B. TSOP38238) an IO11 aus. NEC/NEC2-Frames
 * werden auf gelernte Aktionen geprueft (ir_control.*); alle anderen Frames
 * weiterhin ueber Serial ausgegeben, damit sich ein Sender identifizieren
 * laesst.
 *
 * Der Pin liegt bewusst HIER und nicht in pins.h: pins.h ist upstream-
 * identisch und soll fuer den patchfreien Upstream-Sync unberuehrt bleiben.
 * Auf dem ESP32-C3 ist IO11 nicht am Header und der Flash fast voll, daher
 * ist das Modul dort nur ein Stub.
 */
#ifndef IR_MONITOR_PIN
#define IR_MONITOR_PIN 11
#endif

void irMonitorBegin();
void irMonitorLoop();

#endif // IR_MONITOR_H
