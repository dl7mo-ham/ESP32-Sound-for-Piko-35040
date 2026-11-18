/*
 * sound_playback.h  -  Soundmodul Mini, Piko-35040-Fork (projektlokal)
 *
 * I2SAudio.Play() setzt Playing=true, auch wenn keine Datei geladen werden
 * konnte (FileOK=false). NextSample() liefert dann nur Stille und setzt
 * Playing nie zurueck. Die Motor-State-Machine wartet in STARTING aber auf
 * !Sound_start.Playing und haengt sonst mit leerem Start-Platz dauerhaft fest.
 * soundPlaybackSanitize() raeumt genau dieses Artefakt auf.
 */

#ifndef SOUND_PLAYBACK_H
#define SOUND_PLAYBACK_H

// Kein Include von XT_I2S_Audio.h hier: die Datei hat keinen Include-Guard
// und wird von den Aufrufern bereits eingebunden. Nur vorwaerts deklarieren.
class XT_Wav_Class;

// true, wenn ein stummer Playlist-Eintrag entfernt wurde.
bool soundPlaybackSanitize(XT_Wav_Class* snd);

#endif
