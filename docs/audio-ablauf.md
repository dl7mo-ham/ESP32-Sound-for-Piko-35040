# Soundmodul Mini – Ablauf-Übersicht

## 1. Trigger-Quellen

```
CRSF-Bus (Port A)          Reed-Kontakt (IO5/IO6)       Web / Lua-Menü
       │                          │                            │
  Config()                reedTriggerUpdate()           Sound_on_web[i]=true
  wertet Kanäle aus        erkennt LOW-Flanke              (einmaliger Trigger)
       │                          │                            │
  Sound_on[i] = true/false   Sound_on_web[i] = true          │
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

> [!TIP]
> Throttle-Rampe (`ramp_throttle`) begrenzt die Gasänderungsrate auf `config.throttle_ramp` %/s,
> damit der Motor-Loop-Sound gleichmäßig in der Tonhöhe folgt.

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
  │    └─► Sound_on_web[1/2] = true
  ├─ uartPortsTelemetry() / updateLed()
  ├─ CRSF Lua-Menü (Read/Write/Command)
  ├─ saveConfigForce()                 (500ms nach letzter Änderung)
  ├─ wifiFailsafeCheck()
  ├─ Config()                          ← Sound_on[] aus CRSF-Kanälen setzen
  ├─ handleSound(1..4)                 ← One-Shot / Dauerbetrieb auswerten
  └─ Motor-State-Machine + Throttle-Rampe
```
