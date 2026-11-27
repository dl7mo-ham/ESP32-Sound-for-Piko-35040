/*
 * motor_speed_control.cpp  -  Soundmodul Mini (siehe motor_speed_control.h)
 */

#include "motor_speed_control.h"
#include "config_reed.h"   // reedJsonGetInt()

static uint8_t speedStep       = MOTOR_SPEED_STEP_DEFAULT;
static uint8_t virtualThrottle = 0;      // fluechtig, kein NVS-Key

// Einzige Speed-Rampe: actualSpeed ist die bereits erreichte Wiedergabe in %.
// speedCap verhindert, dass ein Minus-Druck weiterbeschleunigt; er wird erst
// durch den naechsten Plus-Druck aufgehoben. Alles fluechtig, kein NVS-Key.
static constexpr float SPEED_CAP_NONE = 100000.0f;
static float          actualSpeed     = 0.0f;
static float          speedCap        = SPEED_CAP_NONE;
static unsigned long  speedLastMs     = 0;
static bool           engineWasOn     = false;

bool motorSpeedConfigLoad(Preferences& p) {
    int raw = p.getInt("vspstp", MOTOR_SPEED_STEP_DEFAULT);
    uint8_t clamped = motorSpeedStepClamp(raw);
    speedStep = clamped;
    return clamped != raw;
}

void motorSpeedConfigSave(Preferences& p) {
    p.putInt("vspstp", speedStep);
}

void motorSpeedConfigAppendJson(String& j) {
    j += "\"speed_step\":"     + String(speedStep)              + ",";
    j += "\"speed_step_min\":" + String(MOTOR_SPEED_STEP_MIN)   + ",";
    j += "\"speed_step_max\":" + String(MOTOR_SPEED_STEP_MAX)   + ",";
}

bool motorSpeedConfigApplyPost(const String& body) {
    int v;
    if (!reedJsonGetInt(body, "reed_speed_step", v)) return false;
    uint8_t next = motorSpeedStepClamp(v);
    if (next == speedStep) return false;
    speedStep = next;
    return true;
}

uint8_t motorSpeedStep() {
    return speedStep;
}

uint8_t motorSpeedVirtualThrottle() {
    return virtualThrottle;
}

static void motorSpeedAdjust(int direction) {
    virtualThrottle = motorSpeedAdvance(virtualThrottle, direction, speedStep);
    if (direction < 0) speedCap = actualSpeed;   // nie weiter beschleunigen
    else               speedCap = SPEED_CAP_NONE;
    Serial.printf("[Speed] Virtueller Gas %u %% (Schritt %u %%, Speed %.1f %%)\n",
                  (unsigned)virtualThrottle, (unsigned)speedStep, (double)actualSpeed);
}

void motorSpeedStepUp() {
    motorSpeedAdjust(+1);
}

void motorSpeedStepDown() {
    motorSpeedAdjust(-1);
}

float motorSpeedUpdate(int minPct, int maxPct, bool engineRunning,
                       uint8_t rampPctPerSec) {
    unsigned long now = millis();
    unsigned long dt  = speedLastMs ? (now - speedLastMs) : 0UL;
    speedLastMs = now;
    if (dt > 1000UL) dt = 1000UL;
    float step = (float)rampPctPerSec * (float)dt / 1000.0f;   // Prozentpunkte

    // Motor startet: von 0 an, Stopp: Ziel 0.
    if (engineRunning && !engineWasOn) {
        actualSpeed = 0.0f;
        speedCap    = SPEED_CAP_NONE;
    }
    // Ausschalten verwirft den gemerkten Gaswert - der naechste Start
    // beginnt damit wieder bei Drehzahl min statt beim letzten Fahrwert.
    virtualThrottle = motorSpeedResetOnStop(virtualThrottle, engineWasOn, engineRunning);
    engineWasOn = engineRunning;

    float target = engineRunning
                 ? motorSpeedPlaybackBase((float)virtualThrottle, minPct, maxPct)
                 : 0.0f;
    if (target > speedCap) target = speedCap;

    actualSpeed = motorSpeedRampStep(actualSpeed, target, step);
    if (!engineRunning && target == 0.0f && actualSpeed < 0.5f) actualSpeed = 0.0f;

    return actualSpeed;
}
