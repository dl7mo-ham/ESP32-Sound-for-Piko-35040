# Soundmodul Mini – Ablauf-Übersicht

## 1. Trigger-Quellen

```
CRSF-Bus (Port A, Default)  Reed-Trigger (A/B/C)   Web /api/reed/test   Lua "Test Sound"
        │                        │                      │                    │
   Config()              reedUpdate()+debounce   enqueueCommand()     Sound_on_web[i]=true
   wertet Kanäle aus     Pin RX/TX -> Aktion            │                    │ (direkt, keine Queue)
        │                        │                      ▼                    │
        │                        └──────────┬───── Command-Queue (16)         │
        │                                   │              │                  │
        └───────────────────────────────────┼──────────────┼──────────────────┘
                                            ▼              ▼
                                   processCommandQueue() in loop()
                                   (intern: executeCommand())
                                            │
                   Sound_on_web[i] / toggleMotorSound() / flameToggle()
                                            │
                                       handleSound(i)
```

Reed-Impulse und die Testbuttons des Web-Tabs „Reed“ benutzen dieselbe Queue
(`enqueueCommand()` → `processCommandQueue()` → `executeCommand()`). Der Lua-Test
setzt `Sound_on_web[i]` direkt – er läuft **nicht** über die Queue. `handleSound(i)`
verarbeitet danach die One-Shot-Flags.

> [!NOTE]
> `Sound_on[]` wird jeden Loop-Durchlauf neu gesetzt (dauerhaft, solange Quelle aktiv).
> `Sound_on_web[]` ist ein **One-Shot-Flag**: `handleSound` setzt es sofort wieder auf `false`.

> [!NOTE]
> Seit `v0.13+dl7mo.1` (früher v0.14) ist Reed eine Port-Rolle (nicht mehr `REED_TRIGGER_ENABLED`).
> Seit `v0.13+dl7mo.2` (früher v0.15) ist sie projektlokal `APP_UROLE_REED` = 100 (`reed_roles.h`);
> RC_Common 1.5.2 bleibt upstream-identisch.
> Ein Reed-Port liest RX und TX mit Pull-up; ein Impuls (HIGH → LOW, nach Entprellzeit) löst die konfigurierte Aktion
> aus: Sound 1–4, Motor Ein/Aus oder Flamme Ein/Aus. Aktionen sind pro Pin frei wählbar und im
> Web-Tab „Reed“ einzeln testbar. Rollenwechsel brauchen einen Neustart, Aktionen/Entprellzeit
> gelten sofort.

---

## 2. handleSound – Ablauf für Sound 1–4

```
handleSound(i, &SoundX)
│
├─ webTrigger = Sound_on_web[i]
├─ Sound_on_web[i] = false                  ← One-Shot immer zurücksetzen
│
├─ Sound_on[i] || webTrigger ?
│   ├─ JA, noch nicht laufend (Sound_play[i]==false):
│   │     SoundX.LoadWavFile()
│   │     if Mode==Loop: SoundX.RepeatForever = true
│   │     I2SAudio.Play(&SoundX)
│   │     Sound_play[i] = true
│   │
│   └─ JA, läuft bereits: nichts tun (läuft weiter)
│
└─ Sound_on[i] == false && webTrigger == false ?
    ├─ RepeatForever = false
    ├─ Mode==Tippbetrieb: I2SAudio.Stop(&SoundX)
    └─ SoundX.Playing == false && Sound_play[i] == true:
          SoundX.UnLoadWavFile()
          Sound_play[i] = false
          testSoundActive[i] = false
```

> [!NOTE]
> Seit `v0.13+dl7mo.3` ruft `handleSound()` zuerst `soundPlaybackSanitize()`
> (`sound_playback.cpp`) auf. `Play()` setzt `Playing = true`, auch wenn die
> Datei fehlt (`FileOK = false`); `NextSample()` bleibt dann stumm und räumt das
> Flag nie auf. Ohne den Aufruf bliebe bei einem leeren Platz
> `Sound_play[i]` dauerhaft `true` und der Slot ließe sich nicht mehr neu
> auslösen.

---

## 3. Motor-State-Machine (Sound 0)

### Ablauf & Phasen

```
         Sound_play[0] = true
         (Motor-EIN-Signal oder Throttle > 0)
                  │
          ┌───────▼────────┐
          │      OFF       │ ◄─── Sound_play[0] = false
          └───────┬────────┘      (Motor-AUS oder Standgas-Timeout)
                  │ Play(start.wav)
          ┌───────▼────────┐
          │   STARTING     │
          └───────┬────────┘
                  │ start.wav fertig
                  │ Play(loop.wav, RepeatForever=true)
          ┌───────▼────────┐
          │    RUNNING     │ ── Throttle-Rampe → loop.wav.Speed
          └───────┬────────┘
                  │ Sound_play[0] = false
                  │ Stop(loop.wav)
          ┌───────▼────────┐
          │   STOPPING     │
          └───────┬────────┘
                  │ loop.wav fertig → Play(shut.wav)
                  │ shut.wav fertig
                  └──────► OFF
```

> [!NOTE]
> Seit `v0.13+dl7mo.3` korrigiert `motorControlLoop()` denselben Zustand auch für
> `Sound_start` – und zwar **vor** dem `crsfActive`-Return, also auch im
> Reed-Betrieb. Ohne diesen Aufruf bleibt ein leerer Start-Platz dauerhaft in
> `STARTING` hängen (`Sound_start.Playing` bleibt trotz Stille `true`), und
> `loop.wav` wird nie gestartet. `loop`/`shut` werden bewusst nicht korrigiert,
> damit ein nachträgliches Zuweisen dieser Dateien den laufenden Loop direkt
> übernimmt.

Der Motorsound besteht aus drei Phasen:
* **`start.wav`**: Anlassergeräusch beim Einschalten.
* **`loop.wav`**: Dauerloop für das Fahr- und Standgasgeräusch (`RepeatForever = true`). **Nur diese Datei wird dynamisch in Drehzahl und Tonhöhe moduliert.**
* **`shut.wav`**: Abstellgeräusch beim Ausschalten.

### Drehzahl- und Tonhöhenvariation (Pitch & Speed)

Im Zustand `RUNNING` passt das Modul die Wiedergabe kontinuierlich an den aktuellen Gaswert an:

1. **Gas-Auswertung & Totband:**  
   Aus dem konfigurierten CRSF-Gaskanal (`Source_Speed_Sound_0`) wird ein Prozentwert (`throttle`, 0–100 %) ermittelt. Je nach `throttle_mode` geschieht dies für einen Einweg-Hebel oder für Mittelstellung (Vorwärts/Rückwärts). Ein konfigurierbares Totband (`throttle_dead_band`) hält den Motor um den Nullpunkt stabil im Standgas.

2. **Trägheitssimulation via Gas-Rampe (`ramp_throttle`):**  
   Damit die Drehzahl bei abrupten Knüppelbewegungen nicht digital springt, begrenzt `ramp_throttle()` die Änderungsrate auf maximal `throttle_ramp` %/s (Standard: 20 %/s). Der Motor dreht dadurch wie ein schweres Aggregat mit Massenträgheit hoch und tourt sanft wieder ab.

3. **Skalierung auf Wiedergabegeschwindigkeit (`Sound_loop.Speed`):**  
   Der geglättete Gaswert wird linear zwischen der minimalen und maximalen Geschwindigkeit interpoliert:
   ```cpp
   if (config.Min_Speed_Sound_0 != config.Max_Speed_Sound_0)
       Sound_loop.Speed = floatMap(throttle, 0, 100, config.Min_Speed_Sound_0, config.Max_Speed_Sound_0) / 100.0f;
   ```

4. **Resampling in der Audio-Engine (`XT_I2S_Audio`):**  
   Beim Berechnen des nächsten Audio-Samples multipliziert die Audio-Klasse den Leseschritt mit dem Speed-Faktor:
   ```cpp
   float step = IncreaseBy * Speed;
   Count += step;
   ```
   Ein höherer `Speed`-Wert führt dazu, dass die Samples der `loop.wav` schneller durchlaufen werden. Dadurch steigen sowohl die **Tonhöhe (Pitch)** als auch die **Taktfrequenz der Motorgeräusche** proportional an.

### Einstellbare Parameter (Web-UI & Lua-Menü)

Die Parameter können über die Weboberfläche (Tab *Motor*) oder das Lua-Menü am Fernsteuersender angepasst werden:

| Parameter | NVS-Key | Standard | Beschreibung |
|---|---|---|---|
| **Drehzahl min** | `minsp` | `100 %` | Abspielgeschwindigkeit bei Standgas (100 % = Originaltempo der WAV) |
| **Drehzahl max** | `maxsp` | `300 %` | Abspielgeschwindigkeit bei Vollgas (z.B. 300 % = 3-fache Geschwindigkeit/Tonhöhe) |
| **Motor Rampe** | `thrr` | `20 %/s` | Maximale Gasänderungsrate pro Sekunde für sanftes Beschleunigen/Abtouren |
| **Standgas Totband** | `thrdb` | `10 %` | Nullbereich um die Ruhestellung, in dem der Motor im Standgas bleibt |
| **Motor Mode** | `thrm` | `0` | `0` = Eine Richtung (0–100 %); `1` = Zwei Richtungen (Mitte 0 %, Ausschlag vor/zurück 0–100 %) |
| **Quelle Speed** | `spd` | *deaktiviert* | CRSF-Kanal für den Gashebel (0–15) |
| **Motor aus Standgas** | `shdly` | `0 s` | Abschalt-Timer nach Inaktivität im Standgas (0 = dauerhaft an) |
| **Motor EIN Modus** | `entog` | `0` | `0` = Taster/Schalter direkt; `1` = Toggle (Schalterimpuls schaltet ein/aus) |

> [!NOTE]
> Die Drehzahl-Variation erfordert das dynamische Gas-Signal eines RC-Empfängers (CRSF). Steht Port A auf Reed/Aus, gibt es keinen Gaskanal; der Motor läuft dann mit der Mindestdrehzahl (`Drehzahl min`). Dennoch lässt sich der Motorsound per Reed-Aktion „Motor Ein/Aus“ oder dem Web-Testbutton schalten.

---

## 4. Audio-Pipeline (FreeRTOS)

Der Arduino-`loop()` **initiiert** das Abspielen; die eigentliche Audio-Ausgabe läuft vollständig in drei FreeRTOS-Tasks auf Core 1.

```
Arduino loop() (Core 1)
  I2SAudio.Play(&SoundX)
    └─► Play() – Playlist eintragen (PlaylistLockGuard/Mutex)
          └─► SoundX.Init() – erster 1024-Byte-Block synchron von LittleFS lesen

              ┌─────────────────────────────────────────────────────────┐
              │  Core 1 – FreeRTOS-Tasks (laufen dauerhaft parallel)    │
              │                                                         │
              │  I2SProducer  (Prio 2, ~1ms-Takt)                      │
              │    └─► FillBuffer()                                     │
              │          └─► MixSamples()  – alle aktiven Sounds mixen  │
              │                └─► SoundX.NextSample()                  │
              │                      └─► Samples[] verbraucht?          │
              │                            ├─ BackReady: memcpy-Tausch  │
              │                            └─ sonst: ReadFile() (Notfall)│
              │                      └─► RequestBackFill() → Queue      │
              │          └─► RingWriteSlot() → Ringbuffer (16 Slots)    │
              │                                                         │
              │  I2SConsumer  (Prio 2)                                  │
              │    └─► RingBuffer-Slot lesen                            │
              │          └─► i2s_write() → I2S-DMA → MAX98357A         │
              │                                    → Lautsprecher       │
              │                                                         │
              │  SDReader     (Prio 1)                                  │
              │    └─► xQueueReceive(g_sdRefillQueue)                   │
              │          └─► LittleFS.read() → BackSamples[]           │
              │                └─► BackReady = true                     │
              └─────────────────────────────────────────────────────────┘
```

### Puffer-Kette (Doppelpuffer)

```
LittleFS  ──►  BackSamples[]  ──(memcpy)──►  Samples[]  ──►  MixSamples()
              (SDReader füllt               (NextSample
               im Hintergrund)              liest davon)
```

> [!NOTE]
> Jeder Puffer ist **1024 Byte** groß. Bei 44100 Hz Stereo 16-bit entspricht das ca. **5,8 ms** Audio-Daten pro Block.
> Der Ringbuffer hat **16 Slots à 512 Byte** zwischen Producer und Consumer.

### Latenz vom Reed-Impuls bis zum ersten Ton

| Schritt | Dauer |
|---|---|
| Reed-Flanke → nächster `loop()`-Durchlauf | < 1 ms |
| Entprellzeit (stabiler Pegel nötig) | 5–500 ms (Default 50 ms) |
| `I2SAudio.Play()` → `Init()` → erster LittleFS-Read | ~1–5 ms |
| I2SProducer füllt nächsten Slot | ~1 ms |
| I2S-DMA → Lautsprecher | < 1 ms |
| **Gesamt (ohne Entprellzeit)** | **~3–7 ms** |

---

## 5. Zusammenspiel im loop()

Seit v0.15 sind die Blöcke in projektlokale Dateien ausgelagert; `loop()` enthält nur
noch die Aufrufe (Dateiname in Klammern):

```
loop()                                  (ESP32_Sound_Mini_S3.ino)
  ├─ WebServerManager::Webpage()        (nur wenn AP aktiv)
  ├─ uartPortsUpdate()                  (Port B/C: S.Port, GPS)
  ├─ funk.loop()                        (RC_Common/funk_link.cpp, nur bei Port A = CRSF und FunkMode != 0)
  ├─ reedUpdate()                       (reed_trigger.cpp: RX/TX, Entprellung)
  ├─ processCommandQueue()              (command_queue.cpp: Reed + Web-Test)
  │    ├─► Sound_on_web[1..4] = true
  │    ├─► toggleMotorSound()           (motor_control.cpp)
  │    └─► flameToggle()                (flame_led.cpp)
  ├─ crsfLoopTasks()                    (crsf_bridge.cpp, nur bei Port A = CRSF)
  │    ├─► Pakete/Router, BUS_OK, channel_output[], uartPortsTelemetry()
  │    └─► Lua-Menü: Ping/Read/Write/Command-Replies
  ├─ ledArbiterUpdate(updateLed)        (status_led_arbiter.cpp: Flamme vor Status; Reed ohne Flamme = LED aus)
  ├─ saveConfigForce()                  (500ms nach letzter Änderung)
  ├─ wifiFailsafeCheck()
  ├─ Config()                           ← Sound_on[] aus CRSF-Kanälen setzen
  ├─ handleSound(1..4)                  (sound_trigger.cpp: One-Shot / Dauerbetrieb)
  ├─ motorControlLoop()                 (motor_control.cpp: CRSF-Toggle)
  └─ Motor-State-Machine + Throttle-Rampe
```

> Bei Port A = Reed/Aus ist `FunkMode_boot = 0`: `funk.loop()` und `crsfLoopTasks()`
> laufen nicht, die 16 CRSF-Kanäle bleiben ohne Wirkung. Reed-Aktionen und
> Web-Test laufen rein lokal. Der Funk ist nur bei Port A = CRSF aktiv.

---

## 6. Arduino-Loop und FreeRTOS-Tasks

Der Arduino-Framework auf dem ESP32 **ist selbst ein FreeRTOS-Task** – `loop()` läuft als
normaler Task im selben Scheduler wie die Audio-Tasks.

```
FreeRTOS-Scheduler (läuft immer)
│
├─ Task "loopTask"    (Core 1, Prio 1)  ← setup() + loop()
├─ Task "I2SProducer" (Core 1, Prio 2)  ← Audio-Mischer (FillBuffer/MixSamples)
├─ Task "I2SConsumer" (Core 1, Prio 2)  ← i2s_write() → DMA → MAX98357A
└─ Task "SDReader"    (Core 1, Prio 1)  ← LittleFS-Nachladen in BackSamples[]
```

### Warum funktioniert das?

| Eigenschaft | Erklärung |
|---|---|
| **Präemptives Scheduling** | FreeRTOS unterbricht Tasks nach Zeitscheibe oder wenn ein höherpriorer Task bereit ist |
| **Prioritäten** | I2SProducer/Consumer (Prio 2) laufen vor `loop()` (Prio 1) – das sichert gleichmäßiges Audio |
| **Synchronisation** | Geteilte Daten sind mit Mutexen/Semaphoren geschützt |

### Synchronisations-Mittel im Projekt

| Schutzmechanismus | Schützt |
|---|---|
| `PlaylistLockGuard` (rekursiver Mutex) | Playlist-Zeiger (`FirstPlayListItem`, `NextItem` …) |
| `sdMutex` | LittleFS-Zugriffe (`WavFile.read()`, `.seek()`, `.close()`) |
| `volatile BackReady`, `BackRequested` | Handshake zwischen SDReader und I2SProducer |
| `ringMutex` + Semaphoren | Ringbuffer zwischen I2SProducer und I2SConsumer |

### Was man beachten muss

> [!WARNING]
> Variablen, die sowohl aus `loop()` als auch aus einem Task heraus gelesen/geschrieben werden,
> müssen durch einen der obigen Mechanismen geschützt sein. Ungeschützte Zugriffe können je nach
> Timing stille Datenverfälschungen oder Knackser im Audio verursachen.

> [!CAUTION]
> Blockierende Aufrufe in `loop()` (z.B. `delay()`, langes Warten auf SD) verzögern zwar den
> Loop, **nicht aber die Audio-Tasks** – die laufen durch ihren eigenen Stack weiter. Umgekehrt
> dürfen High-Prio-Tasks keine Arduino-APIs aufrufen, die selbst Locks halten.

### Kurzfassung

```
loop()           ── steuert Logik, setzt Trigger-Flags, ruft I2SAudio.Play() auf
FreeRTOS-Tasks   ── liefern Audio kontinuierlich und unterbrechungsfrei
```

Das ist beim ESP32 das übliche Pattern für zeitkritische Ausgaben (Audio, PWM, Kommunikation)
neben normaler Anwendungslogik.
