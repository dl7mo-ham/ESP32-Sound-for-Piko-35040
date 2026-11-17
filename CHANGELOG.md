# Versionsgeschichte Soundmodul Mini (S3 und C3)

Die Version wird bei jeder Änderung erhöht, weil sie als CRSF „Parameter Version Number“ an den Sender geht. Ab v0.09 gilt jede Version für S3 und C3. Neueste Version oben.

## v0.15 (RC_Common 1.4.0, upstream-identisch)
- Reed ist jetzt eine **projektlokale Rolle** `APP_UROLE_REED` = 100 (`reed_roles.h`). `src/RC_Common/` wurde auf den upstream-Stand 1.4.0 zurückgesetzt; die bisherigen Änderungen (Rolle 5, Manifest, Version, Tests, README) sind daraus entfernt. Damit bleiben künftige Upstream-Merges weitgehend konfliktfrei.
- NVS-Migration: gespeicherte Rollenwerte 5 aus v0.14 werden beim Laden auf 100 umgerechnet. Port B/C liegen in eigenen App-Schlüsseln (`approleb`/`approlec`); RC_Common sieht für Reed weiterhin `UROLE_OFF`. Lua/Web zeigen Reed als letzten Dropdown-Eintrag (Index 5 ↔ Wert 100), inklusive Baudrate-Ausblendung.
- Reed-Tests in projektlokale Host-Tests ausgelagert (`firmware/ESP32_Sound_Mini_S3/tests/`); RC_Common-Hosttests, Manifest-Prüfung und Builds für S3/C3 grün. Noch nicht auf Hardware getestet.

## v0.14 (RC_Common 1.5.0)
- Reed-Trigger ist jetzt eine Port-Rolle statt einer Compile-Option: Port A (Default CRSF) ist um Aus/Reed erweiterbar, Port B/C zusätzlich um Reed. Alle Pins eines Reed-Ports laufen als Eingänge mit Pull-up, ohne UART; die Baudrate wird dafür ausgeblendet.
- Neuer Web-Tab „Reed“: je Reed-Port und Pin (RX/TX) Aktion wählen (Keine, Sound 1–4, Motor Ein/Aus, Flamme Ein/Aus) und per Testbutton auslösen. Entprellzeit 5–500 ms (Default 50 ms), gespeichert in NVS und in Konfigurations-Export/-Backup.
- Reed-Impulse und Web-Testbuttons legen ihre Aktion in eine gemeinsame 16-Eintrags-Kommando-Queue; Ausführung in `loop()` über einen zentralen Executor. Motor-Toggle und Flammen-LED nutzen dieselben Funktionen wie die übrige Steuerung; die Flamme überlagert die Status-LED, solange sie an ist.
- Rollenwechsel wirken wie bisher erst nach Neustart (Boot-Snapshot), Aktionen und Entprellzeit sofort. Das Lua-Menü zeigt die neue Rolle und blendet die Baudrate bei Reed aus; die Pin-Aktionen bleiben Web-only.
- Backup-/Export-Import: neue Felder additiv, alte Sicherungen bekommen Defaults. RC_Common-Hosttests und Builds für S3/C3 gruen. Noch nicht auf Hardware getestet.

## v0.13 (RC_Common 1.4.0)
- Status-LED wie alle Module: grün langsam = OK, rot schnell = Failsafe / kein RC-Link, rot Aufblitzen = kein Signal (vorher aus), blau Doppelblitz = Adresskonflikt (neu), gelb Doppelblitz = doppelte Port-Rolle, violett = WLAN an.
- Neues Lua-Feld 62 „LED Farbfolge“ (RGB/GRB, NVS `ledrgb`). BOOT-Taste im Betrieb 2 s halten = WLAN an/aus.
- Noch nicht auf Hardware getestet.

## v0.12
- Sound-Zuordnung: Sounds behalten beim Hochladen ihren Namen (`sound_map.h`). Neuer Web-Tab „Speicher“ mit Auswahlliste je Platz, Lua-Ordner „Sound-Zuordnung“ (Felder 58–61).
- Vorhandene `loop.wav` … `sound4.wav` bleiben auf ihren Plätzen, die Zuordnung steht im Export/Backup.
- Noch nicht auf Hardware getestet.

## v0.11 (RC_Common 1.3.0)
- Fix: Port-Baudraten kommen im Lua vollständig an (Chunk-Fix).
- Ordner zeigen ihren Wert, z. B. „Motor (L 3)“. Felder ohne Wirkung werden ausgeblendet, Feldnummern unverändert.

## v0.10
- Lua: „Kanal Nr“ und „Kanal Nr Speed“ bleiben immer Zahlenfelder (bei Aus/Dauer gesperrt). Vorher ließ sich der Kanal nach Wechsel der Quelle erst nach Verlassen des Menüs ändern.

## v0.09
- Zweites Board **ESP32-C3 Pro Mini** im selben Code (`pins.h` wählt nach dem Board in der Arduino IDE). C3: Port A auf UART0 (IO20/21), Port B auf IO5/6, kein Port C, Audio-Tasks ohne festen CPU-Kern. CI baut S3 und C3.

## v0.08
- Belegung des Sound-Speichers wird aus den Dateien berechnet (4-KB-Blöcke) statt mit `LittleFS.usedBytes()`, das dauerhaft „100 %“ zeigte. Web und Lua zeigen denselben Wert.

## v0.07
- Web-Tab „Debug“: WAV-Liste zeigt, ob die Datei im Flash liegt. RAM-Zeile eindeutig, neue Karte „Dateien im Flash“.

## v0.06
- Speicherbelegung (Firmware, Sounds, RAM, PSRAM) im Web-Tab „Debug“ und im Lua-Ordner „Speicher“ (Felder 53–57). Auf Hardware getestet (28.10.2026).

## v0.05
- WLAN-Taste statt WLAN-Brücke: BOOT-Taste (GPIO0) innerhalb von 3 s nach dem Einschalten drücken = WLAN an. GPIO9 und der Jumper entfallen.

## v0.04 (RC_Common 1.2.0)
- Einstellungs-Backup im Flash statt auf SD-Karte, automatisch vor Import und Firmware-Update. Keine SD-Karte mehr, Sounds nur im Flash.

## v0.03 (RC_Common 1.1.0)
- Gemeinsame Basis RC_Common: Core 3.0.7, CRSF-Bibliothek mit Router (Modulkette über Port B/C), gemeinsame Rollennummern (Aus, CRSF, S.Port, GPS, Raw), gemeinsame NVS-Schlüssel, Lua-Menü als Tabelle, Status-LED, WLAN-Failsafe und OTA.
- Web-API: `portb_role`/`portc_role` statt `portb_mode`/`portc_mode` (alte Sicherungen werden umgerechnet).
- GPS-Baudrate „Auto“ (sucht die Rate, stellt das GPS auf 115200 Baud / 10 Hz).

## v0.02
- Port C ist ein Hardware-UART mit Rolle und Baudrate wie Port B, Pins verschoben.

## v0.01
- Erste Version, abgeleitet von ESP32-RC-Sound v7.19: nur CRSF, Motorsound + 4 Zusatzsounds.
