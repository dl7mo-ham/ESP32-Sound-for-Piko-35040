/*
 * command_queue.cpp  -  Soundmodul Mini
 * Siehe command_queue.h. Der Executor setzt dieselben Flags wie die
 * uebrigen Trigger (One-Shot-Sound, Motor-Toggle, Flammen-Toggle).
 */

#include "command_queue.h"
#include "config.h"
#include "flame_led.h"
#include "motor_speed_control.h"

// Zustand des Hauptsketchs (bis zur Auslagerung der Motor-/Soundlogik).
extern bool Sound_on_web[RCSOUND_NUM_SLOTS];
void toggleMotorSound();

static constexpr uint8_t CMD_QUEUE_SIZE = 16;
static uint8_t cmdQueue[CMD_QUEUE_SIZE];
static uint8_t cmdHead = 0, cmdTail = 0;

void enqueueCommand(uint8_t cmd) {
    if (cmd == REED_ACTION_NONE || cmd >= REED_ACTION_COUNT) return;
    uint8_t next = (uint8_t)((cmdHead + 1) % CMD_QUEUE_SIZE);
    if (next == cmdTail) {
        Serial.printf("[Cmd] Queue voll, Kommando %u verworfen\n", (unsigned)cmd);
        return;
    }
    cmdQueue[cmdHead] = cmd;
    cmdHead = next;
}

// Interner Executor eines Eintrags - einziger Aufrufer: processCommandQueue().
static void executeCommand(uint8_t cmd) {
    switch (cmd) {
        case REED_ACTION_PLAY_SOUND_1:
        case REED_ACTION_PLAY_SOUND_2:
        case REED_ACTION_PLAY_SOUND_3:
        case REED_ACTION_PLAY_SOUND_4:
            Sound_on_web[cmd] = true;   // ID 1-4 == Sound-Slot 1-4
            break;
        case REED_ACTION_TOGGLE_MOTOR: toggleMotorSound();  break;
        case REED_ACTION_TOGGLE_FLAME: flameToggle();       break;
        case REED_ACTION_SPEED_UP:     motorSpeedStepUp();   break;
        case REED_ACTION_SPEED_DOWN:   motorSpeedStepDown(); break;
        default: break;
    }
}

void processCommandQueue() {
    while (cmdTail != cmdHead) {
        uint8_t cmd = cmdQueue[cmdTail];
        cmdTail = (uint8_t)((cmdTail + 1) % CMD_QUEUE_SIZE);
        executeCommand(cmd);
    }
}
