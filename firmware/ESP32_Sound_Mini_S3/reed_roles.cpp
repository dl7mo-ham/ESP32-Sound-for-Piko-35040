/*
 * reed_roles.cpp  -  Soundmodul Mini
 * Siehe reed_roles.h. Rein additiv zu RC_Common, keine Upstream-Datei wird geoeffnet.
 */

#include "reed_roles.h"

const char* appRoleName(uint8_t role) {
    if (appRoleClamp(role) == APP_UROLE_REED) return "Reed";
    return uartRoleName(uartRoleClamp(role));
}

const char* appRoleOptions() { return "Aus;CRSF;S.Port;GPS;Raw;Reed"; }
const char* appRoleValues()  { return "0;1;2;3;4;100"; }

uint8_t appRoleClamp(int raw) {
    if (raw == APP_UROLE_REED || raw == APP_UROLE_REED_LEGACY) return APP_UROLE_REED;
    return uartRoleClamp(raw);
}

uint32_t appRoleDefaultBaud(uint8_t role) {
    if (appRoleClamp(role) == APP_UROLE_REED) return 0;
    return uartRoleDefaultBaud(uartRoleClamp(role));
}

uint8_t appRoleUiIndex(uint8_t role) {
    return appRoleClamp(role) == APP_UROLE_REED ? APP_UROLE_REED_INDEX : uartRoleClamp(role);
}
