#ifndef PINS_H
#define PINS_H

/*
 * pins.h  -  Soundmodul Mini (ESP32-S3-Zero)
 *
 * Alle GPIOs an EINER Stelle. VORLAEUFIG: das endgueltige Pinout wird in
 * Schritt 5 (Board-Pinout) gegen das echte Board geprueft und in
 * docs/pinout.md festgehalten.
 *
 * Auswahlregeln fuer den ESP32-S3:
 *   - GPIO 26-37 sind beim S3FH4R2 durch Flash/PSRAM belegt (tabu)
 *   - GPIO 0, 3, 45, 46 sind Strapping-Pins (gemieden)
 *   - GPIO 19/20 sind der native USB-Anschluss (Konsole)
 *   - GPIO 21 ist beim S3-Zero die RGB-LED
 *   - SD-Karte auf den Standard-SPI-Pins des S3 (FSPI: 10-13), damit
 *     SD.begin(SD_CS) ohne eigene SPI-Konfiguration funktioniert
 */

// ── WLAN-Bootpin (LOW beim Einschalten = AP-Modus) ───────────────────
#define PIN_WIFI      4

// ── Port A: CRSF (Hardware-UART, Serial2) ────────────────────────────
#define PIN_CRSF_RX   5
#define PIN_CRSF_TX   6

// ── I2S-Verstaerker (MAX98357A) ──────────────────────────────────────
#define PIN_I2S_BCLK  7
#define PIN_I2S_LRC   8
#define PIN_I2S_DOUT  9

// ── SD-Karte (optional, SPI) ─────────────────────────────────────────
#define PIN_SD_CS     10
#define PIN_SD_MOSI   11
#define PIN_SD_CLK    12
#define PIN_SD_MISO   13

// ── Port B: zweiter Hardware-UART (Serial1) ──────────────────────────
// S.Port-Rolle: TX -[1N4148 Anode->Kathode]-> S.Port-Signal <- RX
#define PIN_PORTB_RX  1
#define PIN_PORTB_TX  2

// ── Port C: GPS (vorerst SoftwareSerial wie in V7) ───────────────────
#define PIN_GPS_RX    14   // GPS-TX -> ESP32
#define PIN_GPS_TX    15   // ESP32 -> GPS-RX

#endif // PINS_H
