/*
 * sound_store.cpp  -  Soundmodul Mini, siehe sound_store.h
 */

#include "sound_store.h"
#include <LittleFS.h>

static bool flashOk = false;

// LittleFS darf mehrere Dateien gleichzeitig offen halten: 3 Motor-Dateien
// + 4 Sounds + Upload passen ohne Freigabe-Trick hinein.
static constexpr uint8_t FLASH_MAX_FILES = 10;

void soundStoreBegin() {
    // formatOnFail=true: eine leere oder beschaedigte Partition (z. B. nach
    // dem ersten Flashen ohne uploadfs) wird einmal formatiert statt
    // dauerhaft "kein Speicher" zu melden.
    flashOk = LittleFS.begin(true, "/littlefs", FLASH_MAX_FILES);
    Serial.printf("Sound-Speicher: Flash %s\n", flashOk ? "ok" : "FEHLER");
}

fs::FS&     soundFS()         { return LittleFS; }
bool        soundStoreReady() { return flashOk; }
const char* soundStoreName()  { return "Flash"; }

uint64_t soundStoreTotalBytes() { return flashOk ? (uint64_t)LittleFS.totalBytes() : 0; }
uint64_t soundStoreUsedBytes()  { return flashOk ? (uint64_t)LittleFS.usedBytes()  : 0; }
