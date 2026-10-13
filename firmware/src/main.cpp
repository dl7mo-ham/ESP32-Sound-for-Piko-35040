// Soundmodul Mini (S3) - Grundgeruest
// Folgeschritte: Code aus ESP32-RC-Sound v7.18 uebernehmen und kuerzen,
// CRSF-Bibliothek einbinden, Speicher Flash/SD, Pinout.

#include <Arduino.h>

static const char *FW_NAME = "Sound-Mini-S3";
static const char *FW_VERSION = "0.01";

void setup() {
  Serial.begin(115200);
  delay(500);
  Serial.printf("%s v%s\n", FW_NAME, FW_VERSION);
}

void loop() {
  delay(1000);
}
