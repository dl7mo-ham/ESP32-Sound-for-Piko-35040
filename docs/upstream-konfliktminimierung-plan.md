# Plan: Upstream-Konfliktminimierung, Repo-Strategie und Reed-Rolle 100

## Ziel

Die Piko-Anpassungen sollen gegenüber `upstream/main` **minimal-invasiv** werden: Wenn upstream Quellcode ändert, soll es möglichst wenige Merge-Konflikte geben. Dazu werden drei Dinge getrennt angegangen:

1. Die lokale `main`-Historie wird gesichert und durch den aktuellen, direkt auf `upstream/main` basierenden Branch ersetzt.
2. Die Reed-Rolle wird als **projektlokale Rolle mit dem Wert 100** modelliert; `src/RC_Common/` bleibt upstream-identisch.
3. Der verbleibende Code wird schrittweise in eigene Dateien ausgelagert, damit upstream-Dateien nur noch wenige Call-Sites enthalten.

Dieses Dokument ist die Umsetzungsvorlage für die im Review (`2026-11-17`) identifizierten Punkte.

## Gültige Regeln (Stand 2026-11-18)

Die Maßnahmen unten sind umgesetzt; als bleibende Regeln gelten:

- `src/RC_Common/` bleibt upstream-identisch. Neue Funktionalität kommt in eigene Dateien; in Upstream-Dateien höchstens dünne Aufrufstellen.
- Die Fork-Version (`<upstream-Basis>+dl7mo.<Zähler>`) steht nur in [docs/fork-changelog.md](fork-changelog.md) und in der README-Fork-Sektion. `CHANGELOG.md` bleibt upstream-identisch.
- Der CRSF-Cache-Token kommt aus `firmware/ESP32_Sound_Mini_S3/fork_version.h` (`APP_PARAM_VERSION = 200 + FORK_SEQ`), nicht aus `Version` im Sketch. `FORK_SEQ` bei jeder Änderung und bei jedem Upstream-Merge erhöhen.
- Nach einem Upstream-Merge: `sha256sum -c` in RC_Common, beide Host-Test-Suiten, beide Builds, `git diff --stat upstream/main...HEAD`.

---

## Ausgangslage

- Aktueller Branch: `feature/reed-trigger-ports` (`6b05787`), basiert direkt auf `upstream/main` (`5d48e4f`) mit zwei Squash-Commits.
- `upstream/main` steht aktuell noch auf dem Merge-Base; es gibt **noch keine realen Konflikte**. Die Bewertung ist prospektiv.
- Der Branch verändert ~720 Zeilen in upstream-eigenen Dateien. Hotspots:
  - `ESP32_Sound_Mini_S3.ino`: `loop()` (~55 Zeilen CRSF-Block in `if (crsfActive)`), `updateLed()`, `handleSound()`, `setup()`.
  - `WebServerManager.cpp`: Reed-Felder mitten in `handleApiConfigPost()`, `buildConfigJson()` und in bestehenden JS-Funktionen.
  - `src/RC_Common/`: 8 Dateien geändert (Rolle 5, Manifest, Version, Tests, README).
- Die lokale `main` (`da3ea8c`) ist deutlich divergiert (~36 Dateien, ~3100 Zeilen Unterschied zum Branch). Ein späterer Merge Branch → `main` würde größere Konflikte liefern als der Merge mit upstream.

---

## Teil 1: Repo-Strategie — Snapshot und `main` ersetzen

### Vorgehen

```bash
git switch main
git switch -c snapshot/main-vor-v014
git push -u origin snapshot/main-vor-v014          # nicht nur lokal sichern

git switch feature/reed-trigger-ports
git branch -f main HEAD
git log --oneline -3 main                         # Kontrolle
git diff main HEAD                                # muss leer sein
git push --force-with-lease origin main           # nur mit Absprache: main ist shared!
```

### Hinweise

- `--force-with-lease` verwenden, nie `--force`.
- Danach ist `main` identisch mit `upstream/main` + den 2 Squash-Commits. Die alte Historie bleibt im Snapshot erhalten.
- Weitere Branches auf alter Basis (`feature/reed-doppeldruck-fenster`, `merge-upstream-main`) müssen danach neu aufgesetzt oder gerebased werden.
- Offene Pull Requests / Tags auf `origin/main` vorher prüfen.

### Abnahme

- `git merge-base main upstream/main == 5d48e4f` (bzw. aktueller upstream-Stand).
- `snapshot/main-vor-v014` zeigt auf die alte `main`-Spitze und ist gepusht.

---

## Teil 2: Reed als App-Rolle mit Wert 100

### Warum nicht einfach `UROLE_REED` auf 100 setzen?

RC_Common behandelt Rollenwerte als **dichten Index**, nicht als beliebige ID:

| Stelle | Verhalten bei 100 |
|---|---|
| `uart_roles.cpp:15` `uartRoleClamp()` | akzeptiert nur `0 … UROLE_COUNT-1` → 100 wird zu `UROLE_OFF` |
| `uart_roles.cpp:7` `uartRoleName()` | liefert `"Aus"`, da `100 >= UROLE_COUNT` |
| `lua_menu.cpp:32–39` `m.select()` | `max = optionCount(options)-1` (= 5); `LuaMenu::write()` clampet auf 0…5 → 100 würde als „Raw" erscheinen |
| `WebServerManager.cpp:768–776` `fillSelFlat()` | `index == value`; mit 6 Optionen ist 100 nicht wählbar |
| `web_common.cpp:186–190` `/api/uartroles` | erzeugt `id == laufender Index` |

Ein reiner Konstantentausch erfordert daher Sonderfälle in `clamp()`/`name()` plus Mapping in Lua und Web – also **mehr** inline-Änderungen in RC_Common als heute. Zudem bleiben Manifest, Versionsbump und Test-Hunks bestehen.

### Gewählte Lösung

`100` wird als **projektlokale Rolle** geführt. RC_Common wird auf den upstream-Stand 1.4.0 zurückgesetzt. Die Zahl 100 ist weit außerhalb des upstream-Bereichs und kollidiert nicht mit künftigen Rollen 5–99.

| Ebene | Wert | Bemerkung |
|---|---|---|
| RC_Common-API (NVS, UART) | `UROLE_OFF` / `UROLE_CRSF` … `UROLE_RAW` (0–4) | bleibt upstream-identisch |
| App-Konfiguration / NVS | `APP_UROLE_REED = 100` | gespeicherter Wert, `uint8_t` |
| Lua / Web-Dropdown | Index `5` (letzter Eintrag „Reed") | Mapping 5 ↔ 100 an den UI-Grenzen |

### Umsetzungsschritte

1. **RC_Common zurücksetzen** (alle Änderungen aus `6b05787` an dieser Kopie rückgängig machen):
   - `rc_common.h` (`"1.4.0"`), `README.md`, `MANIFEST.sha256`, `tests/test_common.cpp`,
     `uart_roles.h/.cpp`, `uart_ports.h/.cpp`.
   - Danach: `sha256sum -c --quiet MANIFEST.sha256` und Host-Tests (`tests/run_tests.sh`) müssen grün sein.
2. **Neue Dateien `reed_roles.h/.cpp`** (projektlocal, neben `reed_trigger.*`):
   - `static constexpr uint8_t APP_UROLE_REED = 100;`
   - `appRoleName()`, `appRoleOptions()` (`"Aus;CRSF;S.Port;GPS;Raw;Reed"`),
     `appRoleClamp()`, `appRoleDefaultBaud()`.
   - Delegation an RC_Common für `0…4`; `100` wird als Reed akzeptiert, alles andere → `UROLE_OFF`.
   - **Migration:** Der alte Wert `5` (v0.14) wird in `appRoleClamp()` auf `APP_UROLE_REED` abgebildet, damit gespeicherte Konfigurationen ihren Reed-Modus behalten. Da 5 in RC_Common 1.4.0 ungültig ist, ist diese Interpretation eindeutig.
3. **Call-Sites umstellen:**

   | Datei / Stelle | Änderung |
   |---|---|
   | `config.cpp:21–26` | `portaRoleClamp()` gegen `APP_UROLE_REED` prüfen; `portaRoleValues()` → `"0;1;100"` |
   | `reed_trigger.cpp:67` | `uartRoleClamp(roleB/roleC)` → `appRoleClamp()`; sonst wird 100 wieder zu OFF |
   | `reed_trigger.cpp:70`, `WebServerManager.cpp:2119`, `.ino:520` | Vergleich gegen `APP_UROLE_REED` |
   | `WebServerManager.cpp:1953` | Log über `appRoleName()` statt `uartRoleName()` |
   | `.ino:504–513` (Lua) | Getter `100 → 5`, Setter `5 → 100`; Optionen aus `appRoleOptions()` |
   | `WebServerManager.cpp:893–894` (Web) | `portb_values`/`portc_values` wie `porta_values` ergänzen (`"0;1;2;3;4;100"`) und `fillSelFlatVals()` nutzen |
   | `.ino:802–804` (`setup()`) | Vor `uartPortsBegin()` lokale `uartPorts[].role`-Kopien `100 → UROLE_OFF` mappen; `reedBegin()` setzt die Pull-ups selbst |
   | `reed_trigger.h` (Kommentar), Doku | `UROLE_REED` → `APP_UROLE_REED` umbenennen, lokale Konvention vermerken |
4. **Reed-Tests auslagern:** assertions aus `RC_Common/tests/test_common.cpp` in eine projektlokale Testdatei; die RC_Common-Testdatei bleibt upstream-identisch.

### Akzeptierte Nachteile

- `/api/uartroles` und `/rc_common.js` listen weiter nur 0–4 (ohne Reed). Werden nur von externen Konsumenten genutzt; das Firmware-UI nutzt eigene Options-/Wertelisten.
- `100` ist eine lokale Konvention: Backups/Werte anderer Projekte mit 100 würden hier als Reed interpretiert. In Doku und `config.h` vermerken.
- Port A nutzt bereits eine eigene Werteliste; das Web-Mapping wird damit allgemein für A/B/C einheitlich.

---

## Teil 3: Auslagerung in eigene Dateien (Roadmap)

Reihenfolge nach Konfliktgewinn:

| Schritt | Neue Datei(en) | Inhalt | Call-Sites in upstream-Dateien |
|---|---|---|---|
| 1 | `command_queue.cpp/.h` | `REED_ACTION_*`, Ringpuffer, `enqueueCommand()`, `processCommandQueue()`, `executeCommand()` | `loop()` 1 Aufruf; `reed_trigger.h` inkludiert Header statt `extern` |
| 2 | `config_reed.cpp/.h` | `ReedConfig`, zentral geclampte Entprellzeit, Reed-NVS/JSON | `config.cpp` 2 Aufrufe |
| 3 | `reed_web.cpp/.h`, `/reed_ui.js` | Reed-Tab HTML/JS, POST-/JSON-Felder, `/api/reed/test` | 3–4 Registrierungs-/Call-Zeilen |
| 4 | `crsf_bridge.cpp/.h` | `crsfLoopTasks()` (Pakete, Telemetrie, Replies) | `loop()` 1 Zeile |
| 5 | `motor_control.cpp/.h`, `sound_trigger.cpp/.h` | Motor-Toggle, `handleSound()`/One-Shot | `loop()` 2 Aufrufe |
| 6 | `status_led_arbiter` oder `flameUpdate() → bool ownsLed` | LED-Priorität Flamme vs. Status | `loop()` 1 Aufruf |
| 7 | `app_roles_bridge.cpp/.h` | Reed-Mapping an der Lua-Grenze (Optionsliste, 5 ↔ 100, Baud ausblenden), `uartPorts[]`-Mapping, Boot-Log Port A | `.ino` 6 duenne Call-Sites, `buildLuaMenu()` wieder upstream-identisch |

### Quick Wins (unabhängig von den Auslagerungen)

- `config.cpp:28–30`: statisches `reedActionClamp()` entfernen; einzige Definition nutzen (Duplikat zu `reed_trigger.cpp:42–44` verursacht auseinanderlaufende Validierung).
- `.ino:942–945, 966`: `Sound_*.Volume = config.Volumen_Sound[...]` vor `Play(..., true, volume)` streichen (doppelte Zuweisung).
- `reed_trigger.cpp:72`: `p < 2` aus `REED_PORTS` ableiten; `:113` `st.stable != LOW` → `if (st.stable)`.
- `reed_trigger.cpp:86`: geclampte statt rohe Entprellzeit loggen.
- Doku: Executor-Bezeichnung vereinheitlichen (`executeCommand()` vs. `processCommandQueue()`), Ablaufdiagramm Web/Lua → Queue korrigieren.

---

## Reihenfolge und Abnahme

1. Teil 1 (Repo-Strategie) — Snapshot und `main`-Ersatz.
2. Teil 2 (Rolle 100, RC_Common zurücksetzen) — inklusive NVS-Migration 5 → 100.
3. Quick Wins.
4. Auslagerungen Schritt 1–6, nach jedem Schritt Build + Tests.

**Tests vor und nach jedem Schritt:**

- `cd firmware/ESP32_Sound_Mini_S3/src/RC_Common/tests && ./run_tests.sh`
- `sha256sum -c --quiet MANIFEST.sha256` im RC_Common-Ordner
- PlatformIO-Build für `s3` und `c3`
- Web-UI: Rollen-Dropdowns A/B/C, Baudrate-Ausblendung bei Reed, Testbuttons, `/api/config`-Export/Import
- Hardware-Test: Reed-Impuls auf A/B/C, Motor-/Flammen-Toggle, NVS-Migration eines Geräts mit altem Wert 5
