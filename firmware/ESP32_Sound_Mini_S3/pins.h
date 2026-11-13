#ifndef PINS_H
#define PINS_H

/*
 * v0.09: Zwei Boards, ein Code. Welcher Block gilt, entscheidet das in der
 * Arduino IDE gewaehlte Board automatisch:
 *   "ESP32S3 Dev Module"  -> ESP32-S3-Zero (Block unten, wie bisher)
 *   "ESP32C3 Dev Module"  -> ESP32-C3 Pro Mini (Block BOARD_C3_PRO_MINI)
 */
#if defined(CONFIG_IDF_TARGET_ESP32C3)
#define BOARD_C3_PRO_MINI 1
#define BOARD_NAME "ESP32-C3"
#define BOARD_SHORT "C3"

/*
 * ESP32-C3 Pro Mini (ESP32-C3, 4 MB Flash, kein PSRAM, ein Kern)
 *
 * Header von hinten gesehen, USB oben:
 *   links  (Versorgungsreihe): 5V, GND, 3V3, IO4, IO3, IO2, IO1, IO0
 *   rechts:                    IO5, IO6, IO7, IO8, IO9, IO10, IO20, IO21
 *
 * Der MAX98357A wird UMGEDREHT (Bauteilseite zum ESP) direkt auf die
 * Versorgungsreihe geloetet, IO0 bleibt frei:
 *   VIN-5V, GND-GND, SD offen (3V3 nicht geloetet), GAIN-IO4, DIN-IO3,
 *   BCLK-IO2, LRC-IO1
 *
 * Auswahlregeln fuer den ESP32-C3:
 *   - GPIO 2, 8, 9 sind Strapping-Pins und muessen beim Start HIGH sein.
 *     IO2 (BCLK) ist ein Eingang am MAX, der interne Pull-up reicht meist;
 *     sicher ist ein 10k-Widerstand IO2 -> 3V3.
 *   - GPIO 9 ist die BOOT-Taste (WLAN-Taste nach dem Start)
 *   - GPIO 18/19 sind der native USB-Anschluss (Konsole)
 *   - nur zwei Hardware-UARTs: Port A (UART0) und Port B (UART1).
 *     Port C gibt es auf dem C3 nicht.
 *   - keine SD-Karte (Sounds und Backups im Flash)
 */

// ── I2S-Verstaerker (MAX98357A) ──────────────────────────────────────
#define PIN_I2S_DOUT  3
#define PIN_I2S_BCLK  2    // Strapping-Pin, beim Start HIGH (10k nach 3V3 empfohlen)
#define PIN_I2S_LRC   1
#define PIN_MAX_GAIN  4    // offen = 9 dB

// ── Port A: CRSF (Hardware-UART0, Serial0) ───────────────────────────
// IO20/21 sind die RX/TX-Pins des C3. Das ROM-Bootlog auf IO21 (nur beim
// Start) wird von CRSF-Geraeten verworfen.
#define PIN_CRSF_RX   20
#define PIN_CRSF_TX   21

// ── Port B: Hardware-UART1 (Serial1), Rolle waehlbar ─────────────────
#define PIN_PORTB_RX  5
#define PIN_PORTB_TX  6

// ── Port C: gibt es auf dem C3 nicht (nur zwei UARTs) ────────────────
#define HAS_PORT_C    0
#define PIN_PORTC_RX  0xFF
#define PIN_PORTC_TX  0xFF

// ── BOOT-Taste (GPIO9, Pull-up auf dem Board) ────────────────────────
#define PIN_BOOT_BUTTON 9
#define WIFI_BUTTON_WINDOW_MS 3000

// ── Status-LED ───────────────────────────────────────────────────────
// VORLAEUFIG, bitte am Board pruefen: RGB-LED (WS2812) an GPIO8. Hat das
// Board nur eine einfache LED, STATUS_LED_WS2812 auf 0 setzen
// (STATUS_LED_ACTIVE_HIGH: 1 = LED leuchtet bei HIGH, 0 = bei LOW).
#define PIN_STATUS_LED         8
#define STATUS_LED_WS2812      1
#define STATUS_LED_ACTIVE_HIGH 0

#else  // ── ESP32-S3-Zero ─────────────────────────────────────────────
#define BOARD_NAME "ESP32-S3"
#define BOARD_SHORT "S3"
#define HAS_PORT_C 1
#define STATUS_LED_WS2812      1
#define STATUS_LED_ACTIVE_HIGH 1

/*
 * pins.h  -  Soundmodul Mini (ESP32-S3-Zero)
 *
 * Alle GPIOs an EINER Stelle. VORLAEUFIG: das endgueltige Pinout wird in
 * Schritt 5 (Board-Pinout) gegen das echte Board geprueft und in
 * docs/pinout.md festgehalten.
 *
 * Auswahlregeln fuer den ESP32-S3:
 *   - GPIO 26-37 sind beim S3FH4R2 durch Flash/PSRAM belegt (tabu)
 *   - GPIO 0, 45, 46 sind Strapping-Pins (gemieden); GPIO 0 ist die BOOT-
 *     Taste und wird erst nach dem Start als WLAN-Taste gelesen; GPIO 3 nur
 *     JTAG-Auswahl
 *   - GPIO 19/20 sind der native USB-Anschluss (Konsole)
 *   - GPIO 21 ist beim S3-Zero die RGB-LED
 *   - keine SD-Karte (Sounds und Backups im Flash), GPIO 9-13 sind frei
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

// ── BOOT-Taste des S3-Zero (GPIO0, Pull-up auf dem Board) ────────────
// WLAN-Taste: nach dem Einschalten innerhalb von WIFI_BUTTON_WINDOW_MS
// kurz druecken = WLAN an (nur wenn "WLAN dauerhaft an" AUS ist). Beim
// Einschalten NICHT gedrueckt halten, sonst startet der Flash-Modus.
// Seit v0.05 ersetzt sie die WLAN-Bruecke an GPIO9, GPIO9 ist frei.
#define PIN_BOOT_BUTTON 0
#define WIFI_BUTTON_WINDOW_MS 3000

// ── Status-LED: RGB-LED (WS2812) des S3-Zero ─────────────────────────
// Status von Port A (RC_Common, status_led.h)
#define PIN_STATUS_LED 21

// ── Port C: Hardware-UART0 (Serial0), Rolle waehlbar wie Port B ──────
// IO43/44 sind die TX/RX-Pins am Header (UART0 ist frei, die Konsole
// laeuft ueber USB). Das ROM-Bootlog auf IO43 (nur beim Start) wird von
// GPS und CRSF-Geraeten verworfen.
#define PIN_PORTC_RX  44
#define PIN_PORTC_TX  43

#endif // Board

#endif // PINS_H
