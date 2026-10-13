# Projektvorschlag: Soundmodul Mini (Stand 13.10.2026)

Nur Vorschlag, noch kein Repo und kein Code.

Grundlage: Sound-Firmware v7.18 (`mnorden81-ESP32-Sound-bidirectional_code`, `main` @ `4d0f1df`) und die lokale Notiz `Sound/Mini/README.md` (MAX98357, CRSF, Board C3 oder S3-Zero offen).

## 1. Umfang

| | Sound V7 | Mini |
|---|---|---|
| Eingang | SBUS-Familie, CRSF, PWM, GPIO-Pins | **nur CRSF** (Port A) |
| Schaltquellen | BUS-Kanal, PWM, Pin, Einkanal (WM), MKan, Dauer | BUS-Kanal Low/High, Einkanal (WM-Adresse), MKan (CRSF-Gruppenadressen), Dauer, Aus |
| Sounds | Motor + 24 | **Motor (Start/Loop/Stopp) + 4** |
| Speicher | SD-Karte | **Flash (LittleFS)**, SD optional |
| Port B (2. UART) | Aus / S.Port / CRSF / Hott / Raw | bleibt (Hott-Platzhalter fällt weg, bis es einen Treiber gibt) |
| Port C (GPS) | SoftwareSerial | bleibt |
| Hardware-Varianten V1/V2/V3 | ja | entfällt, ein Pinout je Board |
| Lua-Menü | Felder 1–181 | neu, ca. 50 Felder |
| Web-UI, OTA, WLAN-Failsafe | ja | schlank behalten (siehe Frage 2) |

Motorlogik (Gasrampe, Standgas-Verzögerung, Tonhöhe folgt Gas), Lautstärke und Modus je Sound (Normal/Loop/Tipp), S.Port-LiPo, GPS-Telemetrie und WM-Adressschema werden unverändert übernommen.

## 2. Hardware-Annahmen

| | ESP32-S3-Zero (S3FH4R2) | ESP32-C3 (C3FH4) |
|---|---|---|
| Flash / PSRAM | 4 MB / 2 MB | 4 MB / – |
| Hardware-UARTs | 3 → Port A, B und C (GPS) alle in Hardware | 2 → Port A + B, GPS per SoftwareSerial |
| SD + 2. UART gleichzeitig | ja | knapp an GPIOs |
| Konsole | USB nativ | USB nativ |

Empfehlung: **S3-Zero**. Alle drei Ports in Hardware, genug Pins für die optionale SD, PSRAM als Reserve. Der Code bleibt so geschrieben, dass ein C3-Build als zweites Environment möglich ist. Wichtig: Der C3 hat kein `Serial2`; der heutige Code nutzt fest `Serial2` für CRSF.

Verstärker MAX98357A (I2S, mono). Pinout wird nach der Boardwahl in `docs/` festgelegt.

## 3. Flash-Budget (4 MB)

Audio: 16 Bit mono, **22,05 kHz empfohlen** = 44 KB pro Sekunde (44,1 kHz = 88 KB/s).

Typischer Bedarf: Start 3 s, Loop 2 s, Stopp 2 s, 4 Sounds à 2 s = 15 s ≈ **0,7 MB** bei 22,05 kHz.

| Partitionierung | App | Sounds (LittleFS) | Laufzeit bei 22,05 kHz |
|---|---|---|---|
| A: mit OTA (2 × App) | 2 × 1,25 MB | ≈ 1,3 MB | ≈ 30 s |
| B: ohne OTA (1 × App) | 1,5 MB | ≈ 2,4 MB | ≈ 55 s |

Die heutige Firmware belegt ≈ 1,0 MB (77,7 % von 1,25 MB). Ohne SBUS, 20 Slots und Varianten wird die Mini-Firmware kleiner und passt auch mit WLAN und OTA sicher in Variante A. Empfehlung: **A**, B als Ausweichoption. Mit 8/16 MB Flash wächst nur die Sound-Partition.

## 4. Speicher: Flash oder SD

- Eine Einstellung `Speicherort`: **Flash** / **SD** / **Auto** (SD wenn Karte steckt, sonst Flash).
- Technisch: Der Sound-Player bekommt statt fest `SD` ein `fs::FS&`. SD und LittleFS liefern denselben Dateityp, die Abspiel- und Puffer-Logik (Doppelpuffer, Lese-Task) bleibt gleich.
- Dateinamen bleiben: `start.wav`, `loop.wav`, `shut.wav`, `sound1.wav` … `sound4.wav`.
- Sounds ins Flash bringen, zwei Wege:
  1. Upload über die Web-UI (wie heute auf SD, nur Ziel Flash).
  2. Per USB: Ordner `data/` im Projekt, `pio run -t uploadfs`.
- Später möglich: IMA-ADPCM (4:1 komprimiert) für längere Sounds im 4-MB-Flash. Nicht Teil der ersten Version.

## 5. Struktur

Neues Repo **`mnorden81/ESP32-Sound-Mini`**, lokal = `Sound/Mini` (Ordner gibt es schon, wird zum Clone, Inhalt bleibt).

```
Sound/Mini/
  README.md
  firmware/                  PlatformIO-Projekt
    platformio.ini           env s3zero (+ später c3), env native
    partitions_mini.csv
    src/                     main.cpp, audio, motor, sound_store, ports, lua_params, web
    lib/crsf_esp32/          Kopie der gemeinsamen CRSF-Bibliothek
    data/                    Sounds für uploadfs
    test/                    Host-Tests (Motorlogik, Feld-IDs)
  hardware/                  Schaltplan, PCB
  docs/                      Pinout, Changelog
  backups/                   nur lokal, per .git/info/exclude ausgeschlossen
  .github/workflows/ci.yml   Build + Host-Tests
```

Die CRSF-Bibliothek kommt aus dem Thread „gemeinsame Bibliothek“ und liegt, wie von dir gewünscht, als Kopie in `lib/` des Projekts.

## 6. Schritte

1. Repo anlegen, lokalen Ordner `Sound/Mini` damit verbinden, leeres PlatformIO-Gerüst mit grüner CI.
2. Code aus v7.18 übernehmen und kürzen: SBUS, PWM/Pin, Varianten V1/V2, Sounds 5–24, Hott-Platzhalter raus. Ziel: baut grün.
3. CRSF auf die gemeinsame Bibliothek umstellen (sobald sie steht).
4. Speicher-Abstraktion Flash/SD/Auto, Partitionstabelle, `data/`-Upload.
5. Board-Environment und Pinout (S3-Zero), CRSF auf beliebigem UART statt fest `Serial2`.
6. Neue Lua-Parametertabelle (Motor + 4 Sounds + Einstellungen) und schlanke Web-UI.
7. Hardwaretest durch dich; danach Version v1.00 und Backup.

Jeder Schritt als eigener PR, du testest und mergst.

## 7. Offene Fragen

1. **Board:** S3-Zero (empfohlen) / C3 / beide
2. **WLAN + Web-UI:** schlank behalten (empfohlen, für Sound-Upload und OTA) / weglassen (Sounds nur per USB)
3. **Repo:** neues Repo `ESP32-Sound-Mini` (empfohlen) / Ordner im Sound-Repo
4. **Partitionierung:** A mit OTA (empfohlen) / B ohne OTA
