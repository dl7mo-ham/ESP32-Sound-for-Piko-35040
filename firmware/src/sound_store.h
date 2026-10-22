#ifndef SOUND_STORE_H
#define SOUND_STORE_H

/*
 * sound_store.h  -  Soundmodul Mini
 *
 * Waehlt beim Start, woher die Sound-Dateien kommen:
 *   Flash = interne LittleFS-Partition (Partition "spiffs" aus default.csv)
 *   SD    = SD-Karte (SPI, Pins siehe pins.h)
 *   Auto  = SD, wenn eine Karte steckt, sonst Flash
 * Abspielen, Upload, Loeschen und Dateistatus laufen ueber soundFS(), die
 * Dateinamen sind auf beiden Speichern gleich (/loop.wav, /sound1.wav ...).
 * Ein Wechsel der Einstellung wirkt erst nach einem Neustart.
 * Konfigurations-Backups liegen seit v0.04 im Flash (RC_Common
 * config_backup, Ordner /backup in derselben LittleFS-Partition).
 */

#include <Arduino.h>
#include <FS.h>

enum SoundStorage : uint8_t {
    STORAGE_AUTO  = 0,
    STORAGE_FLASH = 1,
    STORAGE_SD    = 2
};
static constexpr uint8_t STORAGE_MAX = STORAGE_SD;
// Reihenfolge == SoundStorage-Wert, fuer Web-UI und CRSF/Lua-Menue.
static constexpr const char* STORAGE_OPTIONS = "Auto;Flash;SD";

// Mountet Flash und SD (soweit vorhanden) und legt den aktiven Speicher
// fest. Einmal in setup() aufrufen, vor dem ersten Laden einer WAV-Datei.
void soundStoreBegin(uint8_t wanted);

fs::FS&     soundFS();            // aktiver Speicher fuer die Sound-Dateien
bool        soundStoreIsSd();     // true = Sounds kommen von der SD-Karte
bool        soundStoreReady();    // false = gewaehlter Speicher nicht verfuegbar
const char* soundStoreName();     // "Flash" oder "SD"
uint64_t    soundStoreTotalBytes();
uint64_t    soundStoreUsedBytes();
bool        sdCardMounted();      // SD-Karte erkannt (auch wenn Sounds aus dem Flash kommen)

#endif
