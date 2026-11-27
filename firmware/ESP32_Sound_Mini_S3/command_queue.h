#ifndef COMMAND_QUEUE_H
#define COMMAND_QUEUE_H

/*
 * command_queue.h  -  Soundmodul Mini
 *
 * Kleine FIFO (16 Eintraege) fuer Reed-Impulse und Web-Testbuttons. Die IDs
 * sind zugleich die Reed-Aktionen: enqueueCommand() legt nur ab,
 * processCommandQueue() (in loop()) arbeitet die Eintraege ab und ruft intern
 * executeCommand() auf. Damit laufen echte Trigger und Testbuttons durch
 * denselben Pfad.
 */

#include <stdint.h>

static constexpr uint8_t REED_ACTION_NONE          = 0;
static constexpr uint8_t REED_ACTION_PLAY_SOUND_1  = 1;
static constexpr uint8_t REED_ACTION_PLAY_SOUND_2  = 2;
static constexpr uint8_t REED_ACTION_PLAY_SOUND_3  = 3;
static constexpr uint8_t REED_ACTION_PLAY_SOUND_4  = 4;
static constexpr uint8_t REED_ACTION_TOGGLE_MOTOR  = 5;
static constexpr uint8_t REED_ACTION_TOGGLE_FLAME  = 6;
static constexpr uint8_t REED_ACTION_SPEED_UP      = 7;
static constexpr uint8_t REED_ACTION_SPEED_DOWN    = 8;
static constexpr uint8_t REED_ACTION_COUNT         = 9;

// Nur gueltige Aktionen werden aufgenommen (Queue voll -> verworfen).
void enqueueCommand(uint8_t cmd);
// Einmal pro loop() aufrufen; fuehrt alle wartenden Kommandos aus.
void processCommandQueue();

#endif // COMMAND_QUEUE_H
