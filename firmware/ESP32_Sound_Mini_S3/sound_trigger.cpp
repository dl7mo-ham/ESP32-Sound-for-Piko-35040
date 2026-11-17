/*
 * sound_trigger.cpp  -  Soundmodul Mini (siehe sound_trigger.h)
 */

#include "sound_trigger.h"
#include "XT_I2S_Audio.h"   // kein Include-Guard, deshalb nur hier
#include "config.h"

// Zustand aus dem Sketch (ESP32_Sound_Mini_S3.ino).
extern bool Sound_on[RCSOUND_NUM_SLOTS];
extern bool Sound_play[RCSOUND_NUM_SLOTS];
extern bool Sound_on_web[RCSOUND_NUM_SLOTS];
extern bool testSoundActive[RCSOUND_NUM_SLOTS];
extern XT_I2S_Class I2SAudio;

void handleSound(uint8_t idx, XT_Wav_Class* snd) {
    if (!snd) return;
    // One-Shot-Trigger sofort verbrauchen - auch wenn der Sound bereits
    // laeuft. Sonst bleibt Sound_on_web[idx] true, der Slot haengt nach dem
    // Wiedergabe-Ende dauerhaft im "laeuft bereits"-Zweig und startet nie
    // wieder (siehe docs/audio-ablauf.md).
    bool webTrigger = Sound_on_web[idx];
    Sound_on_web[idx] = false;
    if (Sound_on[idx] || webTrigger) {
        if (!Sound_play[idx]) {
            Sound_play[idx] = true;
            snd->LoadWavFile();
            if (config.Mode_Sound[idx] == 1) snd->RepeatForever = true;
            I2SAudio.Play(snd, true, config.Volumen_Sound[idx]);
        } else {
            snd->Volume = config.Volumen_Sound[idx];
        }
    } else {
        snd->RepeatForever = false;
        // Lautstaerke auch waehrend des Nachspielens aktualisieren (der
        // One-Shot-Trigger wird oben bereits verbraucht).
        if (Sound_play[idx]) snd->Volume = config.Volumen_Sound[idx];
        if (config.Mode_Sound[idx] == 2 && Sound_play[idx]) I2SAudio.Stop(snd);
        // testSoundActive (Lua "Test Sound") zuruecksetzen, sobald die
        // Wiedergabe endet. Im Loop-Mode bleibt "Ein" bewusst stehen.
        if (!snd->Playing && Sound_play[idx]) {
            snd->UnLoadWavFile();
            Sound_play[idx] = false;
            testSoundActive[idx] = false;
        }
    }
}
