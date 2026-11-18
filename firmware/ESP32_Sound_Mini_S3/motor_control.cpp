/*
 * motor_control.cpp  -  Soundmodul Mini (siehe motor_control.h)
 */

#include "motor_control.h"
#include "config.h"
#include "sound_playback.h"

// Zustand aus dem Sketch (ESP32_Sound_Mini_S3.ino).
extern XT_Wav_Class Sound_start;
extern bool Sound_on[RCSOUND_NUM_SLOTS];
extern bool Sound_on_web[RCSOUND_NUM_SLOTS];
extern bool Sound_on_Motor;
extern bool Sound_on_Motor_state;
extern bool crsfActive;

void setMotorSound(bool on) {
    Sound_on_Motor = on;
    Sound_on_web[0] = false;   // alten Legacy-Trigger nicht dagegenlaufen lassen
    Serial.printf("[Motor] Direktsteuerung -> %s\n", on ? "EIN (Start->Loop)" : "AUS (Shut)");
}

void toggleMotorSound() {
    setMotorSound(!Sound_on_Motor);
}

bool motorSoundIsOn() {
    return Sound_on_Motor;
}

void motorControlLoop() {
    // Muss VOR dem CRSF-Return stehen: mit Port A = Reed gibt es keinen
    // CRSF-Bus, aber auch dort darf ein leerer Start-Platz STARTING nicht
    // blockieren (dann bliebe loop.wav dauerhaft aus).
    soundPlaybackSanitize(&Sound_start);

    if (!crsfActive) return;
    // Direktsteuerung (Web/Reed) bleibt erhalten, solange kein CRSF-Motorsignal
    // anliegt. Mit CRSF folgt Sound_on_Motor den Kanaelen.
    if (Sound_on[0] && !Sound_on_Motor_state && config.engine_on_toggle) {
        Sound_on_Motor = !Sound_on_Motor;
        Sound_on_Motor_state = true;
    } else if (!Sound_on[0] && Sound_on_Motor_state && config.engine_on_toggle) {
        Sound_on_Motor_state = false;
    } else if (!config.engine_on_toggle) {
        Sound_on_Motor = Sound_on[0];
    }
}
