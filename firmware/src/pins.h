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
 *   - GPIO 0, 45, 46 sind Strapping-Pins (gemieden); GPIO 3 nur JTAG-Auswahl
 *   - GPIO 19/20 sind der native USB-Anschluss (Konsole)
 *   - GPIO 21 ist beim S3-Zero die RGB-LED
 *   - SD-Karte auf den Standard-SPI-Pins des S3 (FSPI: 10-13), damit
 *     SD.begin(SD_CS) ohne eigene SPI-Konfiguration funktioniert
 */

// Der MAX98357A-Breakout wird direkt auf die Versorgungsreihe des S3-Zero
// gesteckt/geloetet (Header-Reihe von oben: 5V, GND, 3V3, IO1..IO6):
//   VIN-5V, GND-GND, SD offen (3V3 nicht geloetet), GAIN-IO1, DIN-IO2,
//   BCLK-IO3, LRC-IO4
// Siehe docs/Hardware-Bauplan_Mini-S3.docx.

// ── I2S-Verstaerker (MAX98357A) ──────────────────────────────────────
#define PIN_I2S_DOUT  2
#define PIN_I2S_BCLK  3    // IO3 ist Strapping-Pin nur fuer JTAG-Auswahl (eFuse), unkritisch
#define PIN_I2S_LRC   4
// GAIN des MAX: Eingang ohne Pull (offen) = 9 dB, LOW = 12 dB, HIGH = 6 dB
#define PIN_MAX_GAIN  1

// ── Port A: CRSF (Hardware-UART, Serial2) ────────────────────────────
#define PIN_CRSF_RX   5
#define PIN_CRSF_TX   6

// ── Port B: Hardware-UART1 (Serial1), Rolle waehlbar ─────────────────
// S.Port-Rolle: TX -[1N4148 Anode->Kathode]-> S.Port-Signal <- RX
#define PIN_PORTB_RX  7
#define PIN_PORTB_TX  8

// ── WLAN-Bootpin (LOW beim Einschalten = AP-Modus) ───────────────────
#define PIN_WIFI      9

// ── SD-Karte (optional, SPI) ─────────────────────────────────────────
#define PIN_SD_CS     10
#define PIN_SD_MOSI   11
#define PIN_SD_CLK    12
#define PIN_SD_MISO   13

// ── Status-LED: RGB-LED (WS2812) des S3-Zero ─────────────────────────
// Status von Port A (RC_Common, status_led.h)
#define PIN_STATUS_LED 21

// ── Port C: Hardware-UART0 (Serial0), Rolle waehlbar wie Port B ──────
// IO43/44 sind die TX/RX-Pins am Header (UART0 ist frei, die Konsole
// laeuft ueber USB). Das ROM-Bootlog auf IO43 (nur beim Start) wird von
// GPS und CRSF-Geraeten verworfen.
#define PIN_PORTC_RX  44
#define PIN_PORTC_TX  43

#endif // PINS_H
