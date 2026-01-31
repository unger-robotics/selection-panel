# Selection Panel: FreeRTOS-Firmware

10 Taster → 10 LEDs mit FreeRTOS-Task-Architektur am XIAO ESP32-S3.

## Übersicht

Diese Firmware erweitert das Selection Panel um eine FreeRTOS-basierte Architektur. Zwei Tasks teilen sich die Arbeit: IO-Verarbeitung und Serial-Ausgabe laufen unabhängig voneinander.

```
┌─────────────────────────────────────────────────────────────────────────┐
│                           FreeRTOS Scheduler                            │
├─────────────────────────────────────────────────────────────────────────┤
│                                                                         │
│   taskIO (Prio 1)                    taskSerial (Prio 10)              │
│   ┌─────────────────┐                ┌─────────────────┐               │
│   │ readButtons()   │                │                 │               │
│   │ debounce()      │ ──── Queue ───►│ Serial.print()  │               │
│   │ selectActive()  │   (LogEvent)   │                 │               │
│   │ writeLEDs()     │                │                 │               │
│   └─────────────────┘                └─────────────────┘               │
│         │                                    │                          │
│    5 ms Periode                        Wartet auf Queue                 │
│         │                                    │                          │
└─────────┼────────────────────────────────────┼──────────────────────────┘
          │                                    │
          ▼                                    ▼
     Hardware (SPI)                     USB-CDC Serial
```

## Warum FreeRTOS?

### Problem mit einfacher loop()

```cpp
void loop() {
    readButtons();
    debounce();
    writeLEDs();
    Serial.println(...);  // ← USB-CDC kann 10-100 ms blockieren!
    delay(5);             // ← Timing wird ungenau
}
```

USB-CDC-Ausgabe ist langsam und kann blockieren. Das stört das IO-Timing und verfälscht die Debounce-Logik.

### Lösung mit FreeRTOS

```cpp
taskIO:     vTaskDelayUntil() garantiert exaktes 5 ms Timing
            → Sendet Log-Events in Queue (nicht-blockierend)

taskSerial: Wartet auf Queue, gibt aus wenn CPU frei
            → IO läuft ungestört weiter
```

## Hardware

| Komponente | Anzahl | Funktion |
|------------|--------|----------|
| XIAO ESP32-S3 | 1 | Mikrocontroller |
| 74HC595 | 2 | LED-Treiber |
| CD4021B | 2 | Taster-Eingabe |
| LEDs | 10 | Ausgabe |
| Taster | 10 | Eingabe |

## Pin-Belegung

| GPIO | Signal | Funktion |
|------|--------|----------|
| D8 | SCK | SPI-Takt (gemeinsam) |
| D9 | MISO | Daten von CD4021B |
| D10 | MOSI | Daten zu 74HC595 |
| D1 | P/S | CD4021B Mode Select |
| D0 | RCK | 74HC595 Latch |
| D2 | OE | 74HC595 Helligkeit (PWM) |

## Konfiguration

Die Datei `src/config.h` enthält alle Parameter:

### Timing

```cpp
constexpr uint32_t IO_PERIOD_MS = 5;    // 200 Hz Abtastrate
constexpr uint32_t DEBOUNCE_MS = 30;    // Entprellzeit
```

### Verhalten

```cpp
// true:  Auswahl bleibt nach Loslassen bestehen
// false: Auswahl erlischt wenn kein Taster gedrückt
constexpr bool LATCH_SELECTION = true;
```

### Debug

```cpp
constexpr bool LOG_VERBOSE_PER_ID = true;   // Detaillierte Ausgabe
constexpr bool LOG_ON_RAW_CHANGE = false;   // Auch Prellen loggen
```

### FreeRTOS

```cpp
constexpr BaseType_t CORE_APP = 1;          // Core 0 für WiFi reserviert
constexpr UBaseType_t PRIO_IO = 1;          // IO: niedrige Priorität
constexpr UBaseType_t PRIO_SERIAL = 10;     // Serial: hohe Priorität
```

## Task-Architektur

### taskIO (Priorität 1)

- Läuft alle 5 ms (200 Hz)
- `vTaskDelayUntil()` garantiert konstante Periode
- Nicht-blockierend: Bei voller Queue wird Log verworfen

```
┌────────────────────────────────────────────────────────────────┐
│                        taskIO Zyklus                           │
├────────────────────────────────────────────────────────────────┤
│  vTaskDelayUntil()  ← Warten bis nächste 5 ms Periode          │
│         │                                                      │
│         ▼                                                      │
│  readButtons()      ← SPI-Transfer, First-Bit-Korrektur        │
│         │                                                      │
│         ▼                                                      │
│  debounceUpdate()   ← Zeitbasiertes Entprellen                 │
│         │                                                      │
│         ▼                                                      │
│  selectFromEdges()  ← One-Hot Auswahl                          │
│         │                                                      │
│         ▼                                                      │
│  writeLEDs()        ← SPI-Transfer, Latch                      │
│         │                                                      │
│         ▼                                                      │
│  xQueueSend()       ← Log-Event (timeout=0, nicht-blockierend) │
│         │                                                      │
│         └──────────────────────────────────────────────────────┘
```

### taskSerial (Priorität 10)

- Höhere Priorität, aber blockiert auf Queue
- Gibt Log-Events aus sobald verfügbar
- `vTaskDelay(1)` nach Ausgabe gibt CPU frei

```
┌────────────────────────────────────────────────────────────────┐
│                      taskSerial Zyklus                         │
├────────────────────────────────────────────────────────────────┤
│  xQueueReceive()    ← Blockiert bis Event in Queue             │
│         │                                                      │
│         ▼                                                      │
│  Serial.print(...)  ← Ausgabe (kann langsam sein)              │
│         │                                                      │
│         ▼                                                      │
│  vTaskDelay(1)      ← CPU für andere Tasks freigeben           │
│         │                                                      │
│         └──────────────────────────────────────────────────────┘
```

## Queue-Mechanismus

```cpp
struct LogEvent {
    uint32_t timestamp;
    uint8_t btnRaw[2];
    uint8_t btnDebounced[2];
    uint8_t ledState[2];
    uint8_t activeId;
    bool rawChanged;
    bool debChanged;
    bool activeChanged;
};

// IO-Task: Schreibt ohne Warten (timeout=0)
xQueueSend(logQueue, &event, 0);

// Serial-Task: Wartet unbegrenzt auf Event
xQueueReceive(logQueue, &event, portMAX_DELAY);
```

Bei voller Queue (8 Events) werden neue Events verworfen. Das IO-Timing bleibt stabil, nur Logging geht verloren.

## Build & Upload

```bash
pio run -t upload
pio device monitor
```

## Erwartete Ausgabe

```
========================================
Selection Panel: FreeRTOS + One-Hot
========================================
IO_PERIOD_MS:    5
DEBOUNCE_MS:     30
LATCH_SELECTION: true
---
BTN RAW:    IC0: 01111111 (0x7F) | IC1: 11111111 (0xFF)
BTN DEB:    IC0: 01111111 (0x7F) | IC1: 11111111 (0xFF)
Active LED (One-Hot): 1
LED STATE:  IC0: 00000001 (0x01) | IC1: 00000000 (0x00)
Pressed IDs: 1
Buttons per ID (RAW/DEB)  [pressed=1 | released=0]
  T01  IC0 b7   RAW=1  DEB=1
  T02  IC0 b6   RAW=0  DEB=0
  ...
LEDs per ID (state)       [on=1 | off=0]
  L01  IC0 b0   1
  L02  IC0 b1   0
  ...
```

## Projektstruktur

```
.
├── README.md
├── platformio.ini
└── src/
    ├── config.h      # Hardware + FreeRTOS Konfiguration
    └── main.cpp      # Tasks, IO-Logik, Debounce
```
