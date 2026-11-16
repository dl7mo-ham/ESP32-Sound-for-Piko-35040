# Plan: Reed-Trigger-Modus pro Port (A/B/C) mit konfigurierbaren Pin-Aktionen

## Ziel
Für jeden Port (A, B, C) soll in der Web-UI ein **Reed-Trigger-Modus** aktivierbar sein (alternativ zu CRSF, S.Port, GPS, Raw/Aus). Ist ein Port im Reed-Trigger-Modus, werden **beide IO-Pins des Ports als digitale Eingänge mit Pull-up** betrieben statt als UART.

Zusätzlich erhält die Web-UI einen neuen Tab, in dem pro Pin jedes Reed-Trigger-Ports eine Aktion konfiguriert werden kann und jede Aktion manuell testbar ist. Ein erkannter Reed-Impuls löst die konfigurierte Aktion aus, indem ein Kommando **auf dem gleichen Weg wie aktuell CRSF-Befehle** in die bestehende Befehls-/Trigger-Verarbeitung eingereiht wird.

---

## Ausgangslage (dieser Branch)

- Branch: `feature/reed-trigger-ports`, erstellt von `upstream/main` (neuester Stand: v0.13, RC_Common 1.4.0, Sound-Zuordnung v0.12).
- Unsere Dokumentation (Bilder/Texte) aus `main` wurde übernommen (`README.md`, `docs/audio-ablauf.md`, `docs/hardware-bilder/verdrahtung_mit_piko35040.*`, Anpassungen in `docs/`).
- **Lautstärke-Bugfix** aus `main` (`7bd74a7`, „Lautstärkeeinstellung in Web-UI und Firmware korrigiert") wurde nach Möglichkeit übernommen (Cherry-Pick auf `upstream/main`):
  - `XT_I2S_Audio.cpp`: Volume nicht beim Laden überschreiben, Lautstärke zentral in `MixSamples()` anwenden, Sample-Mix korrekt zusammensetzen.
  - `.ino`: `I2S.Play(..., true, volume)` für Start/Loop/Shut und Zusatzsounds, Laufzeit-Volume bei aktiven Slots nachziehen.
  - `WebServerManager.cpp`: `liveVol()`/`onchange` an Volumen-Slidern, Test spielt mit aktuellem Volumen.
  - **Hinweis:** Durch die v0.12-Sound-Zuordnung (`sound_map.*`, geänderte `handleSound`/Sound-Objekt-Verwaltung) und weitere Upstream-Änderungen muss der Fix nach dem Merge fachlich geprüft und ggf. an die neue Sound-Pfadlogik angepasst werden (insb. `handleSound`, `soundMap*`/Testpfad, Motor-Volume-Pfad). Vor Umsetzung Kompilier- und Funktionstest auf S3.

> Bekannte Abweichung: Der bisherige Piko-/Reed-Sonderstand auf `main` (compile-time `REED_TRIGGER_ENABLED`, feste Pins IO5/IO6 → Sound 1/2, `reed_trigger.*`, `flame_led.*`, Direktsteuerung im Motor-Tab) ist in `upstream/main` nicht enthalten. Dieses Konzept ersetzt ihn durch eine **konfigurierbare, portbasierte** Lösung in der regulären Port-/Rollen-Architektur.

---

## Anforderungen (aus Issue/Task)

1. Neuer Branch + Plan (dieses Dokument).
2. Neueste Version von `upstream` als Basis.
3. Eigene Dokumentationen (Bilder, Texte) von `main` übernehmen.
4. Lautstärke-Bugfix von `main` übernehmen (versucht, siehe oben).
5. Pro Port (A, B, C) in der Web-UI Reed-Trigger-Modus aktivierbar statt CRSF/GPS/ähnlichem.
6. Wenn Reed-Trigger-Modus für einen Port aktiv: **beide IO-Pins des Ports in Pull-up-Modus** schalten statt UART (analog Verhalten auf `main`).
7. Neuer Web-UI-Tab: pro Pin der Reed-Trigger-Ports Aktion festlegen:
   - spiele sound 1 / 2 / 3 / 4
   - Motorsound ein/ausschalten (Toggle)
   - LED-Flamme ein/ausschalten (Toggle)
   - pro Aktion ein Button zum manuellen Triggern (ohne echten Reed-Kontakt).
8. Triggering eines Reed-Impulses: Kommando **auf gleiche Art in die Queue** wie aktuell z. B. über den CRSF-Bus.

---

## Betroffene Bereiche / Architekturüberblick

| Bereich | Relevante Dateien (upstream-Basis) | Zweck |
|---|---|---|
| Portrollen/Enums | `src/RC_Common/uart_roles.{h,cpp}` | Rollen-IDs/Namen/Defaults/Einzelinstanz-Regeln; NVS-Wert |
| Port-Init/Laufzeit | `ESP32_Sound_Mini_S3.ino` (`uartPorts[]`, `Port*_Role_boot`, `uartPortsBegin/Update`) | UART starten, CRSF-Router, GPS/S.Port/Raw |
| Pin-Mapping | `pins.h` (`PIN_CRSF_RX/TX`, `PIN_PORTB_RX/TX`, `PIN_PORTC_RX/TX`, `HAS_PORT_C`) | Physische Pins pro Board (S3/C3) |
| Konfiguration/Persistenz | `config.{h,cpp}`, `src/RC_Common/settings_common.{h,cpp}` | Defaults, Laden/Speichern, Migration alter Werte |
| Web-UI & API | `WebServerManager.{h,cpp}` (PROGMEM HTML+JS, `/api/config`, `/api/sound`, neue Endpunkte) | Anzeige/Bearbeitung Rollen und Aktionen, Testbuttons |
| Lua-Menü | `ESP32_Sound_Mini_S3.ino` (`buildLuaMenu`, Getter/Setter) | Port-Rolle/Baudrate im Sender-Menü |
| Sound/Trigger-Verarbeitung | `ESP32_Sound_Mini_S3.ino` (`Sound_on_web[]`, `handleSound`, `Sound_play`, Motor-State-Machine, `toggleMotorSound`/`setMotorSound` falls vorhanden) | Ausführung von Sound-/Motor-Kommandos |
| Flammen-LED | neu zu integrieren/portieren (vgl. `flame_led.*` auf `main`) + ggf. `src/RC_Common/status_led.*` | Toggle-Effekt auf Status-LED |
| Reed-Logik | neu (ersetzt `reed_trigger.*` von `main`) | Entprellung, Flankenerkennung, Zuordnung Pin→Aktion |

---

## Designentscheidungen (Vorschlag)

### 1. Reed als Port-Rolle
- Neue Rolle **`UROLE_REED`** in `uart_roles.{h,cpp}` (hinten anhängen, `UROLE_COUNT` erhöhen), damit der Wert stabil in NVS/Web/Lua gespeichert werden kann.
- `uartRoleOptions()`/`uartRoleName()` um „Reed" ergänzen. `uartRoleDefaultBaud()` irrelevant; Baudrate für Reed-Ports in UI ausblenden/ignorieren.
- **Port A** ist in `upstream` derzeit fest CRSF-Eingang. Reed-Trigger-Modus für Port A erfordert, Port A ebenfalls rollenfähig zu machen (oder eine explizite Port-A-Rolle `Aus(CRSF)/Reed` zu modellieren):
  - *Empfehlung:* Port A wie B/C als konfigurierbare Rolle behandeln, mit Default `UROLE_CRSF` (bestehendes Verhalten). Nur Rolle CRSF auf Port A aktiviert den RC-Eingang/Bus-Init (`crsf.begin`/`read_packets`/Failsafe-Quelle). Damit bleibt Kompatibilität erhalten und Port A kann auf Reed umgestellt werden (Piko-35040-Anwendungsfall).
  - Alternativ (kleiner Eingriff): separates Flag nur für Port A. **Nicht empfohlen**, da inkonsistent zu B/C und schwerer erweiterbar.
- **Einzelinstanz:** Reed ist eine lokale GPIO-Rolle ohne geteilten Buszustand; mehrere Ports gleichzeitig im Reed-Modus müssen möglich sein (A+B+C je nach Board). `uartRoleSingleInstance` für Reed = `false`. Konfliktauflösung (`uartRoleResolveSecond`) nur dort anwenden, wo sie heute greift (S.Port/GPS/Raw); CRSF/Reed explizit betrachten.
- **Board-Einschränkung:** Port C nur wenn `HAS_PORT_C` (C3 hat keinen Port C) → UI/Config blendet Port C wie bisher aus.

### 2. Pin-Modell und GPIO-Konfiguration
- Jeder Port hat genau 2 Pins: `RX` und `TX` (siehe `pins.h`). Im Reed-Modus gilt pro Pin:
  - `pinMode(pin, INPUT_PULLUP);`
  - kein `Serial.begin`/UART-Attach auf diesem Port.
- Beim Rollenwechsel (Speichern + Neustart, analog bestehendem Verhalten bei Portrollen) werden Pins entsprechend initialisiert. Laufzeit-Rekonfiguration ohne Neustart ist komplex (UART-Detach/Attach, CRSF-Abhängigkeiten) → **empfohlen: Änderung wird beim Speichern vorgemerkt, Wirksamkeit nach Neustart** (wie heute Portrollen/`*_boot`), im UI klar anzeigen.
- Mapping Pin-Identität für Konfiguration: stabil benennen als `portA.rx/portA.tx`, `portB.rx/portB.tx`, `portC.rx/portC.tx` (bzw. Index 0..5), damit Zuordnung unabhängig vom konkreten GPIO ist.

### 3. Aktionen, Speicherung und „Queue wie CRSF"
- Pro konfigurierbarem Reed-Pin eine **Aktion** speichern, z. B. Enum/ID:
  - `NONE` (keine)
  - `PLAY_SOUND_1` … `PLAY_SOUND_4`
  - `TOGGLE_MOTOR`
  - `TOGGLE_FLAME`
- Persistenz: in `ConfigData` (und ggf. `settings_common`/Backup-Format) als Array je Pin/Port, NVS-Version/Migration beachten. Defaults = `NONE`.
- **Ausführungspfad (wichtig, Anforderung):** Reed-Trigger darf nicht „nebenbei" Sonderlogik ausführen, sondern muss denselben Kommando-/Zustandspfad nutzen wie CRSF:
  - Für Sound 1–4: denselben Mechanismus wie `Sound_on_web[idx] = true` / bestehende Test- und CRSF-Auslösung, der in `loop()`/`handleSound` bzw. der aktuellen Sound-Dispatch-Logik verarbeitet wird. Auf `upstream` v0.13 `Sound_on_web[RCSOUND_NUM_SLOTS]` und `handleSound` nutzen; sicherstellen, dass das Setzen identisch zum Web-Test/CRSF-Trigger erfolgt (ggf. kleine Helper-Funktion `enqueueSoundTrigger(slot)` / `enqueueCommand(cmd)` einführen und von Reed, Web-Test und ggf. CRSF-Pfad verwenden).
  - Für Motor/Flamme: dieselben Toggle/Set-Funktionen verwenden, die auch `POST /api/control` bzw. CRSF-Motorsteuerung nutzen (`toggleMotorSound`/`setMotorSound`, `flameToggle`/`flameSet`). Falls auf Upstream-Basis nicht vorhanden, analog zu `main` sauber einführen und an einer Stelle bündeln (gemeinsamer „Command Executor"), damit Reed, UI-Testbutton und CRSF identisch wirken.
  - Optional sauberer: ein kleines **Kommando-Enum + Ringpuffer/Queue** (z. B. `CmdQueue.push(CMD_PLAY_SOUND_1)`), das in `loop()` abgearbeitet wird und intern die obigen Aktionen ausführt. Das erfüllt die „Queue wie CRSF"-Forderung explizit und entkoppelt ISR-/Pin-Abtastung von der Ausführung.
- **Entprellung/Flanke:** INPUT_PULLUP, aktive Flanke typischerweise `HIGH -> LOW` (Kontakt gegen GND), Entprellzeit konfigurierbar (global oder pro Port; Minimum sinnvoll, z. B. 20–100 ms). Verhalten wie bisheriger Reed-Betrieb: bei stabiler Schließflanke einmal triggern (kein Repeat, kein Long-Press nötig, sofern nicht gefordert).

### 4. Web-UI
- **Rollen-Auswahl (bestehender Settings/Ports-Bereich):** In den Dropdowns für Port A/B/C die Option „Reed-Trigger" ergänzen (`/api/config`/`/api/roleopts` bzw. was upstream nutzt). Bei Rolle Reed Baudrate-Feld deaktivieren/verbergen. Board-abhängig Port C nur anzeigen, wenn vorhanden.
- **Neuer Tab „Reed-Trigger"** (Name final abstimmen, z. B. „Trigger"/„Reed"): 
  - Nur Pins von Ports anzeigen, die aktuell als Reed konfiguriert sind (oder alle mit Hinweis „Port nicht im Reed-Modus"). Hinweis: Wirksamkeit nach Neustart, wenn Rolle gerade geändert wurde.
  - Pro Pin: Port/Pin-Bezeichner, Dropdown Aktion (Sound 1–4, Motor an/aus, Flamme an/aus, Keine), **Test-Button** („Auslösen/Testen") je Aktion/Zeile.
  - Globale Reed-Einstellungen: Entprellzeit (ms), optional Pull-up-Hinweis.
- **API (Vorschlag):**
  - Erweitern: `GET/POST /api/config` um Port-A-Rolle (falls neu) + Reed-Pin-Aktionszuordnung + Entprellzeit.
  - `POST /api/reed/test` (oder generisch `POST /api/trigger` / Wiederverwendung `POST /api/control` + Sound-Test-Endpunkt) mit Zielaktion/Pin-Aktion → ruft denselben Command-Executor/Queue-Pfad auf wie echter Reed-Impuls.
  - Status/Capabilities: Ports/Pin-Liste, ob Reed aktiv (laufend vs. konfiguriert), Board-Info (`HAS_PORT_C`).
- UI bleibt im bestehenden Stil (PROGMEM HTML/JS in `WebServerManager.cpp`).

### 5. Lua-Menü
- Port-Rollen-Auswahl um „Reed" erweitern (über `uartRoleOptions()`/bestehende Port-Felder), Baudrate bei Reed ggf. irrelevant/ausgeblendet.
- Aktionen pro Pin sind für Lua-Menü eher umfangreich; **MVP:** Konfiguration im Web-UI, Lua zeigt Rolle Reed an. (Optional später: Untermenü je Port/Pin.) Feldnummern/Kompatibilität der bestehenden Lua-Struktur beachten.

### 6. Zusammenspiel mit bestehenden Funktionen
- **CRSF/RC-Eingang:** Wenn Port A auf Reed steht, kein CRSF-Empfang auf Port A → `BUS_OK`/Failsafe/RC-gesteuerte Sounds verhalten sich wie „kein Signal" (Auto-WLAN-Failsafe beachten: erwünscht dokumentieren). Status-LED-Modus für „kein CRSF" vs. „Reed aktiv" definieren.
- **Status-LED & Flamme:** Auf `main` ist Flammen-Effekt nur im Reed-Modus sinnvoll, weil CRSF-LED dort frei ist. Bei portbasiertem Reed (evtl. nur Port B/C Reed, Port A noch CRSF) Konflikt klären: Flamme ggf. nur erlauben, wenn kein CRSF-Status die LED beansprucht, oder LED-Prioritäten sauber definieren.
- **Motor-Toggle:** Im reinen CRSF-Betrieb kam Motor-Ein/Aus bisher über Kanal/Source; Toggle-Aktion muss mit bestehender State-Machine (`engine_on_toggle`, `Sound_play[0]`, Start/Loop/Stop-Logik) konsistent sein (Zustand halten, nicht nur Impuls), wie `toggleMotorSound` auf `main`.
- **Sound-Zuordnung v0.12:** „spiele sound 1–4" bezieht sich auf die **Slots 1–4** (Trigger-Logik), unabhängig davon, welche Datei diesen Slots zugeordnet ist.

---

## Umsetzungsschritte (empfohlene Reihenfolge)

1. **Basis stabilisieren**
   - Kompilierbarkeit nach Doc-Import + Lautstärke-Cherry-Pick prüfen (S3/C3), Volume-Fix an v0.12-Soundpfade anpassen.
   - Kurzer Rauchtest: Boot, Web-UI, Sound abspielen, Lautstärke live ändern.

2. **Rollenmodell & Konfiguration**
   - `UROLE_REED` in `uart_roles.{h,cpp}` + Optionen/Namen/Defaults.
   - Port A konfigurierbar machen (Config-Felder, Defaults = CRSF, NVS-Migration von altem festen Verhalten).
   - `ConfigData` um Reed-Pin-Aktionen + Entprellzeit erweitern; `load/save/Reset_all`, Backup/Export-Import (`/api/config` JSON) und Settings-Migration (`settings_common`/Versionspfad) aktualisieren.

3. **Runtime: Port-Init & Reed-Scan**
   - In Setup/Port-Initialisierung: für Ports mit Rolle Reed `pinMode(INPUT_PULLUP)` auf RX+TX, kein UART; für CRSF nur initialisieren, wenn entsprechender Port Rolle CRSF hat.
   - Reed-Scan-Modul (neu, boardunabhängig über `pins.h`): Debounce + Flankenerkennung pro Pin, ruft bei Impuls zentralen Command-Dispatch.
   - Sicherstellen, dass CRSF-Poll/Router/Failsafe nur auf tatsächlich aktiven CRSF-Ports zugreifen.

4. **Kommando-Dispatch / Queue**
   - Gemeinsamen Ausführungspfad definieren (`enqueue`/`execute`): Sound 1–4, Motor-Toggle, Flamme-Toggle. Von Reed, Web-Test und (wo sinnvoll) bestehenden Triggern nutzen.

5. **Flammen-LED integrieren**
   - `flame_led` (oder gleichwertig) auf Upstream-Basis portieren/neu implementieren; LED-Zugriff mit `status_led` koordinieren (Priorität, wann Flackern aktiv sein darf).

6. **Web-API & UI**
   - `/api/config` (und ggf. neue Endpunkte) um Port-A-Rolle, Reed-Aktionen, Test-Trigger erweitern.
   - Portrollen-Dropdowns inkl. Reed; neuer Tab „Reed-Trigger" mit Aktionsmatrix pro Pin und Testbuttons; UX-Hinweise (Neustart nötig, Port nicht im Reed-Modus).
   - Web-Testbuttons nutzen denselben Dispatch wie echter Impuls.

7. **Lua & Doku**
   - Lua: Rolle Reed auswählbar, konsistente Anzeige; keine inkonsistenten Feld-IDs.
   - `README.md`, `docs/audio-ablauf.md`, `docs/projektplan.md` aktualisieren (portbasierter Reed, Pin→Aktion, Wegfall compile-time-only Einschränkung soweit umgesetzt).

8. **Tests/Verifikation**
   - Build: S3 und (soweit möglich) C3.
   - Manuell: Rolle setzen → Neustart → Pins hochohmig/pullup, UART inaktiv; Reed-Impuls triggert konfigurierte Aktion; Testbuttons; paralleler CRSF-Betrieb auf anderem Port (falls konfiguriert); Sound 1–4, Motor-Toggle, Flamme-Toggle; Lautstärke-Regler weiterhin korrekt.

---

## Offene Punkte / Klärungsbedarf

- **Port A als CRSF-Eingang:** Bestätigung, dass Port A vollständig rollenfähig werden soll (Default CRSF) oder ob eine Sonderlösung gewünscht ist.
- **„Queue wie CRSF":** Reicht das Setzen derselben Flags/der gleichen Dispatch-Funktion wie der CRSF-Pfad, oder wird eine explizite FIFO-Kommandoqueue im Code erwartet? (Empfehlung: explizite kleine Queue für Nachvollziehbarkeit.)
- **Flamme bei gleichzeitigem CRSF:** Darf die Flammen-LED den CRSF-Status überlagern, und auf welchen Ports/Modi ist sie erlaubt?
- **Aktion „ein/ausschalten" vs. Toggle:** Gewünscht ist laut Task „ein/ausschalten"; üblicherweise als Toggle wie auf `main` implementiert – final festlegen (Toggle oder getrennte Ein/Aus-Kommandos). UI-Buttons können trotzdem gezielt schalten.
- **Persistenz-Format:** Kompatibilität mit bestehenden Backups/`/api/config`-Exporten (neue Felder additiv, sinnvolle Defaults bei Import alter Dateien).

---

## Definition of Done (dieser Plan-Branch)
- [ ] Branch basiert auf aktuellem `upstream/main`.
- [ ] Eigene Dokus/Bilder/Texte aus `main` vorhanden.
- [ ] Lautstärke-Bugfix übernommen oder Abweichungen/Restarbeit dokumentiert.
- [ ] Dieses Plan-Dokument liegt im Branch und beschreibt Architektur, betroffene Stellen, Datenmodell, UI/API, Queue/Dispatch und Verifikation.
