/*
 * app_roles_bridge.cpp  -  Soundmodul Mini (siehe app_roles_bridge.h)
 *
 * Rein additiv zu RC_Common: keine Upstream-Datei wird geoeffnet oder
 * geaendert. Die Sonderfaelle der Reed-Rolle (100) liegen hier statt in der
 * .ino, damit ein spaeterer upstream-Merge möglichst wenige Konflikte hat.
 */

#include "app_roles_bridge.h"

#include <Arduino.h>
#include "config.h"
#include "pins.h"
#include "reed_roles.h"
#include "src/RC_Common/lua_menu.h"
#include "src/RC_Common/uart_ports.h"
#include "src/RC_Common/uart_roles.h"

// Feste Lua-Feld-IDs aus buildLuaMenu() (siehe static_assert in der .ino).
static constexpr uint8_t LUA_FI_PORTB_ROLE = 43;
static constexpr uint8_t LUA_FI_PORTC_ROLE = 45;

static LuaItem* findItem(LuaItem* items, uint8_t count, uint8_t id) {
    for (uint8_t i = 0; i < count; ++i) {
        if (items[i].id == id) return &items[i];
    }
    return nullptr;
}

void appRolesPatchLuaOptions(LuaItem* items, uint8_t count) {
    const char* opts = appRoleOptions();
    const uint8_t max = (uint8_t)(LuaMenu::optionCount(opts) - 1);
    const uint8_t ids[] = { LUA_FI_PORTB_ROLE, LUA_FI_PORTC_ROLE };
    for (uint8_t id : ids) {
        LuaItem* it = findItem(items, count, id);
        // Auf dem C3 sind die Port-C-Felder LUA_INFO (nicht auswaehlbar).
        if (!it || it->type != LUA_SELECT) continue;
        it->text = opts;
        it->max  = max;
    }
}

uint8_t appRolesUiRole(uint8_t appRole) {
    return appRoleUiIndex(appRoleClamp(appRole));
}

void appRolesApplyUiRole(uint8_t port, uint8_t& role, uint32_t& baud, uint8_t uiValue) {
    const uint8_t next = appRoleClamp(uiValue);   // Index 5 (und Legacy 5) -> 100
    if (next != role) {
        // Baudrate auf den Standard der Rolle vorbelegen (Aus/Raw: unveraendert)
        const uint32_t def = appRoleDefaultBaud(next);
        if (def != 0 && next != UROLE_RAW) baud = def;
    }
    role = next;
    markDirty();
    Serial.printf("!!! Port %c Rolle -> %s (wirkt erst nach Neustart) – NEUSTART ERFORDERLICH !!!\n",
                  port ? 'C' : 'B', appRoleName(next));
}

bool appRolesHideBaud(uint8_t appRole) {
    const uint8_t role = appRoleClamp(appRole);
    // Aus und Reed brauchen keine Baudrate; bei Reed setzt reedBegin() die Pins.
    return role == UROLE_OFF || role == APP_UROLE_REED;
}

void appRolesApplyUartRoles(UartPort* ports, uint8_t count,
                            uint8_t roleB, uint32_t baudB,
                            uint8_t roleC, uint32_t baudC) {
    if (count > 0) {
        ports[0].role = appRoleClamp(roleB) == APP_UROLE_REED ? UROLE_OFF : roleB;
        ports[0].baud = baudB;
    }
    if (count > 1) {
        ports[1].role = appRoleClamp(roleC) == APP_UROLE_REED ? UROLE_OFF : roleC;
        ports[1].baud = baudC;
    }
}

void appRolesLogPortABoot(uint8_t role) {
    Serial.printf("Port A: %s (RX=%d, TX=%d), kein CRSF-Eingang\n",
                  appRoleName(role), PIN_CRSF_RX, PIN_CRSF_TX);
}
