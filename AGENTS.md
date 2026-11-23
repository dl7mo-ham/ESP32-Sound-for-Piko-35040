# AGENTS.md – Hinweise für KI-Agenten in diesem Repo

Dieses Repository ist eine **projektlokale Erweiterung** von
[`mnorden81/ESP32-Sound-Mini-S3`](https://github.com/mnorden81/ESP32-Sound-Mini-S3)
(`upstream`) für den Piko 35040 mit Reed-Trigger. `origin` ist unser eigener Fork.
Die Projektsprache ist **Deutsch**: Antworten, Kommentare und Dokumentation auf Deutsch halten.

## Wichtigste Regel: minimal-invasive Änderungen

Unsere Änderungen gegenüber `upstream/main` sollen **so klein wie möglich** und nach
Möglichkeit **vollständig in eigenen Dateien** liegen. Ziel: Bei einem Upstream-Sync
gibt es möglichst wenige oder keine Merge-Konflikte. Das gilt für jeden Commit, nicht nur
für große Refactorings.

Konkret:

- `firmware/ESP32_Sound_Mini_S3/src/RC_Common/` ist **upstream-identisch und tabu**.
  Nichts dort ändern – auch nicht „nebenbei“ (Version, Manifest, README, Tests,
  Rollen-Konstanten). Änderungen an der gemeinsamen Basis gehören upstream.
- Neue Funktionalität kommt in **neue, projektlokale Dateien** neben der Sketch-Datei
  (bewährt: `reed_*.*`, `*_bridge.*`, `command_queue.*`, `*_control.*`, `*_arbiter.*`).
  In Upstream-Dateien bleiben höchstens **dünne Aufrufstellen** (eine Zeile pro Stelle).
- In Dateien, die upstream gehören (`ESP32_Sound_Mini_S3.ino`, `WebServerManager.cpp`,
  `config.cpp`, …): **kein Umformatieren, Umbenennen, Umstrukturieren oder „Aufräumen“**
  von fremdem Code. Keine unwesentlichen Zeilen berühren. Diffs klein halten.
- **Additiv statt destruktiv:** neue NVS-Schlüssel, neue JSON-Felder mit Defaults,
  neue Optionen am Ende. Bestehende Export-/Backup-Formate müssen rückwärtskompatibel
  bleiben; alte Werte ggf. beim Laden migrieren (wie `5 → 100` bei Reed).
- Bestehende Schnittstellen nicht verändern: Lua-Feldnummern, Web-API-Parameter,
  NVS-Schlüssel und die RC_Common-Rollen `0…4` bleiben stabil.
  Reed ist ausschließlich die projektlokale Rolle `APP_UROLE_REED = 100` in
  `reed_roles.h`; das Mapping `Index 5 ↔ 100` gehört an die UI-Grenzen
  (`app_roles_bridge.*`), nie nach RC_Common.
- Vor jeder Änderung in einer Upstream-Datei kurz prüfen: *Lässt sich das in eine
  eigene Datei auslagern?* Wenn ja, das tun.
- Keine Git-Historie-Aktionen (Force-Push, Rebase, Reset, Branch-Löschen) ohne
  ausdrückliche Anweisung. `--force-with-lease` statt `--force`, wenn doch nötig.

Hintergrund und bisherige Umsetzung: [`docs/upstream-konfliktminimierung-plan.md`](docs/upstream-konfliktminimierung-plan.md).

## Struktur und Zuständigkeiten

| Pfad | Zweck |
|---|---|
| `firmware/ESP32_Sound_Mini_S3/ESP32_Sound_Mini_S3.ino` | Arduino-Sketch, `setup()`/`loop()`; nur dünne Aufrufe projektlokaler Module |
| `firmware/ESP32_Sound_Mini_S3/*.cpp/.h` | Projektlokale Erweiterungen (Reed, Queue, Web-Tab, Motor, LED, …) |
| `firmware/ESP32_Sound_Mini_S3/src/RC_Common/` | Upstream-Kopie 1.5.2, **nicht verändern** |
| `firmware/ESP32_Sound_Mini_S3/tests/` | Projektlokale Host-Tests hier ergänzen |
| `firmware/platformio.ini` | Build für `s3` (Default) und `c3` |
| `docs/` | Doku, Pläne, Bilder; `docs/audio-ablauf.md` bei Ablaufänderungen aktualisieren |

## Konventionen

- Fork-Version bei jeder Änderung erhöhen: `docs/fork-changelog.md` und die
  README-Fork-Sektion (`<upstream-Basis>+dl7mo.<Zähler>`). `CHANGELOG.md` bleibt
  upstream-identisch; das RC_Common-Versionsfeld ebenfalls **nur upstream**.
  Der CRSF-„Parameter Version Number“ geht weiter an den Sender, kommt aber aus
  `firmware/ESP32_Sound_Mini_S3/fork_version.h` (`APP_PARAM_VERSION = 200 +
  FORK_SEQ`) und nicht aus der `Version`-Zeile im Sketch. `FORK_SEQ` bei jeder
  Änderung und bei jedem Upstream-Merge erhöhen.
- Die eigene README-Sektion steht **über** der Zeile
  `*Ab hier folgt die unveränderte README des Upstream-Projekts.*` – den Teil darunter
  nicht antasten.
- Gleiche Sketch-Datei für S3 und C3; Board-unterschiede gehören nach `pins.h` bzw.
  in boardabhängige Zweige. Neue Features auf **beiden** Boards im Blick behalten.
- Keine SD-Karte, Sounds in LittleFS. C3: kein PSRAM, kein Port C.

## Prüfen vor Abschluss

```bash
# Host-Tests (projektlokal)
cd firmware/ESP32_Sound_Mini_S3/tests && ./run_tests.sh

# Upstream-Kopie muss unverändert und grün sein
cd firmware/ESP32_Sound_Mini_S3/src/RC_Common
sha256sum -c --quiet MANIFEST.sha256
cd tests && ./run_tests.sh

# Builds für beide Boards
cd firmware && pio run && pio run -e c3

# Konfliktpotenzial sichtbar machen
git diff --stat upstream/main...HEAD
```

Zuletzt prüfen, ob `docs/audio-ablauf.md`, `README.md` und `CHANGELOG.md` zur Änderung
passen. Ohne Hardware Zugriff Änderungen als „noch nicht auf Hardware getestet“ kennzeichnen.
