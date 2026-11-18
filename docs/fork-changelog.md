# Fork-Changelog – Piko 35040 (dl7mo)

Versionsschema: `<upstream-Basis>+dl7mo.<Zähler>`. Der Zähler läuft über alle Fork-Änderungen fort, die Basis wechselt nur, wenn ein neuer `upstream/main`-Stand eingespielt wird. Die Upstream-Historie bleibt unverändert in [CHANGELOG.md](../CHANGELOG.md).

Die Versionszeichenfolge ist rein dokumentarisch: Sie passt nicht in `char versionString[6]`. Web und Lua zeigen deshalb weiter die Upstream-Basis (aktuell `0.13`). Der CRSF-„Parameter Version Number“ kommt projektlokal aus `firmware/ESP32_Sound_Mini_S3/fork_version.h` (`APP_PARAM_VERSION = 200 + FORK_SEQ`) und wird bei jeder Änderung sowie bei jedem Upstream-Merge erhöht.

## v0.13+dl7mo.6 (2026-11-18)

- LED im Reed-Betrieb dunkel, solange die Flamme aus ist: `status_led_arbiter.cpp` prueft die Boot-Snapshot-Rollen und unterdrueckt die RC-Statusanzeige, wenn eine Portrolle `APP_UROLE_REED` ist. Damit gibt es kein „kein Signal“-Blinken (und kein blaues WLAN-Muster) mehr im Reed-Modus. `flameSet(false)` schreibt weiterhin sofort schwarz; die Flamme hat unveraendert Vorrang. Keine Upstream-Datei berührt; CRSF-Cache-Token jetzt 206.
- **Noch nicht auf Hardware getestet.**

## v0.13+dl7mo.5 (2026-11-18)

- Die Einstellung „LED Farbfolge“ war bisher nur im CRSF-Lua-Menü erreichbar – im Reed-Betrieb gibt es kein Lua-Menü. Neue Card „LED-Farbfolge (Status & Flamme)“ im Web-Tab „Reed“ (`reed_web.h`) mit Auswahl `RGB (S3-Zero)` / `GRB (Standard)`.
- Dafür projektlokale REST-Endpunkte `GET/POST /api/led-order` in `reed_web.cpp`; Schreiben setzt `config.LedRgb` und `markDirty()`. `updateLed()` und – bei aktiver Flamme – `flameUpdate()` übernehmen die Reihenfolge im nächsten Loop (kein Neustart nötig). Keine Upstream-Datei berührt; CRSF-Cache-Token jetzt 205.
- **Noch nicht auf Hardware getestet.**

## v0.13+dl7mo.4 (2026-11-18)

- Diagnose zur grünen Flamme: Der Grünanteil im Code war nicht die Ursache. Bei vertauschter WS2812-Farbfolge landet der rote Kanal im Grün der LED – weniger Grün im Code lässt die Flamme daher noch grüner erscheinen. Der ursprüngliche Mix (25 % Grün = warmes Orange) ist wiederhergestellt.
- Abhilfe ist die Lua-Einstellung „LED Farbfolge“ (`RGB (S3-Zero)` / `GRB`): Sie muss zur tatsächlich verbauten LED passen. Auch das bläuliche Blinken bei ausgeschalteter Flamme erklärt sich damit (vorgesehen ist violett bei WLAN, bzw. rot ohne Signal). Keine Upstream-Datei berührt; CRSF-Cache-Token jetzt 204.
- **Noch nicht auf Hardware getestet.**

## v0.13+dl7mo.3 (2026-11-18)

- Leere Sound-Plätze (`(leer)` oder fehlende Datei) lassen `I2SAudio.Play()` trotzdem `Playing = true` setzen; `NextSample()` bleibt stumm und räumt das Flag nie auf. Bei einem leeren **Start**-Platz blieb die Motor-State-Machine dadurch dauerhaft in `STARTING` – `loop.wav` startete nie. Bei leeren Sound-1–4-Plätzen blieben `Sound_play[]` und `testSoundActive[]` hängen.
- Neues projektlokales Modul `sound_playback.*` mit `soundPlaybackSanitize()`: Erkennt `Playing && !FileOK`, ruft `I2SAudio.Stop()` und entfernt damit auch den stummen Playlist-Eintrag. Aufruf in `motor_control.cpp` **vor** dem `crsfActive`-Return (wirkt also auch im Reed-Betrieb) und in `sound_trigger.cpp` für die One-Shots.
- Keine Upstream-Datei berührt: `.ino`, `XT_I2S_Audio.cpp`, `WebServerManager.cpp`, `config.cpp` und `src/RC_Common/` bleiben unverändert. CRSF-Cache-Token jetzt 203.
- Host-Tests (projektlokal und RC_Common), Manifest-Prüfung und Builds für S3/C3 grün. **Noch nicht auf Hardware getestet.**

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
