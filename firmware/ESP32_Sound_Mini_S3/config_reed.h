/*
 * config_reed.h  -  Soundmodul Mini
 *
 * Projektlokale Reed-Konfiguration: NVS (Debounce + Aktionen), JSON-Export
 * und POST-Felder. Die Port-Rollen (inkl. APP_UROLE_REED=100 und der
 * Migration 5 -> 100) bleiben bewusst in config.cpp, weil dort die
 * gemeinsamen RC_Common-Schluessel u1role/u2role mitgelesen werden muessen.
 */

#ifndef CONFIG_REED_H
#define CONFIG_REED_H

#include <Arduino.h>
#include <Preferences.h>
#include "config.h"
#include "reed_trigger.h"

// Entprellzeit auf den erlaubten Bereich begrenzen (5..500 ms).
uint16_t reedDebounceClamp(int raw);

// Gemeinsame Mini-JSON-Key-Suche fuer die Reed-REST-Endpunkte (Wortgrenze
// wie jsonFindKey() in WebServerManager.cpp, kein eigener JSON-Parser noetig).
bool reedJsonGetInt(const String& body, const char* key, int& out);

// Rueckgabe: true, wenn ein Wert korrigiert wurde (configDirty setzen).
bool reedConfigLoad(Preferences& p);
void reedConfigSave(Preferences& p);

// "reed":{...} fuer /api/config (ohne die oeffnende/abschliessende Klammer
// wird der Abschnitt komplett inkl. nachfolgendem Komma erzeugt).
void reedConfigAppendJson(String& j);

// Reed-Felder aus dem flachen /api/config-POST-Body uebernehmen.
// Rueckgabe: true, wenn mindestens ein Feld veraendert wurde.
bool reedConfigApplyPost(const String& body);

#endif
