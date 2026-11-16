# Plan: Reed-Trigger per Web-UI konfigurierbar (Ports A/B/C, Pin→Aktion)

Stand: 2026-11-16, Branch-Basis `main` (`da3ea8c`)

## Ziel
- Ports **A/B/C** in der Web-UI als **UART-Rolle** (wie bisher, inkl. CRSF/S.Port/GPS/Raw/Aus) **oder** als **Reed-Trigger** konfigurierbar machen (statt UART auf diesen Pins).
- Bei Reed-Modus pro **Eingang/Pin** konfigurierbar: **welcher Pin** (`IO5`, `IO6`, … / portbezogen) löst **welche Aktion** aus (`Sound 1..N`, `Motor Toggle`, `Flammen-LED`, ggf. `Motor Ein/Aus`, `keine`).
- Möglichst **upstream-kompatibel/update-fähig** bleiben (v0.12/v0.13: `sound_map`, RC_Common 1.4.0, `uart_ports`/`uart_roles`, Status-LED, Portmodell).

## Rahmenbedingungen / Bestandsaufnahme
- **Port-Pins sind board-/portfest** (`pins.h`): S3 z. B. Port A = IO5/IO6, Port B = IO7/IO8, Port C = IO11/IO12 (2 Pins/Port, RX/TX). C3: Port A = IO20/IO21, Port B = IO5/IO6, **kein Port C** (`HAS_PORT_C==0`).
- **Heute:** Reed ist Compile-Time (`REED_TRIGGER_ENABLED` in `reed_trigger.h`), hardcodiert auf IO5/IO6; CRSF Port A wird im Sketch initialisiert, Port B/C über `RC_Common/uart_ports.{h,cpp}` + `uart_roles.h`/`uartRole*`.
- **Konfliktregel:** Dieselben Pins können nicht gleichzeitig UART (CRSF/S.Port/GPS/Raw) und GPIO-Reed-Eingang sein. Umschalten ist am robustesten **nach Neustart** wirksam (analog `PortB_Role_boot`/`PortC_Role_boot`).
- **Upstream-Pflege:** `firmware/ESP32_Sound_Mini_S3/src/RC_Common/` ist gemeinsame Basis; Änderungen dort erhöhen Merge-Aufwand bei Upstream-Updates.

## Vorschlag Datenmodell / Konfiguration (NVS, `config.h/.cpp`)

### 1) Port-Betriebsart getrennt von UART-Rolle halten (empfohlen, RC_Common-schonend)
Statt `UROLE_REED` in `RC_Common/uart_roles.h` einzuführen, pro Port eine **eigene Funktionsart** in der Mini-Firmware:
- `PortA_Mode`, `PortB_Mode`, `PortC_Mode` (z. B. `PMODE_UART` = bisheriges Verhalten, `PMODE_REED` = Pins als digitale Eingänge).
  - Port A: `PMODE_UART` bedeutet CRSF-Bus (wie heute fest); Port B/C: UART-Rolle kommt weiterhin aus `config.PortB_Role/PortC_Role` (`UROLE_*`).
  - Bei `PMODE_REED` wird die jeweilige UART-Rolle des Ports **ignoriert/nicht gestartet** (UI kann Rolle ausgrauen/anzeigen „—“).

*Alternative (invasiver):* `UROLE_REED` in `uart_roles.h` ergänzen und `uart_ports` behandeln lassen. Vorteil: einheitlich; Nachteil: Patch in `RC_Common` bei jedem Upstream-Sync. **Empfehlung:** separater `PMODE_*` in der Fork, `uart_ports` unverändert lassen und Reed-Ports beim Aufbau der Port-Tabelle/Start überspringen.

### 2) Pin→Aktion-Mapping pro physischem Eingang
Jeder konfigurierbare Port hat 2 Pins (z. B. „Pin 1/Pin 2“ = RX/TX des Ports, auf die konkreten GPIOs aus `pins.h` gemappt). Mappingtabelle, z. B.:
- `ReedMap[6]` (A1,A2,B1,B2,C1,C2) oder generisch `ReedMap[PORTS][2]`.
- Je Eintrag:
  - `action`: `RACT_NONE`, `RACT_SOUND_1`..`RACT_SOUND_4` (bzw. dynamisch bis `RCSOUND_NUM_SOUNDS`), `RACT_MOTOR_TOGGLE`, `RACT_MOTOR_ON`/`RACT_MOTOR_OFF` (optional), `RACT_FLAME_TOGGLE`/`RACT_FLAME_ON`/`RACT_FLAME_OFF`.
  - optional pro Eintrag: `invert`/Pulslogik, falls später nötig (erstmal Default: `INPUT_PULLUP`, Schließen nach GND = Impuls wie heute).
- Bestehende Parameter beibehalten/weiter nutzen: `ReedDebounceMs` (global) – später optional pro Pin, aber nicht nötig für v1.

### 3) Persistenz & Migration
- Neue NVS-Keys/Blob (eigener Namespace/Version wie bisher), z. B. `pamode/pbmode/pcmode` oder Blob `reedcfg`.
- **Migration:** beim Laden ohne neue Keys Defaults ableiten:
  - wenn bisher `REED_TRIGGER_ENABLED`-Build-Verhalten abgebildet werden soll: Port A = `PMODE_REED`, A1→`RACT_SOUND_1`, A2→`RACT_SOUND_2` (entspricht heutigem IO6/IO5 auf S3), andere `NONE/UART`.
  - sonst alle `PMODE_UART`, leeres Mapping (kompatibel zum Upstream-Standard).
- `saveConfigForce/loadConfig/Reset_all` entsprechend erweitern; Legacy-Keys unangetastet lassen.

## Laufzeit-/Architekturänderungen

### 4) Port-/Pin-Abstraktion (boardabhängig, zentral in `pins.h`/Helper)
- Pro Port die **zwei konfigurierbaren GPIOs** und Verfügbarkeit bereitstellen:
  - z. B. `portInputPin(port, idx) -> gpio` und `portExists(port)` (Port C nur wenn `HAS_PORT_C`).
  - Mapping auf bestehende Defines: S3 A=(5,6), B=(7,8), C=(11,12); C3 A=(20,21), B=(5,6), C=none. So bleibt das UI konsistent („Port A, Pin 1/2“) trotz unterschiedlicher IO-Nummern.
- **Regel:** Nur Ports/Pins anbieten, deren GPIOs real existieren und nicht anderweitig fix belegt sind.

### 5) Initialisierung & Boot-Reihenfolge (`ESP32_Sound_Mini_S3.ino`)
- **Boot-Snapshot** (wie `PortB_Role_boot`): `PortA_Mode_boot`/`PortB_Mode_boot`/`PortC_Mode_boot` + Reed-Mapping einfrieren; Änderungen per UI → NVS, wirksam nach Neustart (UI-Hinweis wie bei Port-Rollen).
- **Port A:**
  - `PMODE_UART`: `crsf.init_crsf(PORTA_SERIAL, PIN_CRSF_RX, PIN_CRSF_TX)` wie heute.
  - `PMODE_REED`: kein CRSF-Init auf diesen Pins; Pins als `INPUT_PULLUP` registrieren.
- **Port B/C:** beim Befüllen/Start der `uartPorts[]`-Tabelle bzw. vor `uartPortsBegin()`:
  - `PMODE_UART`: Verhalten unverändert (Rolle/Baud/`uartPortsBegin`, Duplikatregeln in RC_Common).
  - `PMODE_REED`: Port nicht als UART starten (`role` effektiv aus/überspringen), Pins als Reed-Eingänge registrieren. `Port*_Role_boot`/Statusanzeige sauber halten.
- `#ifdef REED_TRIGGER_ENABLED` schrittweise durch `if (portMode==PMODE_REED || anyReedActive)` ersetzen; `reed_trigger.{h,cpp}` von fixen Pins auf **konfigurierbare Pin/Aktions-Liste** generalisieren (Init-Liste aus Config, Polling im Loop entprellt wie heute).

### 6) Ereignis-/Aktionsausführung (zentral, wiederverwendbar)
- Einheitliche Funktion, z. B. `applyReedAction(action, pressed/edge)`:
  - `RACT_SOUND_x` → setzt/stoßt das bestehende Sound-Trigger-Flag für Slot x (wie heute `Sound_on_web[x]=true`/Play-Logik), kompatibel zu `sound_map`-Zuordnung.
  - `RACT_MOTOR_TOGGLE/ON/OFF` → nutzt vorhandene `setMotorSound/toggleMotorSound`.
  - `RACT_FLAME_*` → `flame_led` API.
- Entkopplung von konkretem GPIO: Reed-Modul liefert nur „Aktion ausgelöst“, damit später andere Quellen möglich bleiben.

### 7) Verhalten ohne/mit eingeschränktem CRSF
- Wenn Port A = `PMODE_REED`: `BUS_OK`/`lastCrsfPacket`-Pfad, CRSF-Timeout, Router-Kette über A, kanalbasierte Quellen (BUS/MKan/Einkanal/Speed) und CRSF-Telemetrie sind ohne RC-Bus wirkungslos oder eingeschränkt.
  - WLAN-Auto-Failsafe (`wifiFailsafeCheck`) und Status-LED (`updateLed`) müssen für „kein CRSF konfiguriert/aktiv“ definiert werden (z. B. Failsafe nicht fälschlich toggeln, LED-Zustand „Reed/Standalone“ klar anzeigen).
  - CRSF-/Lua-Menü für RC-Quellen/Adressen kann ausgegraut oder als inaktiv markiert werden (nur UI/Anzeige, keine Funktionsänderung an RC_Common nötig).

## Web-UI / API / Lua

### 8) Web-UI (`WebServerManager.cpp`)
- **Port-Konfiguration:** pro Port (A/B/C, C nur wenn vorhanden) ein Modus-Schalter `UART/CRSF` vs `Reed-Trigger`. Bei UART die bestehenden Rollen-/Baudratenfelder; bei Reed Felder ausblenden/deaktivieren.
- **Reed-Mapping-Bereich:** Tabelle „Eingang → Aktion“
  - Eingänge = verfügbare Pins der Ports, die auf Reed stehen (Anzeige: `Port A – Pin 1 (IOx)` / `Pin 2 (IOy)` boardgerecht).
  - Aktion pro Eingang: Dropdown (`Keine`, `Sound 1`..`Sound N` aus `RCSOUND_NUM_SOUNDS`/`sound_map`-Labels, `Motor Toggle`, `Flammen-LED an/aus/toggle` nach Bedarf).
  - Global: `ReedDebounceMs` (vorhanden) weiterhin editierbar.
- **Hinweise/Status:** „wirkt nach Neustart“, Warnung bei Pin-/Port-Konflikt (z. B. Reed auf Port A ⇒ kein CRSF-Empfang).

### 9) REST-API (bestehende Muster weiterverwenden)
- Config lesen/schreiben (`/api/config` o. ä., wie heute): Felder für `porta_mode/portb_mode/portc_mode` (bzw. Port A separat) und Mapping-Array/Einzelfelder.
- Boot-vs-konfiguriert konsistent melden (wie `portb_role` vs `PortB_Role_boot`) für korrekte UI-Anzeige.
- Bestehende Endpunkte (`/api/control`, Sound-Upload/-Map aus v0.12) unverändert lassen, wo möglich.

### 10) CRSF-/Lua-Menü (`buildLuaMenu`, RC_Common `lua_menu`)
- Einträge für Port-Modi und pro Reed-Eingang die Aktion (Auswahllisten) ergänzen; Menü-IDs/Felder hinten anfügen, um bestehende IDs stabil zu halten (wichtig für Kompatibilität).
- Lange Listen/Chunking beachten (bereits Thema in v0.11) und Anzeige auf verfügbare Ports/Pins begrenzen.

## Upstream-Kompatibilität / Dateistrategie
- **Nicht (oder nur minimal) ändern:** `src/RC_Common/*` – idealerweise keine neuen Rollen/Semantik erzwingen; falls unvermeidbar, Änderung klein und isoliert halten (einfach rebasebar).
- **Fork-spezifische Logik kapseln:** Reed-Mapping, Aktions-Dispatch und Port-Mode-Auflösung in eigenen Modulen/Abschnitten (z. B. `reed_trigger.*` generalisieren + kleine `port_inputs.*`/Helper) statt quer in Upstream-Logik zu streuen.
- **Konfigurations-Erweiterungen additiv:** neue Felder/Keys mit Defaults, keine Umdeutung bestehender `PortB_Role/PortC_Role`-Werte.
- **UI/JS:** neue Bereiche ergänzen, bestehende v0.12-Sound-Zuordnungs- und RC_Common-OTA/Backup-Teile möglichst unverändert übernehmen.
- **Branch-/Merge-Praxis:** Feature in eigenem Branch, Upstream-Änderungen regelmäßig mergen; Konfliktflächen klein halten (wenige, klar markierte Integrationspunkte im `.ino`/`WebServerManager`).

## Umsetzungsphasen (inkrementell, jeweils baubar/testbar)
1. **Config & Abstraktion:** `PMODE_*`, Mapping-Struct, NVS-Migration/Defaults, Port→GPIO-Helper (`pins.h`).
2. **Runtime:** Boot-Snapshots, bedingte Init Port A/B/C, `reed_trigger` auf konfigurierbare Inputs umstellen, Aktions-Dispatch anbinden.
3. **Web-UI/API:** Modus pro Port + Mapping-UI + Persistenz + Status/Hinweise.
4. **Lua-Menü:** spiegelnde Einstellmöglichkeiten (kompakt).
5. **Robustheit:** BUS_OK/Failsafe/LED/CRSF-inaktiv-Fälle, Validierung (nur existierende Ports/Pins/Aktionen), Doku/README-Hinweis.

## Test-/Abnahmekriterien
- S3: Port A als Reed konfigurierbar (kein CRSF-Init), Pins A1/A2 lösen konfigurierbare Aktionen aus; Zurückschalten auf CRSF nach Neustart stellt bisherigen Betrieb her.
- Port B/C: Reed-Modus blockiert UART-Start auf dem Port; anderer Port bleibt mit UART-Rolle nutzbar; C3 ohne Port C korrekt.
- Aktionen: Sound 1–4 (mit `sound_map`), Motor-Toggle, Flammen-LED zuverlässig, Entprellung wirkt.
- Persistenz/Migration: bestehende Konfigurationen booten ohne Datenverlust; Upstream-Standardfall (alles UART/CRSF) unverändert.
