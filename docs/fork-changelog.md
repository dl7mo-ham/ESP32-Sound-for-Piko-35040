# Fork-Changelog – Piko 35040 (dl7mo)

Versionsschema: `<upstream-Basis>+dl7mo.<Zähler>`. Der Zähler läuft über alle Fork-Änderungen fort, die Basis wechselt nur, wenn ein neuer `upstream/main`-Stand eingespielt wird. Die Upstream-Historie bleibt unverändert in [CHANGELOG.md](../CHANGELOG.md).

Die Versionszeichenfolge ist rein dokumentarisch: Sie passt nicht in `char versionString[6]`. Web und Lua zeigen deshalb weiter die Upstream-Basis (aktuell `0.13`). Der CRSF-„Parameter Version Number“ kommt projektlokal aus `firmware/ESP32_Sound_Mini_S3/fork_version.h` (`APP_PARAM_VERSION = 200 + FORK_SEQ`) und wird bei jeder Änderung sowie bei jedem Upstream-Merge erhöht.

## v0.15+dl7mo.11 (2026-11-27)

- **IR-Steuerung statt nur Serial-Monitor:** Der IR-Empfänger liegt jetzt an **IO11** (S3, `ir_monitor.h`), Arduino-IRremote 5.0.0 bleibt nur bei `env:s3`. NEC und NEC2 werden auf gelernte Aktionen geprüft; Repeat-Frames werden ignoriert. Andere Protokolle erscheinen weiterhin im Serial-Monitor.
- Neue projektlokale Dateien `ir_code.{h,cpp}` (protokollunabhängige Code-Darstellung, host-testbar), `ir_control.{h,cpp}` (NVS-Key `ircode`, Lernmodus mit 10 s Timeout, Zuordnung `Code → Aktion`) und die zugehörigen REST-Routen in `reed_web.cpp`: `POST /api/ir/learn`, `POST /api/ir/clear`, `GET /api/ir/status`.
- **Web-Tab „Reed & IR“ ist jetzt aktionszentriert:** Eine Zeile pro Aktion mit Radio-Buttons für die Reed-Pins (intern weiterhin `Pin → Aktion` in `reedact`) und dem gelernten IR-Code. „IR lernen“ startet den Lernmodus, die UI pollt den Status und zeigt den Code hex an. Bei Kollision gewinnt die neue Zuordnung; die alte wird gelöscht. Die gewählten Reed-Pins werden weiterhin über `/api/config` gespeichert.
- **Additiv/rückwärtskompatibel:** `/api/config` liefert ein neues Feld `"ir"` mit `available`, `timeout_ms` und `codes`; der POST importiert `ir_<n>_protocol/_address/_command`. Alte Backups ohne IR-Feld bleiben gültig, `src/RC_Common/` bleibt unverändert. In `config.cpp` und `WebServerManager.cpp` nur je zwei dünne Aufrufstellen, in der `.ino` nur ein Kommentar (IO12 → IO11).
- Die geplante Modellbahn-Konfiguration (Port A = Aus, Port B/C = Reed, Aktionen per Reed oder IR) wird damit unterstützt; Port A bleibt ohne CRSF und ohne Funk.
- Der Tab heißt jetzt **„Reed & IR“** (vorher „Reed“); die interne Tab-ID `reed` bleibt unverändert, damit bestehende Links und JS-Aufrufe weiter funktionieren. Das Verdrahtungsdiagramm [`verdrahtung_mit_piko35040.drawio`](hardware-bilder/verdrahtung_mit_piko35040.drawio) und das zugehörige PNG zeigen jetzt den IR-Empfänger (TSOP31238) an IO11.
- CRSF-Cache-Token jetzt 211. Host-Tests (5), RC_Common-Tests (55), Manifest-Prüfung sowie Builds für `s3` (96,9 % Flash) und `c3` (99,0 %) sind grün. **Auf Hardware getestet (ESP32-S3-Zero, 27.11.2026):** Port A ohne CRSF, Reed auf Port B/C, IR-Lernen an IO11 und die aktionszentrierte Web-UI funktionieren. Die C3-Variante bleibt weiterhin nicht auf Hardware getestet.

## v0.15+dl7mo.10 (2026-11-24)

- Neuer projektlokaler IR-Serial-Monitor `ir_monitor.{h,cpp}`: Arduino-IRremote 5.0.0 dekodiert am **ESP32-S3 an IO12** (IR-Empfänger OUT → IO12, VCC → 3V3, GND → GND). Jeder empfangene Frame erscheint im Serial-Monitor (115200 Baud) als `IR: Protocol=… Address=… Command=… Raw-Data=…`. Unbekannte Protokolle werden zusätzlich als Roh-Timings ausgegeben, damit sich der Sender trotzdem identifizieren lässt. Kein Eingriff in Sound-/Reed-Logik, keine neuen NVS-/JSON-Felder, `src/RC_Common/` unverändert.
- Der Pin steht bewusst im lokalen Modul statt in der upstream-identischen `pins.h`. In der `.ino` nur zwei dünne Aufrufstellen; `lib_deps` nur bei `env:s3` (C3: IO12 nicht am Header, Flash knapp, daher Stub). CRSF-Cache-Token jetzt 210.
- Host-Tests (3), RC_Common-Tests (55), Manifest-Prüfung sowie Builds für `s3` (96,3 % Flash) und `c3` (98,4 %) sind grün. **Noch nicht auf Hardware getestet.**

## v0.15+dl7mo.9 (2026-11-23)

- Upstream-Merge auf `v0.15` eingespielt (Funk/ESP-NOW v0.14, Prüfstand, Browser-Sound-Aufbereitung, v0.15-Mischer-Reparatur). `RC_Common` ist damit upstream-identisch bei **1.5.2** (neu: `funk_link.{h,cpp}`, CRSF-Router-Port D); das Manifest ist grün.
- Textliche Konflikte in `ESP32_Sound_Mini_S3.ino`, `WebServerManager.cpp`, `XT_I2S_Audio.cpp` und `config.h` aufgelöst. Alle projektlokalen Reed-/Queue-/Bridge-Dateien blieben unverändert; in Upstream-Dateien nur die nötigen Aufrufstellen ergänzt.
- **Reed bleibt rein lokal:** `FunkMode_boot` wird bei Port A = Reed/Aus auf 0 gesetzt, `funk.loop()` läuft nur bei aktivem Funk, die 16 CRSF-Kanäle kommen weiter aus `crsf_bridge.cpp`. Web-Tab „Reed“ und neues Funk-Tab koexistieren (`TABS` enthält beide). Die Status-LED berücksichtigt `funk.conflict()`/`funk.dupAddr()` nur bei aktiver CRSF.
- Im Mischer ist upstreams Begrenzer mit 16-Bit-Maske statt `ApplyCompressor()` übernommen; die beiden lokalen Lautstärke-Korrekturen (kein `Volume = 100` beim Laden, kein `SetVolume()` in `NextSample()`) bleiben erhalten.
- **Achtung Hardware:** Port A liegt beim S3 jetzt auf RX/TX = IO44/43 (`Serial0`), Port C auf IO5/6 (`Serial2`). Die Reed-Pins folgen den `pins.h`-Makros; vorhandene Verdrahtung muss umgesteckt werden. Web-Anzeige und Doku zeigen jetzt IO44/43.
- CRSF-Cache-Token jetzt 209. Host-Tests (3), RC_Common-Tests (55), `sha256sum -c` sowie Builds für `s3` und `c3` sind grün. Beim C3 ist der Flash mit 98,4 % allerdings sehr knapp.
- **Noch nicht auf Hardware getestet.**

## v0.13+dl7mo.8 (2026-11-18)

- Die sofortige Lautstärke-Übertragung der Web-Regler (`m-vol`, `s-vol`) steckte als Fork-Diff mitten in `WebServerManager.cpp`: zwei `onchange`-Attribute, `liveVol()` und ein geändertes `testSound()`. Sie liegt jetzt als `appLiveVol()` plus zwei `change`-Listener im projektlokalen `/reed_ui.js` (`reed_web.cpp`). Das Backend `/api/sound` mit `{id,vol}` bleibt unverändert; das Verhalten ist gleich.
- `WebServerManager.cpp` ist an diesen drei Stellen wieder upstream-identisch. Kein neues API, keine neue Route, kein Eingriff in `src/RC_Common/`; CRSF-Cache-Token jetzt 208.
- **Auf Hardware getestet (ESP32-S3-Zero, 18.11.2026):** Die sofortige Lautstärkeübertragung der Regler und der Testbutton funktionieren wie vor dem Umbau. Die C3-Variante bleibt weiterhin nicht auf Hardware getestet.

## v0.13+dl7mo.7 (2026-11-18)

- Web-Tab „Reed“ zeigt bei jeder Pin-Zeile jetzt auch die zugehörige GPIO-Nummer, z. B. `RX (IO5)` / `TX (IO6)` (S3 Port A). Die Nummern kommen aus der vorhandenen Pin-Tabelle in `reed_trigger.cpp` (`pins.h`) und werden additiv als `pins:{rx,tx}` je Port in `/api/config` geliefert; bestehende Export-/Backup-Formate bleiben unverändert.
- Keine Upstream-Datei berührt; CRSF-Cache-Token jetzt 207.
- **Auf Hardware getestet (ESP32-S3-Zero, 18.11.2026):** Die GPIO-Anzeige im Reed-Tab stimmt. Die C3-Variante bleibt weiterhin ungetestet.

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
