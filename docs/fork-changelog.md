# Fork-Changelog – Piko 35040 (dl7mo)

Versionsschema: `<upstream-Basis>+dl7mo.<Zähler>`. Der Zähler läuft über alle Fork-Änderungen fort, die Basis wechselt nur, wenn ein neuer `upstream/main`-Stand eingespielt wird. Die Upstream-Historie bleibt unverändert in [CHANGELOG.md](../CHANGELOG.md).

Die Versionszeichenfolge ist rein dokumentarisch: Sie passt nicht in `char versionString[6]`. Web und Lua zeigen deshalb weiter die Upstream-Basis (aktuell `0.13`). Der CRSF-„Parameter Version Number“ kommt projektlokal aus `firmware/ESP32_Sound_Mini_S3/fork_version.h` (`APP_PARAM_VERSION = 200 + FORK_SEQ`) und wird bei jeder Änderung sowie bei jedem Upstream-Merge erhöht.

## v0.13+dl7mo.2 (2026-11-18, bisher v0.15)

- Basis: upstream v0.13, `src/RC_Common/` wieder upstream-identisch (1.4.0). Die früheren Eingriffe (Rolle 5, Manifest, Version, Tests, README) sind daraus entfernt.
- Reed ist jetzt eine **projektlokale Rolle** `APP_UROLE_REED` = 100 (`reed_roles.h`). NVS-Migration: gespeicherte Werte 5 aus dl7mo.1 werden beim Laden auf 100 umgerechnet. Port B/C liegen in eigenen App-Schlüsseln (`approleb`/`approlec`); RC_Common sieht für Reed weiterhin `UROLE_OFF`.
- Lua/Web zeigen Reed als letzten Dropdown-Eintrag (Index 5 ↔ Wert 100), inklusive Ausblendung der Baudrate. Das Mapping steckt in `app_roles_bridge.*`; `buildLuaMenu()` ist wieder upstream-identisch.
- Die verbleibende Reed-Sonderlogik (CRSF-Ping, Timeout, Kommando-Queue, Motor, Sound-Trigger, LED-Arbitration) liegt in eigenen Dateien; die `.ino` enthält nur noch dünne Aufrufe.
- **CRSF entkoppelt:** `uint16_t Version` im Sketch ist wieder upstream-identisch (13). Der Cache-Token für Konfiguratoren steht jetzt in `fork_version.h` (aktuell 202). Damit berührt kein Versions-Update mehr eine Upstream-Datei.
- Reed-Tests in projektlokale Host-Tests ausgelagert (`firmware/ESP32_Sound_Mini_S3/tests/`). RC_Common-Hosttests, Manifest-Prüfung und Builds für S3/C3 grün. **Noch nicht auf Hardware getestet.**

## v0.13+dl7mo.1 (bisher v0.14, damals RC_Common 1.5.0)

- Reed-Trigger ist jetzt eine Port-Rolle statt einer Compile-Option: Port A (Default CRSF) ist um Aus/Reed erweiterbar, Port B/C zusätzlich um Reed. Alle Pins eines Reed-Ports laufen als Eingänge mit Pull-up, ohne UART; die Baudrate wird dafür ausgeblendet.
- Neuer Web-Tab „Reed“: je Reed-Port und Pin (RX/TX) Aktion wählen (Keine, Sound 1–4, Motor Ein/Aus, Flamme Ein/Aus) und per Testbutton auslösen. Entprellzeit 5–500 ms (Default 50 ms), gespeichert in NVS und im Konfigurations-Export/-Backup.
- Reed-Impulse und Web-Testbuttons legen ihre Aktion in eine gemeinsame 16-Eintrags-Kommando-Queue; Ausführung in `loop()` über einen zentralen Executor. Motor-Toggle und Flammen-LED nutzen dieselben Funktionen wie die übrige Steuerung; die Flamme überlagert die Status-LED, solange sie an ist.
- Rollenwechsel wirken wie bisher erst nach Neustart (Boot-Snapshot), Aktionen und Entprellzeit sofort. Das Lua-Menü zeigt die neue Rolle und blendet die Baudrate bei Reed aus; die Pin-Aktionen bleiben Web-only.
- Backup-/Export-Import: neue Felder additiv, alte Sicherungen bekommen Defaults. RC_Common-Hosttests und Builds für S3/C3 grün. **Noch nicht auf Hardware getestet.**
- Hinweis: Diese Version hatte RC_Common noch verändert (Rolle 5). In dl7mo.2 wurde das zugunsten der lokalen Rolle 100 zurückgenommen.
