/*
 * ir_monitor.cpp - projektlokal (nicht upstream)
 *
 * Einziger IR-Empfaenger (IO11, nur S3). NEC/NEC2-Frames ohne Repeat werden
 * an ir_control.cpp weitergegeben (Lernen oder Aktion); alle anderen
 * Protokolle werden weiterhin als Serial-Monitor ausgegeben, damit sich ein
 * unbekannter Sender identifizieren laesst.
 */

#include "ir_monitor.h"

#if !defined(BOARD_C3_PRO_MINI)

// IRremote.hpp enthaelt die Implementierung und darf deshalb nur in DIESER
// Uebersetzungseinheit included werden (kein USE_IRREMOTE_HPP_AS_PLAIN_INCLUDE).
#include <IRremote.hpp>
#include "ir_control.h"

void irMonitorBegin() {
    IrReceiver.begin(IR_MONITOR_PIN, USE_DEFAULT_FEEDBACK_LED_PIN, DISABLE_LED_FEEDBACK);
    Serial.printf("IR-Monitor: IRremote %s an IO%d, NEC/NEC2 -> Aktionen\n",
                  VERSION_IRREMOTE, IR_MONITOR_PIN);
}

void irMonitorLoop() {
    irLearnTick();
    if (!IrReceiver.decode()) return;

    // NEC sendet bei gedrueckt gehaltener Taste Wiederholframes. Die wuerden
    // sonst mehrfach in die Kommando-Queue schreiben.
    if (IrReceiver.decodedIRData.flags & IRDATA_FLAGS_IS_REPEAT) {
        IrReceiver.resume();
        return;
    }

    uint8_t proto = IR_PROTO_NONE;
    if (IrReceiver.decodedIRData.protocol == NEC) proto = IR_PROTO_NEC;
    else if (IrReceiver.decodedIRData.protocol == NEC2) proto = IR_PROTO_NEC2;

    if (proto != IR_PROTO_NONE) {
        uint16_t address = IrReceiver.decodedIRData.address;
        uint8_t  command = (uint8_t)IrReceiver.decodedIRData.command;
        IrReceiver.resume();
        if (irHandleNec(proto, address, command)) return;
        Serial.printf("IR: NEC%s Adresse=0x%04X Command=0x%02X ohne Zuordnung\n",
                      proto == IR_PROTO_NEC2 ? "2" : "", (unsigned)address, (unsigned)command);
        return;
    }

    // Andere Protokolle: wie bisher den Decoded-Report ausgeben.
    Serial.print(F("IR: "));
    IrReceiver.printIRResultShort(&Serial);
    Serial.println();
    IrReceiver.resume();
}

#else  // ESP32-C3: IO11 nicht am Header, Flash knapp

void irMonitorBegin() {
    Serial.println(F("IR-Monitor: auf ESP32-C3 nicht aktiv (IO11 nicht am Header)"));
}

void irMonitorLoop() {}

#endif
