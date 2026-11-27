#ifndef IR_CONTROL_H
#define IR_CONTROL_H

/*
 * ir_control.h  -  Soundmodul Mini (projektlokal)
 *
 * IR-Aktionen (NEC/NEC2) auf dem festen Pin aus ir_monitor.h. Ein gelernter
 * Code wird pro Aktion gespeichert und bei Empfang ueber enqueueCommand()
 * in dieselbe Queue wie Reed-Impulse und Web-Tests gelegt.
 *
 * Der Decoder selbst bleibt in ir_monitor.cpp (dort liegt IRremote.hpp);
 * diese Datei enthaelt nur Zustand, NVS, JSON und den Lernmodus.
 */

#include <Arduino.h>
#include <Preferences.h>
#include "command_queue.h"
#include "ir_code.h"

// Zeitfenster fuer "IR-Command festlegen" (siehe Web-Tab Reed).
static constexpr uint32_t IR_LEARN_TIMEOUT_MS = 10000;

// Nur der S3 hat den IR-Eingang am Header; der C3 bleibt Stub (siehe
// ir_monitor.cpp). Die Web-UI blendet die Lernbuttons entsprechend aus.
#if defined(BOARD_C3_PRO_MINI)
static constexpr uint8_t IR_AVAILABLE = 0;
#else
static constexpr uint8_t IR_AVAILABLE = 1;
#endif

// NVS/JSON (Aufruf aus config.cpp bzw. WebServerManager.cpp).
bool irConfigLoad(Preferences& p);          // true = Wert korrigiert -> configDirty
void irConfigSave(Preferences& p);
void irConfigAppendJson(String& j);         // "ir":{...}, inkl. abschliessendem Komma
bool irConfigApplyPost(const String& body); // ir_<n>_protocol/_address/_command

const IrCode& irCodeForAction(uint8_t action);
uint8_t irFindAction(const IrCode& code);   // 0 = kein Treffer
void irClearCode(uint8_t action);           // Web-Button "Löschen"

// Lernmodus (REST in reed_web.cpp, Tick in ir_monitor.cpp).
void irLearnStart(uint8_t action);
void irLearnCancel();
void irLearnTick();                          // Timeout pruefen
bool irLearnActive();
uint8_t irLearnAction();
uint32_t irLearnRemainingMs();

// Ergebnis des letzten Lernvorgangs fuer die UI (bleibt bis zum naechsten
// Lernen stehen, damit ein Poll nach dem Timeout noch loaderfolg meldet).
bool irLearnResultPending();
uint8_t irLearnResultAction();
IrCode irLearnResultCode();

// Von ir_monitor.cpp fuer jeden NEC/NEC2-Frame ohne Repeat gerufen.
// true = Frame wurde verarbeitet (dann kein Serial-Monitor-Log noetig).
bool irHandleNec(uint8_t protocol, uint16_t address, uint8_t command);

#endif // IR_CONTROL_H
