# Soundmodul Mini – Ablauf-Übersicht

## 1. Trigger-Quellen

```
CRSF-Bus (Port A)          Reed-Trigger (IO5/IO6)       Web / Lua-Menü
       │                          │                            │
  Config()                reedTriggerUpdate()           Sound_on_web[i]=true
  wertet Kanäle aus        IO6: kurz sound1 / doppelt Motor    (einmaliger Trigger)
       │                   IO5: kurz sound2 / doppelt LED          │
  Sound_on[i] = true/false        │                            │
       └──────────────────────────┴────────────────────────────┘
                                  │
                             handleSound(i)
```

> [!NOTE]
> `Sound_on[]` wird jeden Loop-Durchlauf neu gesetzt (dauerhaft, solange Quelle aktiv).
> `Sound_on_web[]` ist ein **One-Shot-Flag**: `handleSound` setzt es sofort wieder auf `false`.

---

## 2. handleSound – Ablauf für Sound 1–4

```
handleSound(i, &SoundX)
│
├─ Sound_on[i] || Sound_on_web[i] ?
│   ├─ JA, noch nicht laufend (Sound_play[i]==false):
│   │     SoundX.LoadWavFile()
│   │     if Mode==Loop: SoundX.RepeatForever = true
│   │     I2SAudio.Play(&SoundX)
│   │     Sound_play[i] = true
│   │     Sound_on_web[i] = false          ← One-Shot zurücksetzen
│   │
│   └─ JA, läuft bereits: nichts tun (läuft weiter)
│
└─ Sound_on[i] == false && Sound_on_web[i] == false ?
    ├─ RepeatForever = false
    ├─ Mode==Tippbetrieb: I2SAudio.Stop(&SoundX)
    └─ SoundX.Playing == false && Sound_play[i] == true:
          SoundX.UnLoadWavFile()
          Sound_play[i] = false
          testSoundActive[i] = false
```

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
> Die Drehzahl-Variation erfordert das dynamische Gas-Signal eines RC-Empfängers (CRSF). Wird das Modul mit Reed-Kontakten betrieben (`REED_TRIGGER_ENABLED`), sind die UART-Pins für Reed-Eingänge belegt; der Motor läuft mangels Gaskanal dauerhaft mit der Mindestdrehzahl (`Drehzahl min`).

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
| `I2SAudio.Play()` → `Init()` → erster LittleFS-Read | ~1–5 ms |
| I2SProducer füllt nächsten Slot | ~1 ms |
| I2S-DMA → Lautsprecher | < 1 ms |
| **Gesamt** | **~3–7 ms** |

---

## 5. Zusammenspiel im loop()

```
loop()
  ├─ WebServerManager::Webpage()       (nur wenn AP aktiv)
  ├─ uartPortsUpdate()                 (Port B/C: S.Port, GPS)
  ├─ crsf.read_packets()               (Port A – entfällt bei REED_TRIGGER_ENABLED)
  │    └─► BUS_OK, channel_output[]
  ├─ reedTriggerUpdate()               (nur bei REED_TRIGGER_ENABLED)
  │    └─► IO6: kurz sound1 / doppelt Motorsound-Toggle,
  │         IO5: kurz sound2 / doppelt LED-Toggle (Flammen-Effekt)
  ├─ uartPortsTelemetry() / updateLed()
  ├─ CRSF Lua-Menü (Read/Write/Command)
  ├─ saveConfigForce()                 (500ms nach letzter Änderung)
  ├─ wifiFailsafeCheck()
  ├─ Config()                          ← Sound_on[] aus CRSF-Kanälen setzen
  ├─ handleSound(1..4)                 ← One-Shot / Dauerbetrieb auswerten
  └─ Motor-State-Machine + Throttle-Rampe
```

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
