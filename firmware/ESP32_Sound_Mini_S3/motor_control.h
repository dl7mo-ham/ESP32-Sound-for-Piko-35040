/*
 * motor_control.h  -  Soundmodul Mini
 *
 * Motor-Zustand: direkte Steuerung (Web/Reed) und der CRSF-Toggle aus
 * loop(). Die Zustandsvariablen bleiben im Sketch; hier liegt nur die
 * Logik (v0.15 aus der .ino ausgelagert).
 */

#ifndef MOTOR_CONTROL_H
#define MOTOR_CONTROL_H

// Web-UI / Reed-Aktion: Motor ein-/ausschalten. Loescht zugleich den
// alten Legacy-One-Shot-Trigger Sound_on_web[0].
void setMotorSound(bool on);
void toggleMotorSound();
bool motorSoundIsOn();

// CRSF-Toggle (Nur-Ein-/Aus-Modus und Dauerbetrieb) aus loop(). Ruft nichts
// auf, wenn crsfActive == false.
void motorControlLoop();

#endif
