/*
 * ir_control.cpp  -  Soundmodul Mini (projektlokal), siehe ir_control.h
 */

#include "ir_control.h"
#include "config.h"        // markDirty()
#include "config_reed.h"   // reedJsonGetInt()
#include "reed_trigger.h"  // reedActionName()

static IrCode irCodes[REED_ACTION_COUNT];
static uint8_t  learnAction   = REED_ACTION_NONE;
static uint32_t learnStarted  = 0;
static bool     resultPending = false;
static uint8_t  resultAction  = REED_ACTION_NONE;
static IrCode   resultCode    = { IR_PROTO_NONE, 0, 0 };
static IrCode   lastCode      = { IR_PROTO_NONE, 0, 0 };
static uint8_t  lastAction    = REED_ACTION_NONE;
static uint32_t lastMs        = 0;

static void irCodeClear(IrCode& code) {
    code.protocol = IR_PROTO_NONE;
    code.address  = 0;
    code.command  = 0;
}

static void irNoteLast(const IrCode& code, uint8_t action) {
    lastCode   = code;
    lastAction = action;
    lastMs     = millis();
}

// ── NVS / JSON ────────────────────────────────────────────────────────────

bool irConfigLoad(Preferences& p) {
    bool corrected = false;
    size_t len = p.getBytesLength("ircode");
    if (len == sizeof(irCodes)) {
        p.getBytes("ircode", irCodes, sizeof(irCodes));
    } else {
        // Bewusst kein Migrationspfad: dl7mo.12 war nur lokal im Einsatz.
        if (len > 0) corrected = true;
        for (uint8_t a = 0; a < REED_ACTION_COUNT; a++) irCodeClear(irCodes[a]);
    }
    for (uint8_t a = 0; a < REED_ACTION_COUNT; a++) {
        uint8_t proto = irCodeClampProtocol(irCodes[a].protocol);
        if (proto != irCodes[a].protocol) corrected = true;
        irCodes[a].protocol = proto;
        if (proto == IR_PROTO_NONE) {
            if (irCodes[a].address || irCodes[a].command) corrected = true;
            irCodeClear(irCodes[a]);
        }
    }
    return corrected;
}

void irConfigSave(Preferences& p) {
    p.putBytes("ircode", irCodes, sizeof(irCodes));
}

void irConfigAppendJson(String& j) {
    j += "\"ir\":{";
    j += "\"available\":" + String((int)IR_AVAILABLE) + ",";
    j += "\"timeout_ms\":" + String((unsigned)IR_LEARN_TIMEOUT_MS) + ",";
    j += "\"codes\":[";
    for (uint8_t a = 1; a < REED_ACTION_COUNT; a++) {
        if (a > 1) j += ",";
        j += "{\"action\":"     + String((int)a) + ",";
        j += "\"protocol\":"    + String((int)irCodes[a].protocol) + ",";
        j += "\"address\":"     + String((int)irCodes[a].address) + ",";
        j += "\"command\":"     + String((int)irCodes[a].command) + "}";
    }
    j += "]},";
}

bool irConfigApplyPost(const String& body) {
    bool changed = false;
    int v;
    for (uint8_t a = 1; a < REED_ACTION_COUNT; a++) {
        String protoKey = "ir_" + String((int)a) + "_protocol";
        String addrKey  = "ir_" + String((int)a) + "_address";
        String cmdKey   = "ir_" + String((int)a) + "_command";

        if (reedJsonGetInt(body, protoKey.c_str(), v)) {
            uint8_t proto = irCodeClampProtocol(v);
            if (proto != irCodes[a].protocol) {
                irCodes[a].protocol = proto;
                changed = true;
            }
        }
        if (reedJsonGetInt(body, addrKey.c_str(), v)) {
            uint16_t addr = (uint16_t)constrain(v, 0, 0xFFFF);
            if (addr != irCodes[a].address) { irCodes[a].address = addr; changed = true; }
        }
        if (reedJsonGetInt(body, cmdKey.c_str(), v)) {
            uint8_t cmd = (uint8_t)constrain(v, 0, 0xFF);
            if (cmd != irCodes[a].command) { irCodes[a].command = cmd; changed = true; }
        }
        if (irCodes[a].protocol == IR_PROTO_NONE &&
            (irCodes[a].address || irCodes[a].command)) {
            irCodeClear(irCodes[a]);
            changed = true;
        }
    }
    return changed;
}

// ── Zuordnung ─────────────────────────────────────────────────────────────

const IrCode& irCodeForAction(uint8_t action) {
    static const IrCode empty = { IR_PROTO_NONE, 0, 0 };
    return (action < REED_ACTION_COUNT) ? irCodes[action] : empty;
}

uint8_t irFindAction(const IrCode& code) {
    if (!irCodeValid(code)) return REED_ACTION_NONE;
    for (uint8_t a = 1; a < REED_ACTION_COUNT; a++) {
        if (irCodeValid(irCodes[a]) && irCodeEqual(irCodes[a], code)) return a;
    }
    return REED_ACTION_NONE;
}

void irClearCode(uint8_t action) {
    if (action == REED_ACTION_NONE || action >= REED_ACTION_COUNT) return;
    if (!irCodeValid(irCodes[action])) return;
    irCodeClear(irCodes[action]);
    markDirty();
    Serial.printf("[IR] Code von Aktion %d geloescht\n", (int)action);
}

// ── Lernmodus ─────────────────────────────────────────────────────────────

void irLearnStart(uint8_t action) {
    if (action >= REED_ACTION_COUNT) return;
    if (action == REED_ACTION_NONE) { irLearnCancel(); return; }
    if (!IR_AVAILABLE) return;
    learnAction   = action;
    learnStarted  = millis();
    resultPending = false;
    resultAction  = REED_ACTION_NONE;
    irCodeClear(resultCode);   // kein alter Wert darf ein Timeout als Erfolg melden
    Serial.printf("[IR] Lernmodus: Aktion %d, %u ms auf Taste warten\n",
                  (int)action, (unsigned)IR_LEARN_TIMEOUT_MS);
}

void irLearnCancel() {
    if (!learnAction) return;
    learnAction = REED_ACTION_NONE;
    Serial.println(F("[IR] Lernmodus abgebrochen"));
}

void irLearnTick() {
    if (!learnAction) return;
    if ((millis() - learnStarted) >= IR_LEARN_TIMEOUT_MS) {
        learnAction = REED_ACTION_NONE;
        Serial.println(F("[IR] Lernzeit abgelaufen"));
    }
}

bool irLearnActive()      { return learnAction != REED_ACTION_NONE; }
uint8_t irLearnAction()   { return learnAction; }

uint32_t irLearnRemainingMs() {
    if (!learnAction) return 0;
    uint32_t elapsed = millis() - learnStarted;
    return elapsed >= IR_LEARN_TIMEOUT_MS ? 0 : (IR_LEARN_TIMEOUT_MS - elapsed);
}

bool irLearnResultPending() { return resultPending; }
uint8_t irLearnResultAction() { return resultAction; }
IrCode irLearnResultCode()  { return resultCode; }

void irLastReceived(IrCode& code, uint8_t& action, uint32_t& ageMs) {
    code   = lastCode;
    action = lastAction;
    ageMs  = (lastCode.protocol == IR_PROTO_NONE) ? 0 : (millis() - lastMs);
}

bool irHandleNec(uint8_t protocol, uint16_t address, uint8_t command) {
    IrCode code = { protocol, address, command };
    if (!irCodeValid(code)) return false;
    irLearnTick();

    uint8_t action = learnAction ? learnAction : irFindAction(code);
    irNoteLast(code, action);   // auch ohne Zuordnung fuer die Web-UI sichtbar

    if (learnAction) {
        // "Letzter gewinnt": derselbe Code darf nicht auf zwei Aktionen liegen.
        for (uint8_t a = 1; a < REED_ACTION_COUNT; a++) {
            if (a != action && irCodeValid(irCodes[a]) && irCodeEqual(irCodes[a], code)) {
                Serial.printf("[IR] Code war Aktion %d, jetzt Aktion %d\n",
                              (int)a, (int)action);
                irCodeClear(irCodes[a]);
            }
        }
        irCodes[action] = code;
        resultPending = true;
        resultAction  = action;
        resultCode    = code;
        learnAction   = REED_ACTION_NONE;
        markDirty();
        Serial.printf("[IR] gelernt: Aktion %d, Proto %u, Adresse 0x%04X, Command 0x%02X\n",
                      (int)resultAction, (unsigned)code.protocol,
                      (unsigned)code.address, (unsigned)code.command);
        return true;
    }

    if (action == REED_ACTION_NONE) return false;
    enqueueCommand(action);
    Serial.printf("[IR] Trigger -> %s\n", reedActionName(action));
    return true;
}
