/*
 * ir_monitor.cpp - projektlokal (nicht upstream)
 *
 * Minimaler IR-Serial-Monitor mit Arduino-IRremote. Es wird bewusst nur der
 * Empfang genutzt; die vorhandene Status-LED wird nicht als Feedback-LED
 * vereinnahmt. Unbekannte Protokolle werden zusaetzlich als Roh-Timings
 * ausgegeben, damit sich der Sender trotzdem identifizieren laesst.
 */
#include "ir_monitor.h"

#if !defined(BOARD_C3_PRO_MINI)

// IRremote.hpp enthaelt die Implementierung und darf deshalb nur in DIESER
// Uebersetzungseinheit included werden (kein USE_IRREMOTE_HPP_AS_PLAIN_INCLUDE).
#include <IRremote.hpp>

void irMonitorBegin() {
    IrReceiver.begin(IR_MONITOR_PIN, USE_DEFAULT_FEEDBACK_LED_PIN, DISABLE_LED_FEEDBACK);
    Serial.printf("IR-Monitor: IRremote %s an IO%d, warte auf Signale\n",
                  VERSION_IRREMOTE, IR_MONITOR_PIN);
}

void irMonitorLoop() {
    if (!IrReceiver.decode()) return;

    const bool unknown = (IrReceiver.decodedIRData.protocol == UNKNOWN);
    if (unknown) {
        // Rohdaten muessen vor resume() gedruckt werden, danach ist der
        // Puffer bereits wieder fuer den naechsten Frame freigegeben.
        Serial.print(F("IR: "));
        IrReceiver.printIRResultShort(&Serial);
        Serial.println();
        IrReceiver.printIRResultRawFormatted(&Serial, true);
        IrReceiver.resume();
    } else {
        IrReceiver.resume();
        Serial.print(F("IR: "));
        IrReceiver.printIRResultShort(&Serial);
        Serial.println();
    }
}

#else  // ESP32-C3: IO12 nicht am Header, Flash knapp

void irMonitorBegin() {
    Serial.println(F("IR-Monitor: auf ESP32-C3 nicht aktiv (IO12 nicht am Header)"));
}

void irMonitorLoop() {}

#endif
