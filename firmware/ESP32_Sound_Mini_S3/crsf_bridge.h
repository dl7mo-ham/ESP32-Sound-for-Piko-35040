/*
 * crsf_bridge.h  -  Soundmodul Mini
 *
 * Die CRSF-Loop-Aufgaben (Pakete/Router/Telemetrie sowie Ping- und
 * Lua-Parameter-Replies) aus loop() der .ino. Der Aufrufer prueft
 * crsfActive; ohne aktiven CRSF-Bus darf keine CRSF-Methode benutzt werden.
 */

#ifndef CRSF_BRIDGE_H
#define CRSF_BRIDGE_H

void crsfLoopTasks();

#endif
