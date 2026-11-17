/*
 * config_reed.cpp  -  Soundmodul Mini
 *
 * Reed-spezifische Konfiguration (siehe config_reed.h).
 */

#include "config_reed.h"
#include "reed_roles.h"

// Schluessel der Reed-Aktionen fuer /api/config (POST) und das JSON-
// Exportformat. Ports A/B/C, Pins 0=RX, 1=TX.
static const char* const REED_CONFIG_KEYS[REED_PORTS][REED_PINS] = {
    {"reed_a_rx", "reed_a_tx"},
    {"reed_b_rx", "reed_b_tx"},
    {"reed_c_rx", "reed_c_tx"},
};
static const char* const REED_PORT_KEYS[REED_PORTS]   = {"a", "b", "c"};
static const char* const REED_PORT_LABELS[REED_PORTS] = {"Port A", "Port B", "Port C"};
static const char* const REED_PIN_KEYS[REED_PINS]     = {"rx", "tx"};

uint16_t reedDebounceClamp(int raw) {
    return (uint16_t)constrain(raw, REED_DEBOUNCE_MIN_MS, REED_DEBOUNCE_MAX_MS);
}

bool reedConfigLoad(Preferences& p) {
    bool corrected = false;
    config.ReedDebounceMs = reedDebounceClamp(p.getInt("reeddb", REED_DEBOUNCE_DEFAULT_MS));
    if (p.getBytesLength("reedact") == sizeof(config.ReedAction)) {
        p.getBytes("reedact", config.ReedAction, sizeof(config.ReedAction));
    }
    for (uint8_t pi = 0; pi < REED_PORTS; pi++) {
        for (uint8_t i = 0; i < REED_PINS; i++) {
            uint8_t a = reedActionClamp(config.ReedAction[pi][i]);
            if (a != config.ReedAction[pi][i]) {
                config.ReedAction[pi][i] = a;
                corrected = true;
            }
        }
    }
    return corrected;
}

void reedConfigSave(Preferences& p) {
    p.putInt("reeddb", (int)config.ReedDebounceMs);
    p.putBytes("reedact", config.ReedAction, sizeof(config.ReedAction));
}

// Miniatur-JSON-Key-Suche (passend zu jsonFindKey() in WebServerManager.cpp:
// Schluessel muss an einer Wortgrenze stehen, Leerraum nach ':' erlaubt).
static bool reedJsonFindKey(const String& body, const char* key, int& valueStart) {
    String k = "\"" + String(key) + "\"";
    int searchFrom = 0;
    for (;;) {
        int pos = body.indexOf(k, searchFrom);
        if (pos < 0) return false;
        bool boundaryOk = (pos == 0) || !(isalnum((unsigned char)body[pos - 1]) || body[pos - 1] == '_');
        int afterKey = pos + k.length();
        int colon = afterKey;
        while (colon < (int)body.length() && isspace((unsigned char)body[colon])) colon++;
        if (boundaryOk && colon < (int)body.length() && body[colon] == ':') {
            valueStart = colon + 1;
            while (valueStart < (int)body.length() && isspace((unsigned char)body[valueStart])) valueStart++;
            return true;
        }
        searchFrom = pos + 1;
    }
}
bool reedJsonGetInt(const String& body, const char* key, int& out) {
    int vs;
    if (!reedJsonFindKey(body, key, vs)) return false;
    out = body.substring(vs).toInt();
    return true;
}

bool reedConfigApplyPost(const String& body) {
    bool changed = false;
    int v;
    if (reedJsonGetInt(body, "reed_debounce_ms", v)) {
        config.ReedDebounceMs = reedDebounceClamp(v);
        changed = true;
    }
    for (uint8_t p = 0; p < REED_PORTS; p++) {
        for (uint8_t i = 0; i < REED_PINS; i++) {
            if (reedJsonGetInt(body, REED_CONFIG_KEYS[p][i], v)) {
                config.ReedAction[p][i] = reedActionClamp(v);
                changed = true;
            }
        }
    }
    return changed;
}

void reedConfigAppendJson(String& j) {
    // v0.14: Reed-Trigger. actions ist ein flaches Objekt (a_rx, a_tx, ...),
    // ports liefert Praesenz, gespeicherte Rolle und die beim Start aktive
    // Rolle - damit die Web-UI den Neustart-Hinweis anzeigen kann.
    j += "\"reed\":{";
    j += "\"debounce_ms\":"  + String(config.ReedDebounceMs)        + ",";
    j += "\"debounce_min\":" + String(REED_DEBOUNCE_MIN_MS)         + ",";
    j += "\"debounce_max\":" + String(REED_DEBOUNCE_MAX_MS)         + ",";
    j += "\"action_options\":\"" + String(reedActionOptions())      + "\",";
    j += "\"actions\":{";
    for (uint8_t p = 0; p < REED_PORTS; p++) {
        for (uint8_t i = 0; i < REED_PINS; i++) {
            if (p || i) j += ",";
            j += "\"" + String(REED_PORT_KEYS[p]) + "_" + String(REED_PIN_KEYS[i]) + "\":";
            j += String(config.ReedAction[p][i]);
        }
    }
    j += "},\"ports\":[";
    for (uint8_t p = 0; p < REED_PORTS; p++) {
        if (p) j += ",";
        bool present = reedPortPresent(p);
        uint8_t role = (p == 0) ? config.PortA_Role
                     : (p == 1) ? config.PortB_Role : config.PortC_Role;
        bool configured = present && role == APP_UROLE_REED;
        j += "{\"key\":\"" + String(REED_PORT_KEYS[p]) + "\",";
        j += "\"label\":\"" + String(REED_PORT_LABELS[p]) + "\",";
        j += "\"present\":"    + String(present ? 1 : 0)    + ",";
        j += "\"configured\":" + String(configured ? 1 : 0) + ",";
        j += "\"active\":"     + String(reedPortActive(p) ? 1 : 0) + "}";
    }
    j += "]},";
}
