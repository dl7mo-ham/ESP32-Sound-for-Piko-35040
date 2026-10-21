#ifndef CONFIG_H
#define CONFIG_H

/*
 * config.h  -  Soundmodul Mini
 *
 * Abgeleitet von ESP32-RC-Sound v7.19 (config.h), reduziert auf:
 *   - nur CRSF als RC-Bus (kein SBUS, kein PWM, keine Hardware-Varianten)
 *   - Motor + 4 Zusatzsounds
 *
 * Das Port-Modell bleibt:
 *   Port A = RC-Eingang CRSF (fest)
 *   Port B = Hardware-UART1, Port C = Hardware-UART0. Rolle je Port
 *            waehlbar (RC_Common, uart_roles.h): Aus, CRSF (Modulkette),
 *            S.Port-Master (LiPo-Telemetrie), GPS oder Raw-UART, je mit
 *            frei waehlbarer Baudrate. S.Port, GPS und Raw laufen nur auf
 *            einem Port (doppelt gewaehlt gewinnt Port B), CRSF auf beiden.
 * Pins siehe pins.h.
 */

#include <Arduino.h>

// Rollen fuer Port B und Port C kommen ab v0.03 aus RC_Common (RC_Common/
// uart_roles.h, gleiche Nummern in allen Modulen): UROLE_OFF, UROLE_CRSF,
// UROLE_SPORT, UROLE_GPS, UROLE_RAW. Alte NVS-Werte (bis v0.02: 3 = Raw,
// 4 = GPS) werden beim ersten Start einmalig umgerechnet.
#include "RC_Common/uart_roles.h"

// Motor(0) + 4 Zusatzsounds = 5 Slots.
static constexpr int RCSOUND_NUM_SOUNDS = 4;
static constexpr int RCSOUND_NUM_SLOTS  = RCSOUND_NUM_SOUNDS + 1;

// Anzahl der unabhaengig konfigurierbaren CRSF-Gruppenadressen fuer die
// 16 Einzelkanaele (Quelltyp "MKan" 80-95). Jede Gruppe liefert 8
// Kanaele. Adresse 255 = Gruppe deaktiviert.
static constexpr int RCSOUND_NUM_GROUPS = 2;

struct ConfigData {
    int Source_Start_Sound[RCSOUND_NUM_SLOTS];   // Quelle Motor(0) + Sound 1-4
    int Volumen_Sound[RCSOUND_NUM_SLOTS];        // Lautstaerke 0-200
    int Mode_Sound[RCSOUND_NUM_SLOTS];           // 0=Normal, 1=Loop, 2=Tippbetrieb

    int Source_Speed_Sound_0;    // Quelle Motorgeschwindigkeit (CRSF-Kanal 0-15, 999=aus)
    int throttle_mode;           // 0=Eine Richtung, 1=Zwei Richtungen
    int Min_Speed_Sound_0;       // Mindestgeschwindigkeit Motor (%)
    int Max_Speed_Sound_0;       // Maximalgeschwindigkeit Motor (%)
    int shutdowndelay;           // Motor aus Standgas Verzoegerung (s)
    int engine_on_toggle;        // 0=Normal, 1=Toggle
    int throttle_ramp;           // Gas-Rampe (%/s)
    int throttle_dead_band;      // Standgas-Totband (%)

    int modul_adress;            // Modul-Adresse (WM) - eigene CRSF-Busadresse 0xC0+Adresse,
                                 // zugleich Adresse fuer die Einkanal-Schalter (Quelltyp 70-77)

    // CRSF-Gruppenadressen fuer die Einzelkanaele (Quelltyp "MKan",
    // 80-95). Unabhaengig von modul_adress, 255 = Gruppe deaktiviert.
    int EK_Gruppen_Adresse[RCSOUND_NUM_GROUPS];

    // Port-Modell - siehe Erklaerung am Dateianfang.
    uint8_t  PortB_Role;        // UROLE_* (uart_roles.h) - Default UROLE_OFF
    uint32_t PortB_Baud;        // Baudrate Port B, frei waehlbar (Vorbelegung je nach Rolle)
    uint8_t  PortC_Role;        // Rolle Port C, gleiche Werte - Default UROLE_OFF
    uint32_t PortC_Baud;        // Baudrate Port C

    // Sound-Speicher 0=Auto, 1=Flash, 2=SD (siehe sound_store.h), wirkt
    // erst nach einem Neustart.
    int      Sound_Storage;

    uint8_t sport_poll_id[2];    // S.Port Physical Poll-ID Sensor 1+2 (z.B. 0xA1, 0x22)

    char WiFi_SSID[32];
    char WiFi_Password[64];
    char WiFi_IP[16];
    char Device_Name[24];

    // WLAN automatisch aktivieren, wenn ueber die eingestellte Zeit kein
    // gueltiges RC-Signal anliegt (siehe wifiFailsafeCheck() in main.cpp).
    // Default AUS. Der manuelle "WLAN jetzt"-Schalter (Web/Lua) wird
    // bewusst nicht persistiert, siehe WebServerManager::enableAP().
    bool     WifiAutoEnable;
    uint16_t WifiAutoTimeoutSec;   // 5-240s, Default 60

    // WLAN dauerhaft aktiv: der AP wird in setup() fest eingeschaltet und
    // bleibt an; Bootpin, manueller Schalter und Auto-Failsafe werden dann
    // uebersprungen. Default AN.
    bool     WifiAlwaysOn;
};

extern ConfigData    config;
extern bool          configDirty;
extern unsigned long configDirtyMs;

void loadConfig();
void saveConfig();
void saveConfigForce();
void markDirty();
void Reset_all();
// Fortlaufende, ueber Neustarts hinweg persistente Nummer fuer
// "Konfiguration auf SD sichern" (siehe WebServerManager.cpp).
uint32_t nextSdBackupCounter();

#endif
