#ifndef MOTOR_SPEED_CONTROL_H
#define MOTOR_SPEED_CONTROL_H

/*
 * motor_speed_control.h  -  Soundmodul Mini (projektlokal)
 *
 * Virtueller Gaswert als Ersatz fuer den CRSF-Kanal, wenn kein
 * Speed-Kanal konfiguriert ist (Source_Speed_Sound_0 = 999). Die Aktionen
 * "Drehzahl +" und "Drehzahl -" erhoehen/verringern ihn in Schritten. Die
 * tatsaechliche Wiedergabegeschwindigkeit wird im Modul mit einer einzigen
 * Rampe auf den Wert aus Drehzahl min/max gefuehrt (auch beim Start/Stopp).
 *
 * Der Gaswert selbst ist fluechtig (0 nach Neustart). Nur die Schrittweite
 * wird in NVS gespeichert und im Web-Tab "Reed & IR" eingestellt.
 *
 * Die beiden helper funktionen am Anfang sind Arduino-frei und werden auch
 * von tests/test_motor_speed.cpp verwendet.
 */

#include <stdint.h>

static constexpr uint8_t MOTOR_SPEED_STEP_DEFAULT = 20;   // Prozent pro Tastendruck
static constexpr uint8_t MOTOR_SPEED_STEP_MIN     = 5;
static constexpr uint8_t MOTOR_SPEED_STEP_MAX     = 50;
static constexpr uint8_t MOTOR_THROTTLE_MAX       = 100;

inline uint8_t motorSpeedStepClamp(int raw) {
    if (raw < (int)MOTOR_SPEED_STEP_MIN) return MOTOR_SPEED_STEP_MIN;
    if (raw > (int)MOTOR_SPEED_STEP_MAX) return MOTOR_SPEED_STEP_MAX;
    return (uint8_t)raw;
}

inline uint8_t motorSpeedAdvance(uint8_t throttle, int direction, uint8_t step) {
    int v = (int)throttle + direction * (int)step;
    if (v < 0) return 0;
    if (v > (int)MOTOR_THROTTLE_MAX) return MOTOR_THROTTLE_MAX;
    return (uint8_t)v;
}

// Normale Fahr-Mapping: Gas 0 % -> Drehzahl min, Gas 100 % -> Drehzahl max.
// Host-testbar, ohne Arduino-Abhaengigkeiten.
inline float motorSpeedPlaybackBase(float throttle, int minPct, int maxPct) {
    float t = throttle < 0.0f ? 0.0f : (throttle > 100.0f ? 100.0f : throttle);
    if (minPct == maxPct) return (float)minPct;
    return (float)minPct + t * ((float)maxPct - (float)minPct) / 100.0f;
}

// Einzige Rampe: bewegt actual in Richtung target, nie darueber hinaus und
// niemals unter 0. Host-testbar, damit "+" immer erhoeht und "-" immer
// verringert - auch wenn eine Rampe noch hinter dem Sollwert hinterherhinkt.
inline float motorSpeedRampStep(float actual, float target, float step) {
    if (step < 0.0f) step = 0.0f;
    if (actual < target) {
        actual += step;
        if (actual > target) actual = target;
    } else {
        actual -= step;
        if (actual < target) actual = target;
    }
    return actual < 0.0f ? 0.0f : actual;
}

// Beim Ausschalten des Motors wird der gemerkte Gaswert verworfen, damit der
// naechste Start wieder bei Drehzahl min beginnt. Host-testbar.
inline uint8_t motorSpeedResetOnStop(uint8_t throttle, bool wasRunning, bool running) {
    return (wasRunning && !running) ? 0 : throttle;
}

#ifndef RC_COMMON_HOST_TEST

#include <Arduino.h>
#include <Preferences.h>

// NVS ("vspstp", Default 20) und /api/config-Felder im "reed"-Block.
bool motorSpeedConfigLoad(Preferences& p);          // true = korrigiert
void motorSpeedConfigSave(Preferences& p);
void motorSpeedConfigAppendJson(String& j);         // inkl. abschliessendem Komma
bool motorSpeedConfigApplyPost(const String& body); // "reed_speed_step"

uint8_t motorSpeedStep();
uint8_t motorSpeedVirtualThrottle();
void motorSpeedStepUp();
void motorSpeedStepDown();

// Einzige Speed-Rampe: fuehrt die tatsaechliche Wiedergabegeschwindigkeit
// (Ruckgabewert in %, 0..maxPct) auf das Ziel aus virtualThrottle und
// Drehzahl min/max. Beim Start bei 0, beim Stoppziel 0. Die Steilheit kommt
// aus config.throttle_ramp ("Motor Rampe").
float motorSpeedUpdate(int minPct, int maxPct, bool engineRunning,
                       uint8_t rampPctPerSec);

#endif // RC_COMMON_HOST_TEST

#endif // MOTOR_SPEED_CONTROL_H
