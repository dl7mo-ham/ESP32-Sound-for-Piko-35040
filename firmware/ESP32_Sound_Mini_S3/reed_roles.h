#ifndef REED_ROLES_H
#define REED_ROLES_H

/*
 * reed_roles.h  -  Soundmodul Mini (projektlokal, NICHT RC_Common)
 *
 * RC_Common 1.4.0 behandelt Rollenwerte als dichten Index 0..UROLE_COUNT-1.
 * Die Reed-Rolle wird deshalb hier als App-Wert 100 gefuehrt: weit ausserhalb
 * des Upstream-Bereichs, kollidiert nicht mit spaeteren Upstream-Rollen 5..99.
 * RC_Common sieht fuer Reed weiterhin UROLE_OFF (kein UART, keine Telemetrie);
 * die Pull-ups setzt reedBegin().
 *
 * Gespeicherte Werte (NVS/Web/Lua): 0..4 wie RC_Common, 100 = Reed.
 * Der alte Wert 5 aus v0.14 (dort UROLE_REED in RC_Common 1.5.0) wird beim
 * Laden und an allen UI-Grenzen auf 100 umgerechnet.
 *
 * Ohne Arduino-Abhaengigkeit, laeuft auch in den Host-Tests (tests/).
 */

#include <stdint.h>
#include "src/RC_Common/uart_roles.h"

static constexpr uint8_t APP_UROLE_REED        = 100;
static constexpr uint8_t APP_UROLE_REED_LEGACY = 5;    // nur Migration v0.14
static constexpr uint8_t APP_UROLE_REED_INDEX  = 5;    // letzter Eintrag im Dropdown

// Name/Optionen inklusive Reed ("Aus;CRSF;S.Port;GPS;Raw;Reed").
const char* appRoleName(uint8_t role);
const char* appRoleOptions();
const char* appRoleValues();  // "0;1;2;3;4;100" passend zu appRoleOptions()
// 0..4 bleibt upstream-identisch, 5 (Legacy) und 100 -> Reed, alles andere -> Aus.
uint8_t     appRoleClamp(int raw);
// Reed braucht keine UART-Baudrate (0); sonst Delegation an RC_Common.
uint32_t    appRoleDefaultBaud(uint8_t role);
// App-Wert -> Index in appRoleOptions() (Reed -> 5), fuer Lua/Web-Dropdowns.
uint8_t     appRoleUiIndex(uint8_t role);

#endif // REED_ROLES_H
