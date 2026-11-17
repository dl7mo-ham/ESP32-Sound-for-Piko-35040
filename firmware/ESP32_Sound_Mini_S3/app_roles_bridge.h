/*
 * app_roles_bridge.h  -  Soundmodul Mini (projektlokal, NICHT RC_Common)
 *
 * Bindet die projektlokale Reed-Rolle (APP_UROLE_REED = 100) an die drei
 * Stellen an, an denen upstream die .ino zustaendig ist:
 *   - Lua-Rollen-Dropdown (Optionen und Wertumsetzung 5 <-> 100)
 *   - uartPorts[] bei setup() (Reed wird dort als UROLE_OFF angemeldet)
 *   - Boot-Log fuer Port A ohne CRSF
 *
 * Damit steht die komplette Reed-Sonderlogik in einer lokalen Datei; die
 * .ino enthaelt nur noch duenne Aufrufe und bleibt gegenueber upstream
 * weitgehend unveraendert.
 */

#ifndef APP_ROLES_BRIDGE_H
#define APP_ROLES_BRIDGE_H

#include <stdint.h>

struct LuaItem;
struct UartPort;

// Optionen der Rollen-Dropdowns (Feld 43/45) auf sechs Eintraege inkl. Reed
// umstellen. Einmal nach buildLuaMenu() aufrufen. Felder, die auf dem Board
// nicht als Select existieren (C3: Port C), werden uebersprungen.
void appRolesPatchLuaOptions(LuaItem* items, uint8_t count);

// Wertumsetzung an der Lua-Grenze: App-Wert 100 <-> Dropdown-Index 5.
uint8_t appRolesUiRole(uint8_t appRole);
void    appRolesApplyUiRole(uint8_t port, uint8_t& role, uint32_t& baud, uint8_t uiValue);
bool    appRolesHideBaud(uint8_t appRole);

// App-Rollen auf die Werte abbilden, die RC_Common versteht (Reed -> Aus).
void appRolesApplyUartRoles(UartPort* ports, uint8_t count,
                            uint8_t roleB, uint32_t baudB,
                            uint8_t roleC, uint32_t baudC);

// Boot-Log fuer Port A ohne CRSF, mit dem App-Namen (inkl. "Reed").
void appRolesLogPortABoot(uint8_t role);

#endif // APP_ROLES_BRIDGE_H
