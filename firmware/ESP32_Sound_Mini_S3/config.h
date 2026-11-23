#ifndef CONFIG_H
#define CONFIG_H

/*
 * config.h  -  Soundmodul Mini
 *
 * Abgeleitet von ESP32-RC-Sound v7.19 (config.h), reduziert auf:
 *   - nur CRSF als RC-Bus (kein SBUS, kein PWM, keine Hardware-Varianten)
 *   - Motor + 4 Zusatzsounds
 *
 * Port-Modell:
 *   Port A = Hardware-UART, Default CRSF. Seit v0.14 in der Web-UI
 *            umschaltbar auf Aus oder Reed (beide Pins als Tastereingaenge).
 *   Port B = Hardware-UART1, Port C = Hardware-UART0. Rolle je Port
 *            waehlbar (RC_Common, uart_roles.h): Aus, CRSF (Modulkette),
 *            S.Port-Master (LiPo-Telemetrie), GPS, Raw-UART oder Reed, je
 *            mit frei waehlbarer Baudrate. S.Port, GPS und Raw laufen nur
 *            auf einem Port (doppelt gewaehlt gewinnt Port B), CRSF auf
 *            beiden, Reed auf beliebig vielen.
 * Pins siehe pins.h.
 */

#include <Arduino.h>

// Rollen fuer Port B und Port C kommen ab v0.03 aus RC_Common (RC_Common/
// uart_roles.h): UROLE_OFF..UROLE_RAW (0..4). Reed ist seit v0.15 eine
// projektlokale Rolle (APP_UROLE_REED = 100, reed_roles.h); RC_Common bleibt
// upstream-identisch. Alte NVS-Werte (bis v0.02: 3 = Raw, 4 = GPS, und
// v0.14: 5 = Reed) werden beim Laden umgerechnet.
#include "src/RC_Common/uart_roles.h"
#include "reed_roles.h"
#include "command_queue.h"   // REED_ACTION_*: IDs der Reed-Aktionen/Queue

// v0.14: Reed-Trigger. Pro Port (A/B/C) und Pin (RX/TX) eine Aktion. Die IDs
// stehen jetzt in command_queue.h (zugleich Kommando-IDs der Queue).
static constexpr int REED_PORTS = 3;
static constexpr int REED_PINS  = 2;

static constexpr int REED_DEBOUNCE_MIN_MS = 5;
static constexpr int REED_DEBOUNCE_MAX_MS = 500;
static constexpr int REED_DEBOUNCE_DEFAULT_MS = 50;

// Port A kennt nur Aus/CRSF/Reed. Ungueltige Werte (z.B. aus einer alten
// Sicherung) fallen auf CRSF zurueck, damit der RC-Eingang wie bisher laeuft.
uint8_t     portaRoleClamp(int raw);
const char* portaRoleOptions();   // "Aus;CRSF;Reed"
const char* portaRoleValues();    // "0;1;100" (Werte fuer die Web-UI)

#include "src/RC_Common/funk_link.h"   // v0.14: FunkConfig

// Motor(0) + 4 Zusatzsounds = 5 Slots.
static constexpr int RCSOUND_NUM_SOUNDS = 4;
static constexpr int RCSOUND_NUM_SLOTS  = RCSOUND_NUM_SOUNDS + 1;

// Anzahl der unabhaengig konfigurierbaren CRSF-Gruppenadressen fuer die
// 16 Einzelkanaele (Quelltyp "MKan" 80-95). Jede Gruppe liefert 8
// Kanaele. Adresse 255 = Gruppe deaktiviert.
static constexpr int RCSOUND_NUM_GROUPS = 2;

// v0.12: Sound-Zuordnung (sound_map.h). Jede Datei behaelt ihren Namen, pro
// Platz steht hier, welche Datei dort spielt: 0 = Loop, 1 = Standgas,
// 2 = Start, 3.. = Sound 1-4. Leerer Name = Platz leer.
static constexpr int    SND_FILE_SLOTS    = 3 + RCSOUND_NUM_SOUNDS;
static constexpr size_t SND_NAME_MAX_BASE = 26;   // Flash: Name ohne ".wav"
static constexpr size_t SND_FILE_LEN      = SND_NAME_MAX_BASE + 4 + 2;   // + ".wav" + '\0' (32)

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
    uint8_t  PortA_Role;        // UROLE_OFF/UROLE_CRSF/APP_UROLE_REED - Default CRSF
    uint8_t  PortB_Role;        // UROLE_* (0..4) oder APP_UROLE_REED - Default OFF
    uint32_t PortB_Baud;        // Baudrate Port B, frei waehlbar (Vorbelegung je nach Rolle)
    uint8_t  PortC_Role;        // Rolle Port C, gleiche Werte - Default UROLE_OFF
    uint32_t PortC_Baud;        // Baudrate Port C

    // v0.14: Reed-Trigger. ReedAction[port][0=RX,1=TX] = REED_ACTION_*,
    // ReedDebounceMs = Entprellzeit in ms (5-500). Aenderungen wirken sofort,
    // die Port-Rolle dagegen erst nach einem Neustart.
    uint16_t ReedDebounceMs;
    uint8_t  ReedAction[REED_PORTS][REED_PINS];

    uint8_t sport_poll_id[2];    // S.Port Physical Poll-ID Sensor 1+2 (z.B. 0xA1, 0x22)

    char WiFi_SSID[32];
    char WiFi_Password[64];
    char WiFi_IP[16];
    char Device_Name[24];

    // WLAN automatisch aktivieren, wenn ueber die eingestellte Zeit kein
    // gueltiges RC-Signal anliegt (siehe wifiFailsafeCheck() in ESP32_Sound_Mini_S3.ino).
    // Default AUS. Der manuelle "WLAN jetzt"-Schalter (Web/Lua) wird
    // bewusst nicht persistiert, siehe WebServerManager::enableAP().
    bool     WifiAutoEnable;
    uint16_t WifiAutoTimeoutSec;   // 5-240s, Default 60

    // WLAN dauerhaft aktiv: der AP wird in setup() fest eingeschaltet und
    // bleibt an; BOOT-Taste, manueller Schalter und Auto-Failsafe werden dann
    // uebersprungen. Default AN.
    bool     WifiAlwaysOn;

    // v0.13: Farbfolge der Status-LED: true = RGB (S3-Zero), false = GRB
    bool     LedRgb;

    // v0.14: Funk (ESP-NOW, RC_Common funk_link). FunkMode 0 = Kabel,
    // 1 = Kabel + Funk (Zentrale), 2 = Funk (Teilnehmer). Wirkt nach Neustart.
    uint8_t    FunkMode;
    FunkConfig Funk;             // Kennung, Kanal, Leistung, Rate, Rueckkanal

    // v0.12: Dateiname je Platz (ohne "/"), siehe SND_FILE_SLOTS oben.
    // Werkseinstellung: loop.wav, shut.wav, start.wav, sound1.wav ...
    char Sound_File[SND_FILE_SLOTS][SND_FILE_LEN];
};

extern ConfigData    config;
extern bool          configDirty;
extern unsigned long configDirtyMs;

void loadConfig();
void saveConfig();
void saveConfigForce();
void markDirty();
void Reset_all();

#endif
