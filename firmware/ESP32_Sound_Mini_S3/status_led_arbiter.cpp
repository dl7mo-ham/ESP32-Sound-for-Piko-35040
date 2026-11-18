/*
 * status_led_arbiter.cpp  -  Soundmodul Mini (siehe status_led_arbiter.h)
 */

#include "status_led_arbiter.h"
#include "flame_led.h"
#include "reed_roles.h"

// Boot-Snapshot der Portrollen (in der Sketch-Datei definiert). Rollenwechsel
// wirken erst nach einem Neustart - das ist also die tatsaechlich aktive Rolle.
extern uint8_t PortA_Role_boot;
extern uint8_t PortB_Role_boot;
extern uint8_t PortC_Role_boot;

static bool reedModeActive() {
    return appRoleClamp(PortA_Role_boot) == APP_UROLE_REED
        || appRoleClamp(PortB_Role_boot) == APP_UROLE_REED
        || appRoleClamp(PortC_Role_boot) == APP_UROLE_REED;
}

void ledArbiterUpdate(void (*statusUpdate)()) {
    if (flameIsOn()) {
        flameUpdate();          // Flamme hat Vorrang, Status-LED wird übersprungen
    } else if (reedModeActive()) {
        // Reed-Betrieb ohne Flamme: LED bleibt dunkel. flameSet(false) hat
        // bereits schwarz geschrieben; die RC-Statusanzeige wird bewusst nicht
        // gerufen (kein "kein Signal"-Blinken im Reed-Modus).
        return;
    } else if (statusUpdate) {
        statusUpdate();
    }
}
