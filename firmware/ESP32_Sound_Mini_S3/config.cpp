/*
 * config.cpp  -  Soundmodul Mini
 *
 * Persistente Konfiguration im NVS (Preferences). Eigener Namespace
 * "soundmini", damit sich ein Modul, auf dem vorher die V7-Firmware lief,
 * nicht mit deren Werten (24 Slots, SBUS/PWM/Hardware-Varianten) mischt.
 */

#include "config.h"
#include "pins.h"   // v0.13: STATUS_LED_RGB_DEFAULT
#include <Preferences.h>
#include "src/RC_Common/settings_common.h"
#include "sound_map.h"
#include "reed_trigger.h"   // reedActionClamp(): einzige Definition liegt dort

ConfigData    config;
bool          configDirty   = false;
unsigned long configDirtyMs = 0;

static constexpr const char* NVS_NS = "soundmini";

uint8_t portaRoleClamp(int raw) {
    uint8_t role = appRoleClamp(raw);
    if (role == UROLE_OFF || role == UROLE_CRSF || role == APP_UROLE_REED) return role;
    return UROLE_CRSF;
}
const char* portaRoleOptions() { return "Aus;CRSF;Reed"; }
const char* portaRoleValues()  { return "0;1;100"; }

// Siehe isValidEinSource() in ESP32_Sound_Mini_S3.ino (Definition dort, damit dieselbe
// Bereichspruefung wie einTyp() verwendet wird).
extern bool isValidEinSource(int c);

void markDirty() { configDirty = true; configDirtyMs = millis(); }

void Reset_all() {
    for (int i = 0; i < RCSOUND_NUM_SLOTS; i++) {
        config.Source_Start_Sound[i] = 999;
        config.Volumen_Sound[i]      = 100;
        config.Mode_Sound[i]         = 0;
    }
    config.Source_Speed_Sound_0   = 999;
    config.throttle_mode          = 0;
    config.Min_Speed_Sound_0      = 100;
    config.Max_Speed_Sound_0      = 300;
    config.shutdowndelay          = 0;
    config.engine_on_toggle       = 0;
    config.throttle_ramp          = 20;
    config.throttle_dead_band     = 10;
    config.modul_adress           = 0;
    // Gruppe 1 defaultmaessig = modul_adress (0), Gruppe 2 deaktiviert (255).
    config.EK_Gruppen_Adresse[0]  = 0;
    for (int i = 1; i < RCSOUND_NUM_GROUPS; i++) config.EK_Gruppen_Adresse[i] = 255;
    config.sport_poll_id[0]       = 0xA1;   // Physical ID 0x02 (Werkseinstellung Sensor 1)
    config.sport_poll_id[1]       = 0x22;   // Physical ID 0x03 (Sensor 2 umprogrammiert)
    // Port A startet wie bisher als CRSF-Eingang. Port B und Port C starten
    // deaktiviert; PortB_Baud bekommt trotzdem einen sinnvollen Platzhalter
    // fuer ein spaeteres Umschalten auf CRSF.
    config.PortA_Role             = UROLE_CRSF;
    config.PortB_Role             = UROLE_OFF;
    config.PortB_Baud             = uartRoleDefaultBaud(UROLE_CRSF);
    config.PortC_Role             = UROLE_OFF;
    config.PortC_Baud             = uartRoleDefaultBaud(UROLE_GPS);
    config.ReedDebounceMs         = REED_DEBOUNCE_DEFAULT_MS;
    for (int p = 0; p < REED_PORTS; p++)
        for (int i = 0; i < REED_PINS; i++)
            config.ReedAction[p][i] = REED_ACTION_NONE;
    strncpy(config.WiFi_SSID,     "Sound-Mini",  sizeof(config.WiFi_SSID)     - 1);
    strncpy(config.WiFi_Password, "123456789",   sizeof(config.WiFi_Password) - 1);
    strncpy(config.WiFi_IP,       "192.168.1.1", sizeof(config.WiFi_IP)       - 1);
    strncpy(config.Device_Name,   "Sound-Mini",  sizeof(config.Device_Name)   - 1);
    config.WiFi_SSID[sizeof(config.WiFi_SSID)-1]         = '\0';
    config.WiFi_Password[sizeof(config.WiFi_Password)-1] = '\0';
    config.WiFi_IP[sizeof(config.WiFi_IP)-1]             = '\0';
    config.Device_Name[sizeof(config.Device_Name)-1]     = '\0';
    config.WifiAutoEnable      = false;
    config.WifiAutoTimeoutSec  = 60;
    config.WifiAlwaysOn        = true;
    config.LedRgb              = STATUS_LED_RGB_DEFAULT;   // v0.13
    for (int i = 0; i < SND_FILE_SLOTS; i++) soundMapDefaultName(i, config.Sound_File[i]);
    configDirty = true;
}

// ── Gemeinsame Einstellungen (RC_Common, settings_common.h) ──────────────
// WLAN, Modul-Adresse und die Rollen von Port B/C liegen unter den
// Schluesseln, die alle Module verwenden.
static void configToCommon(RcCommonSettings& s) {
    memset(&s, 0, sizeof(s));
    strncpy(s.ssid,    config.WiFi_SSID,     sizeof(s.ssid) - 1);
    strncpy(s.pass,    config.WiFi_Password, sizeof(s.pass) - 1);
    strncpy(s.ip,      config.WiFi_IP,       sizeof(s.ip) - 1);
    strncpy(s.devName, config.Device_Name,   sizeof(s.devName) - 1);
    s.moduleAddr  = config.modul_adress;
    s.wifiAuto    = config.WifiAutoEnable;
    s.wifiTimeout = config.WifiAutoTimeoutSec;
    s.wifiAlways  = config.WifiAlwaysOn;
    // RC_Common kennt nur 0..4. Reed (100) wird dort als "Aus" abgelegt;
    // der App-Wert steht in den eigenen Schluesseln approleb/approlec.
    s.role[0] = config.PortB_Role == APP_UROLE_REED ? UROLE_OFF : config.PortB_Role;
    s.baud[0] = config.PortB_Baud;
    s.role[1] = config.PortC_Role == APP_UROLE_REED ? UROLE_OFF : config.PortC_Role;
    s.baud[1] = config.PortC_Baud;
}

static void commonToConfig(const RcCommonSettings& s) {
    strncpy(config.WiFi_SSID,     s.ssid,    sizeof(config.WiFi_SSID) - 1);
    strncpy(config.WiFi_Password, s.pass,    sizeof(config.WiFi_Password) - 1);
    strncpy(config.WiFi_IP,       s.ip,      sizeof(config.WiFi_IP) - 1);
    strncpy(config.Device_Name,   s.devName, sizeof(config.Device_Name) - 1);
    config.WiFi_SSID[sizeof(config.WiFi_SSID)-1]         = '\0';
    config.WiFi_Password[sizeof(config.WiFi_Password)-1] = '\0';
    config.WiFi_IP[sizeof(config.WiFi_IP)-1]             = '\0';
    config.Device_Name[sizeof(config.Device_Name)-1]     = '\0';
    config.modul_adress       = s.moduleAddr;
    config.WifiAutoEnable     = s.wifiAuto;
    config.WifiAutoTimeoutSec = s.wifiTimeout;
    config.WifiAlwaysOn       = s.wifiAlways;
    config.PortB_Role         = s.role[0];
    config.PortB_Baud         = s.baud[0];
    config.PortC_Role         = s.role[1];
    config.PortC_Baud         = s.baud[1];
}

void loadConfig() {
    Reset_all(); configDirty=false;
    Preferences p; p.begin(NVS_NS,true);
    if (!p.isKey("sssArr")) { p.end(); configDirty=true; return; }
    char key[8];
    // Die drei Sound-Arrays liegen als je ein Blob im NVS (ein Flash-Commit
    // je Array statt einem pro Slot, siehe saveConfigForce()). Passt die
    // Groesse nicht (anderer Firmware-Stand), bleiben die Defaults stehen.
    if (p.getBytesLength("sssArr") == sizeof(config.Source_Start_Sound)
        && p.getBytesLength("vsArr") == sizeof(config.Volumen_Sound)
        && p.getBytesLength("msArr") == sizeof(config.Mode_Sound)) {
        p.getBytes("sssArr", config.Source_Start_Sound, sizeof(config.Source_Start_Sound));
        p.getBytes("vsArr",  config.Volumen_Sound,       sizeof(config.Volumen_Sound));
        p.getBytes("msArr",  config.Mode_Sound,          sizeof(config.Mode_Sound));
    } else {
        Serial.println("Sound-Slots: NVS-Blob passt nicht zu RCSOUND_NUM_SLOTS - Sounds auf Default.");
        configDirty = true;
    }
    config.Source_Speed_Sound_0   = p.getInt("spd",  999);
    config.throttle_mode          = p.getInt("thrm", 0);
    config.Min_Speed_Sound_0      = p.getInt("minsp",100);
    config.Max_Speed_Sound_0      = p.getInt("maxsp",300);
    config.shutdowndelay          = p.getInt("shdly",0);
    config.engine_on_toggle       = p.getInt("entog",0);
    config.throttle_ramp          = p.getInt("thrr", 20);
    config.throttle_dead_band     = p.getInt("thrdb",10);
    for (int i = 0; i < RCSOUND_NUM_GROUPS; i++) {
        snprintf(key,sizeof(key),"gadr%d",i);
        config.EK_Gruppen_Adresse[i] = p.getInt(key, (i==0) ? 0 : 255);
    }
    // v0.12: Sound-Zuordnung. Fehlt der Schluessel (Firmware bis v0.11),
    // bleiben die festen Namen loop.wav ... sound4.wav aus Reset_all().
    if (p.getBytesLength("sfiles") == sizeof(config.Sound_File)) {
        p.getBytes("sfiles", config.Sound_File, sizeof(config.Sound_File));
        for (int i = 0; i < SND_FILE_SLOTS; i++) {
            config.Sound_File[i][SND_FILE_LEN - 1] = '\0';
            if (config.Sound_File[i][0] && !soundNameOk(config.Sound_File[i], false))
                config.Sound_File[i][0] = '\0';
        }
    }
    config.LedRgb                 = p.getBool("ledrgb", STATUS_LED_RGB_DEFAULT);   // v0.13
    config.sport_poll_id[0]       = (uint8_t)p.getInt("spid0", 0xA1);
    config.sport_poll_id[1]       = (uint8_t)p.getInt("spid1", 0x22);
    // v0.14: Port A und Reed-Trigger. Fehlende Schluessel behalten die
    // Werkseinstellung (Port A = CRSF, keine Aktionen).
    config.PortA_Role             = portaRoleClamp(p.getInt("parole", UROLE_CRSF));
    // v0.15: Reed ist projektlokal (APP_UROLE_REED=100). Die rohen Werte der
    // gemeinsamen Schluessel werden VOR rcSettingsLoad() gelesen: dessen
    // uartRoleClamp() wuerde den alten Reed-Wert 5 (v0.14) zu OFF machen.
    int rawRoleB = p.getInt("u1role", config.PortB_Role);
    int rawRoleC = p.getInt("u2role", config.PortC_Role);
    uint8_t appRoleB = appRoleClamp(p.isKey("approleb") ? p.getInt("approleb", rawRoleB) : rawRoleB);
    uint8_t appRoleC = appRoleClamp(p.isKey("approlec") ? p.getInt("approlec", rawRoleC) : rawRoleC);
    bool roleMigrated = (rawRoleB == APP_UROLE_REED_LEGACY || rawRoleC == APP_UROLE_REED_LEGACY
                         || !p.isKey("approleb") || !p.isKey("approlec"));
    config.ReedDebounceMs         = (uint16_t)constrain(p.getInt("reeddb", REED_DEBOUNCE_DEFAULT_MS),
                                                        REED_DEBOUNCE_MIN_MS, REED_DEBOUNCE_MAX_MS);
    if (p.getBytesLength("reedact") == sizeof(config.ReedAction)) {
        p.getBytes("reedact", config.ReedAction, sizeof(config.ReedAction));
    }
    for (int pi = 0; pi < REED_PORTS; pi++) {
        for (int i = 0; i < REED_PINS; i++) {
            uint8_t a = reedActionClamp(config.ReedAction[pi][i]);
            if (a != config.ReedAction[pi][i]) {
                config.ReedAction[pi][i] = a;
                configDirty = true;
            }
        }
    }
    // WLAN, Modul-Adresse, Port B/C: gemeinsame Schluessel (RC_Common).
    // Fehlen sie, werden einmalig die alten (pbmode/pcmode/pcgps, pbbaud,
    // pcbaud, ...) umgerechnet und beim naechsten Speichern unter den neuen
    // abgelegt. Fehlende Schluessel behalten die Werkseinstellung.
    RcCommonSettings cs;
    configToCommon(cs);
    if (rcSettingsLoad(p, cs, nullptr)) {
        Serial.println("Config: gemeinsame Einstellungen (RC_Common) aus alten Schluesseln uebernommen.");
        configDirty = true;
    }
    p.end();
    commonToConfig(cs);
    // Die App-Rollen (inkl. 100) wieder herstellen - RC_Common liefert nur
    // den reduzierten Wert 0..4 zurueck. Alte Werte 5 sind jetzt migriert.
    config.PortB_Role = appRoleB;
    config.PortC_Role = appRoleC;
    if (roleMigrated) configDirty = true;
    // Ein ungueltiger Quellwert wird auf 999 (Deaktiviert) bereinigt.
    for (int i = 0; i < RCSOUND_NUM_SLOTS; i++) {
        if (!isValidEinSource(config.Source_Start_Sound[i])) {
            Serial.printf("Sound %d: ungueltiger Quellwert %d -> Deaktiviert (999).\n",
                          i, config.Source_Start_Sound[i]);
            config.Source_Start_Sound[i] = 999;
            configDirty = true;
        }
    }
    Serial.println("Config aus NVS geladen.");
}

void saveConfig() { if(!configDirty)return; saveConfigForce(); }

void saveConfigForce() {
    Preferences p; p.begin(NVS_NS,false);
    char key[8];
    p.putBytes("sssArr", config.Source_Start_Sound, sizeof(config.Source_Start_Sound));
    p.putBytes("vsArr",  config.Volumen_Sound,       sizeof(config.Volumen_Sound));
    p.putBytes("msArr",  config.Mode_Sound,          sizeof(config.Mode_Sound));
    p.putInt("spd",  config.Source_Speed_Sound_0);
    p.putInt("thrm", config.throttle_mode);
    p.putInt("minsp",config.Min_Speed_Sound_0);
    p.putInt("maxsp",config.Max_Speed_Sound_0);
    p.putInt("shdly",config.shutdowndelay);
    p.putInt("entog",config.engine_on_toggle);
    p.putInt("thrr", config.throttle_ramp);
    p.putInt("thrdb",config.throttle_dead_band);
    for (int i = 0; i < RCSOUND_NUM_GROUPS; i++) {
        snprintf(key,sizeof(key),"gadr%d",i);
        p.putInt(key, config.EK_Gruppen_Adresse[i]);
    }
    p.putBytes("sfiles", config.Sound_File, sizeof(config.Sound_File));
    p.putBool("ledrgb", config.LedRgb);   // v0.13
    p.putInt("spid0",(int)config.sport_poll_id[0]);
    p.putInt("spid1",(int)config.sport_poll_id[1]);
    // v0.14/v0.15: Port A, Reed-Trigger und die App-Rollen von Port B/C
    // (projektlokal, nicht in RC_Common). Reed wird dort als 100 gespeichert,
    // in den gemeinsamen Schluesseln u1role/u2role dagegen als UROLE_OFF.
    p.putInt("parole", (int)config.PortA_Role);
    p.putInt("approleb", (int)config.PortB_Role);
    p.putInt("approlec", (int)config.PortC_Role);
    p.putInt("reeddb", (int)config.ReedDebounceMs);
    p.putBytes("reedact", config.ReedAction, sizeof(config.ReedAction));
    // WLAN, Modul-Adresse, Port B/C unter den gemeinsamen Schluesseln
    RcCommonSettings cs;
    configToCommon(cs);
    rcSettingsSanitize(cs);
    rcSettingsSave(p, cs);
    p.end();
    configDirty=false;
    Serial.println("Config in NVS gespeichert.");
}

