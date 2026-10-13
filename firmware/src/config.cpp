/*
 * config.cpp  -  Soundmodul Mini
 *
 * Persistente Konfiguration im NVS (Preferences). Eigener Namespace
 * "soundmini", damit sich ein Modul, auf dem vorher die V7-Firmware lief,
 * nicht mit deren Werten (24 Slots, SBUS/PWM/Hardware-Varianten) mischt.
 */

#include "config.h"
#include <Preferences.h>

ConfigData    config;
bool          configDirty   = false;
unsigned long configDirtyMs = 0;

static constexpr const char* NVS_NS = "soundmini";

// Siehe isValidEinSource() in main.cpp (Definition dort, damit dieselbe
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
    // Port B und Port C starten deaktiviert; PortB_Baud bekommt trotzdem einen
    // sinnvollen Platzhalter fuer ein spaeteres Umschalten auf CRSF.
    config.PortB_Mode             = PORTB_OFF;
    config.PortB_Baud             = PORTB_BAUD_DEFAULT_CRSF;
    config.PortC_GPS_Enabled      = 0;
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
    configDirty = true;
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
    config.modul_adress           = p.getInt("madr", 0);
    for (int i = 0; i < RCSOUND_NUM_GROUPS; i++) {
        snprintf(key,sizeof(key),"gadr%d",i);
        config.EK_Gruppen_Adresse[i] = p.getInt(key, (i==0) ? 0 : 255);
    }
    config.sport_poll_id[0]       = (uint8_t)p.getInt("spid0", 0xA1);
    config.sport_poll_id[1]       = (uint8_t)p.getInt("spid1", 0x22);
    config.PortB_Mode             = p.getInt("pbmode", PORTB_OFF);
    if (config.PortB_Mode < PORTB_OFF || config.PortB_Mode > PORTB_RAW) config.PortB_Mode = PORTB_OFF;
    config.PortB_Baud             = (uint32_t)p.getInt("pbbaud", (int)PORTB_BAUD_DEFAULT_CRSF);
    config.PortC_GPS_Enabled      = p.getInt("pcgps", 0) ? 1 : 0;
    String ssid=p.getString("ssid","Sound-Mini");
    String pass=p.getString("pass","123456789");
    String ip  =p.getString("wip", "192.168.1.1");
    String dnam=p.getString("dnam","Sound-Mini");
    bool     wifiAutoEnable  = p.getInt("wfauto", 0) ? true : false;
    uint16_t wifiAutoTimeout = (uint16_t)constrain(p.getInt("wftmo", 60), 5, 240);
    bool     wifiAlwaysOn    = p.getInt("wfalways", 1) ? true : false;
    p.end();
    strncpy(config.WiFi_SSID,    ssid.c_str(),sizeof(config.WiFi_SSID)-1);
    strncpy(config.WiFi_Password,pass.c_str(),sizeof(config.WiFi_Password)-1);
    strncpy(config.WiFi_IP,      ip.c_str(),  sizeof(config.WiFi_IP)-1);
    strncpy(config.Device_Name,  dnam.c_str(),sizeof(config.Device_Name)-1);
    config.WiFi_SSID[sizeof(config.WiFi_SSID)-1]='\0';
    config.WiFi_Password[sizeof(config.WiFi_Password)-1]='\0';
    config.WiFi_IP[sizeof(config.WiFi_IP)-1]='\0';
    config.Device_Name[sizeof(config.Device_Name)-1]='\0';
    config.WifiAutoEnable     = wifiAutoEnable;
    config.WifiAutoTimeoutSec = wifiAutoTimeout;
    config.WifiAlwaysOn       = wifiAlwaysOn;
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
    p.putInt("madr", config.modul_adress);
    for (int i = 0; i < RCSOUND_NUM_GROUPS; i++) {
        snprintf(key,sizeof(key),"gadr%d",i);
        p.putInt(key, config.EK_Gruppen_Adresse[i]);
    }
    p.putInt("spid0",(int)config.sport_poll_id[0]);
    p.putInt("spid1",(int)config.sport_poll_id[1]);
    p.putInt("pbmode",config.PortB_Mode);
    p.putInt("pbbaud",(int)config.PortB_Baud);
    p.putInt("pcgps", config.PortC_GPS_Enabled);
    p.putString("ssid",config.WiFi_SSID);
    p.putString("pass",config.WiFi_Password);
    p.putString("wip", config.WiFi_IP);
    p.putString("dnam",config.Device_Name);
    p.putInt("wfauto", config.WifiAutoEnable ? 1 : 0);
    p.putInt("wftmo",  (int)config.WifiAutoTimeoutSec);
    p.putInt("wfalways", config.WifiAlwaysOn ? 1 : 0);
    p.end();
    configDirty=false;
    Serial.println("Config in NVS gespeichert.");
}

// Eigener, kleiner Preferences-Zugriff fuer die Backup-Nummer - ein
// einzelner Int-Key, unabhaengig von der eigentlichen Konfiguration.
uint32_t nextSdBackupCounter() {
    Preferences p; p.begin(NVS_NS,false);
    uint32_t n = p.getUInt("sdbkupn", 0) + 1;
    p.putUInt("sdbkupn", n);
    p.end();
    return n;
}
