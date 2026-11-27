#ifndef IR_CODE_H
#define IR_CODE_H

/*
 * ir_code.h  -  Soundmodul Mini (projektlokal)
 *
 * Protokoll-unabhaengige Darstellung eines gelernten IR-Codes. Die Werte
 * sind bewusst nicht die IRremote-enums, damit NVS und /api/config auch bei
 * einer Bibliotheks-Aktualisierung stabil bleiben.
 *
 * Diese Datei kommt ohne Arduino-Header aus und kann daher auf dem Host
 * getestet werden (tests/test_ir_code.cpp).
 */

#include <stdint.h>

static constexpr uint8_t IR_PROTO_NONE = 0;
static constexpr uint8_t IR_PROTO_NEC  = 1;
static constexpr uint8_t IR_PROTO_NEC2 = 2;
static constexpr uint8_t IR_PROTO_COUNT = 3;

struct IrCode {
    uint8_t  protocol;
    uint16_t address;
    uint8_t  command;
};

// Ein Code ist nur gueltig, wenn ein bekanntes Protokoll hinterlegt ist.
bool irCodeValid(const IrCode& code);
bool irCodeEqual(const IrCode& a, const IrCode& b);

// Unbekannte Werte werden auf "nicht belegt" zurueckgefuehrt.
uint8_t irCodeClampProtocol(int raw);

#endif // IR_CODE_H
