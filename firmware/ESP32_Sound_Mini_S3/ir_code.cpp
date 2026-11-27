/*
 * ir_code.cpp  -  Soundmodul Mini (projektlokal), siehe ir_code.h
 */

#include "ir_code.h"

bool irCodeValid(const IrCode& code) {
    return code.protocol != IR_PROTO_NONE && code.protocol < IR_PROTO_COUNT;
}

bool irCodeEqual(const IrCode& a, const IrCode& b) {
    return a.protocol == b.protocol &&
           a.address  == b.address &&
           a.command  == b.command;
}

uint8_t irCodeClampProtocol(int raw) {
    return (raw > 0 && raw < IR_PROTO_COUNT) ? (uint8_t)raw : IR_PROTO_NONE;
}
