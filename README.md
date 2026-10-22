# Soundmodul Mini (S3)

Kompaktes Soundmodul für Motorgeräusche und Effekte auf Basis ESP32-S3-Zero (ESP32-S3FH4R2, 4 MB Flash, 2 MB PSRAM).
Abgeleitet von [ESP32-RC-Sound V7](https://github.com/mnorden81/mnorden81-ESP32-Sound-bidirectional_code), reduziert auf:

- **Eingang:** nur CRSF (Port A), busbasiert
- **Sounds:** Motor (Start/Loop/Stopp) + 4 Zusatzsounds
- **Speicher:** Flash (LittleFS), SD-Karte optional
- **Ports:** Port A (CRSF), Port B und Port C als Hardware-UART, Rolle je Port in Web-UI und Lua wählbar: Aus, CRSF (Modulkette), S.Port, GPS oder Raw
- **Status-LED:** RGB-LED des S3-Zero (GPIO 21) zeigt den CRSF-Link
- **Web-UI:** Sound-Upload und OTA-Update per WLAN
- **Verstärker:** MAX98357A (I2S)

**Aktuelle Version:** v0.03 (nicht auf Hardware getestet)

Gemeinsamer Code mit Multiswitch und Soundmodul V7 liegt in `firmware/src/RC_Common/` (identische Kopie, siehe README dort). Gebaut wird mit dem Arduino-ESP32-Core 3.0.7 (PlatformIO über pioarduino).

Projektbeschreibung: [`docs/Projektbeschreibung.docx`](docs/Projektbeschreibung.docx) · Projektplan: [`docs/projektplan.md`](docs/projektplan.md)

## Flash-Aufteilung (4 MB)

2 × 1,25 MB Firmware (OTA) + ca. 1,3 MB Sounds (≈ 30 s bei 22,05 kHz, 16 Bit mono).

## Sounds

Dateinamen: `start.wav`, `loop.wav`, `shut.wav`, `sound1.wav` … `sound4.wav` (16 Bit PCM, 22,05 kHz mono empfohlen).

Einstellung **Sound-Speicher** (Web-UI → Einstellungen oder Lua-Menü → Einstellungen), wirkt nach Neustart:

- **Auto** (Standard): SD-Karte, wenn eine steckt, sonst Flash
- **Flash**: interner Speicher (LittleFS), keine SD-Karte nötig
- **SD**: nur SD-Karte

Sounds ins Flash bringen:

1. Web-UI: Tab „Motor“ bzw. „Sounds“ → WAV hochladen (geht in den aktiven Speicher)
2. USB: Dateien nach `firmware/data/` legen, dann `pio run -t uploadfs`

Konfigurations-Backups liegen ab v0.04 im Flash (Weboberfläche, Karte „Konfiguration sichern“: im Modul sichern, wiederherstellen, löschen; automatisch vor jedem Import und Firmware-Update). `pio run -t uploadfs` überschreibt den ganzen Flash-Speicher und damit auch diese Backups.

## Bauen

```bash
cd firmware
pio run                 # Firmware bauen
pio run -t upload       # flashen
pio run -t uploadfs     # Sounds aus firmware/data/ ins Flash
```

## Hardware

Bauplan mit Bauteilliste, Pinbelegung und Grafiken: [`docs/Hardware-Bauplan_Mini-S3.docx`](docs/Hardware-Bauplan_Mini-S3.docx).
Die Pins kommen aus `firmware/src/pins.h` (ein KiCad-Schaltplan fehlt noch). Die SD-Karte ist in diesem Aufbau nicht vorgesehen.

Der MAX98357A wird direkt auf die Versorgungsreihe des S3-Zero gelötet (VIN-5V, GND-GND, SD-Pin nicht gelötet, 3V3 bleibt frei).

![MAX direkt am ESP](docs/hardware-bilder/max_direkt_S3.png)

![Verdrahtungsplan](docs/hardware-bilder/verdrahtung_S3.png)

| Funktion | GPIO |
|---|---|
| MAX98357A direkt auf 5V, GND, 3V3, IO1..IO4 | GAIN 1, DIN 2, BCLK 3, LRC 4 |
| Port A CRSF RX / TX | 5 / 6 |
| Port B RX / TX | 7 / 8 |
| WLAN-Brücke (nach GND) | 9 |
| Port C RX / TX | 44 / 43 |
| Status-LED (WS2812 auf dem S3-Zero) | 21 |

## Versionen

| Version | Änderung |
|---|---|
| 0.03 | Gemeinsame Basis **RC_Common 1.1.0**: Core 3.0.7, CRSF-Bibliothek 1.1.0 mit Router (Port B/C mit Rolle CRSF reichen Frames zwischen Port A und dem Port weiter), Rollen mit gemeinsamen Nummern (Aus, CRSF, S.Port, GPS, Raw; alte Werte werden beim ersten Start umgerechnet), gemeinsame NVS-Schlüssel, Lua-Menü als Tabelle mit unveränderten Feld-IDs, Status-LED, WLAN-Failsafe und OTA aus RC_Common. Web-API: `portb_role`/`portc_role` statt `portb_mode`/`portc_mode` (alte Sicherungen werden umgerechnet). **GPS-Baudrate „Auto“** (Vorgabe für GPS, wie Betaflight): Der Port sucht die Baudrate des GPS selbst, stellt es auf 115200 Baud und 10 Hz um (ATGM336H und u-blox) und sucht neu, wenn 3 s keine Daten kommen. Baudraten in der Weboberfläche als Auswahlliste mit „Auto“. |
| 0.02 | Port C mit Rolle und Baudrate wie Port B, Pins verschoben |

## Ordner

- `firmware/` PlatformIO-Projekt
- `hardware/` Schaltplan, PCB
- `docs/` Projektbeschreibung, Projektplan, später Pinout und Changelog
- `backups/` funktionierende Stände (nur lokal, nicht im Repo)

## Lizenz

GPL-3.0, wie das Ursprungsprojekt (enthält Code von XTronical).
