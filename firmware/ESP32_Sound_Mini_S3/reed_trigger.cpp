/*
 * reed_trigger.cpp  -  Soundmodul Mini
 *
 * Reed-/Tastereingaenge je Port. Ein Trigger wird nur bei der schliessenden
 * Flanke (HIGH -> LOW) ausgeloest, nachdem der Pegel die konfigurierte
 * Entprellzeit stabil gehalten hat. Kein Repeat, kein Long-Press.
 */

#include "reed_trigger.h"
#include "reed_roles.h"
#include "config_reed.h"   // reedDebounceClamp()
#include "pins.h"

struct ReedPinState {
    bool     rawLast;
    bool     stable;
    uint32_t lastChangeMs;
    uint32_t count;
};

static const uint8_t REED_PIN_MAP[REED_PORTS][REED_PINS] = {
    { PIN_CRSF_RX,  PIN_CRSF_TX  },
    { PIN_PORTB_RX, PIN_PORTB_TX },
    { PIN_PORTC_RX, PIN_PORTC_TX },
};

static ReedPinState pinState[REED_PORTS][REED_PINS];
static bool    portPresent[REED_PORTS];
static bool    portActive[REED_PORTS];

static const char* const REED_ACTION_NAMES[REED_ACTION_COUNT] = {
    "Keine", "Sound 1", "Sound 2", "Sound 3", "Sound 4",
    "Motor Ein/Aus", "Flamme Ein/Aus"
};

const char* reedActionName(uint8_t action) {
    return action < REED_ACTION_COUNT ? REED_ACTION_NAMES[action] : REED_ACTION_NAMES[REED_ACTION_NONE];
}

const char* reedActionOptions() {
    return "Keine;Sound 1;Sound 2;Sound 3;Sound 4;Motor Ein/Aus;Flamme Ein/Aus";
}

uint8_t reedActionClamp(int raw) {
    return (raw >= 0 && raw < REED_ACTION_COUNT) ? (uint8_t)raw : REED_ACTION_NONE;
}

bool reedPortPresent(uint8_t port) {
    return port < REED_PORTS && portPresent[port];
}

bool reedPortActive(uint8_t port) {
    return port < REED_PORTS && portActive[port];
}

uint8_t reedPinNumber(uint8_t port, uint8_t pin) {
    if (port >= REED_PORTS || pin >= REED_PINS) return 0xFF;
    return REED_PIN_MAP[port][pin];
}

uint8_t reedActiveCount() {
    uint8_t n = 0;
    for (uint8_t p = 0; p < REED_PORTS; p++) if (portActive[p]) n++;
    return n;
}

uint32_t reedTriggerCount(uint8_t port, uint8_t pin) {
    if (port >= REED_PORTS || pin >= REED_PINS) return 0;
    return pinState[port][pin].count;
}

void reedBegin(uint8_t roleA, uint8_t roleB, uint8_t roleC) {
    const uint8_t roles[REED_PORTS] = {
        portaRoleClamp(roleA), appRoleClamp(roleB), appRoleClamp(roleC)
    };
    unsigned long now = millis();

    for (uint8_t p = 0; p < REED_PORTS; p++) {
        portPresent[p] = (p < REED_PORTS - 1) || HAS_PORT_C;
        portActive[p]  = portPresent[p] && roles[p] == APP_UROLE_REED;
        for (uint8_t i = 0; i < REED_PINS; i++) {
            ReedPinState& st = pinState[p][i];
            st.rawLast      = true;   // Pull-up: offener Kontakt = HIGH
            st.stable       = true;
            st.lastChangeMs = now;
            st.count        = 0;
            if (portActive[p]) pinMode(REED_PIN_MAP[p][i], INPUT_PULLUP);
        }
        if (portActive[p]) {
            Serial.printf("[Reed] Port %c: RX=%d TX=%d als Eingang (Pull-up), "
                          "Entprellung %u ms, RX->%s, TX->%s\n",
                          (char)('A' + p), REED_PIN_MAP[p][0], REED_PIN_MAP[p][1],
                          (unsigned)reedDebounceClamp(config.ReedDebounceMs),
                          reedActionName(config.ReedAction[p][0]),
                          reedActionName(config.ReedAction[p][1]));
        }
    }
}

void reedUpdate() {
    unsigned long now = millis();
    uint16_t debounce = reedDebounceClamp(config.ReedDebounceMs);

    for (uint8_t p = 0; p < REED_PORTS; p++) {
        if (!portActive[p]) continue;
        for (uint8_t i = 0; i < REED_PINS; i++) {
            ReedPinState& st = pinState[p][i];
            bool raw = (bool)digitalRead(REED_PIN_MAP[p][i]);

            if (raw != st.rawLast) {
                st.lastChangeMs = now;
                st.rawLast = raw;
            }
            if ((now - st.lastChangeMs) < debounce) continue;
            if (raw == st.stable) continue;

            st.stable = raw;
            if (st.stable) continue;   // nur die schliessende Flanke (LOW)

            st.count++;
            uint8_t action = reedActionClamp(config.ReedAction[p][i]);
            Serial.printf("[Reed] Port %c %s Impuls #%u -> %s\n",
                          (char)('A' + p), i == 0 ? "RX" : "TX",
                          (unsigned)st.count, reedActionName(action));
            if (action != REED_ACTION_NONE) enqueueCommand(action);
        }
    }
}
