/*
 * sound_trigger.h  -  Soundmodul Mini
 *
 * One-Shot-Trigger der Sound-Slots (v0.15 aus der .ino ausgelagert).
 */

#ifndef SOUND_TRIGGER_H
#define SOUND_TRIGGER_H

#include <Arduino.h>

// Kein Include von XT_I2S_Audio.h hier: die Datei hat keinen Include-Guard
// und wird vom Sketch bereits eingebunden. Nur vorwaerts deklarieren.
class XT_Wav_Class;

void handleSound(uint8_t idx, XT_Wav_Class* snd);

#endif
