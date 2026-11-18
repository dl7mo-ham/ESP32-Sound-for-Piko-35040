/*
 * sound_playback.cpp  -  Soundmodul Mini (siehe sound_playback.h)
 */

#include "sound_playback.h"
#include "XT_I2S_Audio.h"   // kein Include-Guard, deshalb nur hier

extern XT_I2S_Class I2SAudio;

bool soundPlaybackSanitize(XT_Wav_Class* snd) {
    if (!snd || !snd->Playing || snd->FileOK) return false;
    // Stop() setzt Playing=false und entfernt den stummen Eintrag aus der
    // Playliste (unter Playlist-Mutex). Ohne das Entfernen wuerde MixSamples()
    // bei RepeatForever=true jeden Sample neu initialisieren.
    I2SAudio.Stop(snd);
    return true;
}
