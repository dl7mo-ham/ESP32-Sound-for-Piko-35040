/*
 * crsf_bridge.cpp  -  Soundmodul Mini
 *
 * Ausgelagert aus loop() in ESP32_Sound_Mini_S3.ino (v0.15). Die globale
 * CRSF-Instanz und die zugehoerigen Zustandsvariablen bleiben im Sketch;
 * hier steht nur die Verarbeitung, damit die .ino einen Aufruf statt
 * ~45 Zeilen CRSF-Code enthaelt. Der Sketch muss luaMenu nicht mehr static
 * deklarieren, damit diese Datei darauf zugreifen kann.
 */

#include "crsf_bridge.h"
#include <Arduino.h>
#include "config.h"
#include "fork_version.h"   // projektlokal: CRSF-Parameter-Version
#include "src/RC_Common/CRSF_ESP32/crsf_esp32.h"
#include "src/RC_Common/lua_menu.h"
#include "src/RC_Common/uart_ports.h"

// Zustand aus dem Sketch (siehe ESP32_Sound_Mini_S3.ino).
extern CRSF         crsf;
extern LuaMenu      luaMenu;
extern bool         crsfActive;
extern bool         BUS_OK;
extern uint16_t     channel_output[16];
void einkanalFunctionCRSF();   // bleibt im Sketch (MWset-Auswertung)

static constexpr unsigned long CRSF_TIMEOUT_MS = 2000;
static unsigned long lastCrsfPacket = 0;

static void checkCrsfTimeout() {
    if (millis() - lastCrsfPacket > CRSF_TIMEOUT_MS) BUS_OK = false;
}

// Ping-Antwort erst im eigenen Zeit-Slot (Wilhelm-Meier-Schema), damit sich
// mehrere Module auf dem Bus nicht ueberlappen. War vorher static inline in
// der .ino und wird nur hier benutzt.
static constexpr uint16_t CRSF_SLOT_MS = 5;
static inline uint16_t crsfSlotDelayMs() {
    return (uint16_t)((uint8_t)constrain(config.modul_adress, 0, 15) * 2) * CRSF_SLOT_MS;
}

void crsfLoopTasks() {
    if (!crsfActive) return;

    // Port A: Pakete lesen und die Modulkette routen.
    crsf.read_packets(0);
    crsf.route_packets();   // Modulkette Port A <-> Port B/C (nur bei Rolle CRSF aktiv)
    for (int i = 0; i < 16; i++) channel_output[i] = crsf.get_crfs_channels(i);
    // BUS_OK haengt daran, ob gerade ein gueltiger CRSF-Frame geparst
    // wurde (Zaehler validFrames), nicht an einem Kanalwert.
    static uint32_t lastValidFrameCount = 0;
    uint32_t vf = crsf.getValidFrames();
    if (vf != lastValidFrameCount) { lastValidFrameCount = vf; BUS_OK = true; lastCrsfPacket = millis(); }
    checkCrsfTimeout();

    // Alle 100 ms Zellspannungen (S.Port) und GPS als CRSF-Telemetrie,
    // kein Batterie-Frame (0x08) - sonst wuerde die RxBt des Empfaengers
    // ueberschrieben.
    uartPortsTelemetry(crsf);

    // Antworten auf CRSF-Ping, Parameter-Read/-Write und Kommandos.
    if (crsf.getDeviceInfoReplyPending() && (millis() - crsf.getPingTime() >= crsfSlotDelayMs())) {
        // Erst im eigenen Zeit-Slot auf den Broadcast-Ping antworten.
        crsf.setDeviceInfoReplyPending(false);
        char dn[24]; snprintf(dn, sizeof(dn), "%s@%d", config.Device_Name, config.modul_adress);
        // Projektlokale Fork-Version als "Parameter Version Number", damit
        // Konfiguratoren eine geaenderte Parameterstruktur neu einlesen.
        // Die Upstream-Variable Version bleibt unberuehrt (Merge-Ruhe).
        crsf.send_device_info(dn, luaMenu.maxId(), APP_PARAM_VERSION);
    }
    if (crsf.getDeviceReadReplyPending()) {
        uint8_t pi = crsf.getParamReadIndex();
        crsf.setDeviceReadReplyPending(false);
        luaMenu.send(crsf, pi);
    }
    if (crsf.getDeviceWriteReplyPending()) {
        uint8_t wi = crsf.getParamWriteIndex(), wv = crsf.getParamWriteValue();
        crsf.setDeviceWriteReplyPending(false);
        luaMenu.write(wi, wv); luaMenu.send(crsf, wi);
    }
    if (crsf.getDeviceCommandReplyPending()) {
        crsf.setDeviceCommandReplyPending(false);
        einkanalFunctionCRSF();
    }
}
