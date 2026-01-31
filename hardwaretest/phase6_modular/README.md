# Selection Panel: Modulare FreeRTOS-Architektur

10 Taster → 10 LEDs mit sauberer Schichtentrennung am XIAO ESP32-S3.

## Übersicht

Diese Firmware demonstriert eine modulare Architektur für Embedded-Systeme. Jede Schicht hat eine klare Verantwortung, Abhängigkeiten fließen nur nach unten.

```
┌─────────────────────────────────────────────────────────────────────────────┐
│                              main.cpp                                       │
│                         (Entry Point, Verdrahtung)                          │
└───────────────────────────────────┬─────────────────────────────────────────┘
                                    │
                                    ▼
┌─────────────────────────────────────────────────────────────────────────────┐
│                               app/                                          │
│                    (FreeRTOS Tasks: io_task, serial_task)                   │
└───────────────────────────────────┬─────────────────────────────────────────┘
                                    │
                    ┌───────────────┴───────────────┐
                    ▼                               ▼
┌───────────────────────────────┐   ┌─────────────────────────────────────────┐
│            logic/             │   │               drivers/                  │
│  (debounce, selection)        │   │         (cd4021, hc595)                 │
└───────────────────────────────┘   └───────────────────────┬─────────────────┘
                                                            │
                                                            ▼
                                    ┌─────────────────────────────────────────┐
                                    │                 hal/                    │
                                    │              (spi_bus)                  │
                                    └─────────────────────────────────────────┘
                                                            │
                                                            ▼
                                    ┌─────────────────────────────────────────┐
                                    │               Hardware                  │
                                    │    (SPI, GPIO, PWM, CD4021B, 74HC595)   │
                                    └─────────────────────────────────────────┘
```

## Projektstruktur

```
.
├── include/
│   ├── bitops.h          # Bit-Adressierung (MSB/LSB, Active-Low)
│   ├── config.h          # Zentrale Konfiguration
│   └── types.h           # Gemeinsame Datentypen (LogEvent)
├── src/
│   ├── app/
│   │   ├── io_task.cpp/h     # FreeRTOS Task: Hardware-IO
│   │   └── serial_task.cpp/h # FreeRTOS Task: Debug-Ausgabe
│   ├── drivers/
│   │   ├── cd4021.cpp/h      # Taster-Treiber
│   │   └── hc595.cpp/h       # LED-Treiber
│   ├── hal/
│   │   └── spi_bus.cpp/h     # SPI-Abstraktion mit Mutex
│   ├── logic/
│   │   ├── debounce.cpp/h    # Zeitbasiertes Entprellen
│   │   └── selection.cpp/h   # One-Hot Auswahl
│   └── main.cpp              # Entry Point
├── platformio.ini
└── README.md
```

## Schichten im Detail

### HAL (Hardware Abstraction Layer)

**spi_bus**: Kapselt den SPI-Bus mit FreeRTOS Mutex.

```cpp
class SpiBus {
    void begin(sck, miso, mosi);
    void lock();    // Mutex nehmen
    void unlock();  // Mutex freigeben
};

// RAII Guard für sichere Transaktionen
class SpiGuard {
    SpiGuard(bus, settings);   // lock + beginTransaction
    ~SpiGuard();               // endTransaction + unlock
};
```

**Warum Mutex?** Falls später mehrere Tasks auf SPI zugreifen (z.B. SD-Karte), ist der Bus geschützt.

### Drivers

**cd4021**: Treiber für CD4021B Schieberegister (Taster-Eingabe).

```cpp
class Cd4021 {
    void init();
    void readRaw(bus, out);  // Mit First-Bit-Korrektur
};
```

**hc595**: Treiber für 74HC595 Schieberegister (LED-Ausgabe).

```cpp
class Hc595 {
    void init();
    void setBrightness(percent);
    void write(bus, state);  // Mit Ghost-Maskierung
};
```

### Logic

**debounce**: Zeitbasiertes Entprellen.

```cpp
class Debouncer {
    void init();
    bool update(nowMs, raw, deb);  // true wenn Änderung
};
```

**selection**: One-Hot Auswahl ("last press wins").

```cpp
class Selection {
    void init();
    bool update(debNow, activeId);  // true wenn Änderung
};
```

### App (FreeRTOS Tasks)

**io_task**: Periodischer Hardware-Zugriff.

```
Alle 5 ms:
  1. readButtons()      → btnRaw
  2. debounce()         → btnDebounced
  3. selection()        → activeId
  4. writeLEDs()        → Hardware
  5. xQueueSend()       → logQueue (nicht-blockierend)
```

**serial_task**: Debug-Ausgabe aus Queue.

```
Endlos:
  1. xQueueReceive()    ← Blockiert bis Event
  2. Serial.print()     → USB-CDC
  3. vTaskDelay(1)      → CPU freigeben
```

## Datenfluss

```
Hardware                 Firmware                              Hardware
────────                 ────────                              ────────

Taster ──────────────────► Cd4021::readRaw()
                               │
                               ▼
                           btnRaw[]
                               │
                               ▼
                           Debouncer::update()
                               │
                               ▼
                           btnDebounced[]
                               │
                               ▼
                           Selection::update()
                               │
                               ▼
                           activeId
                               │
                               ▼
                           Hc595::write() ─────────────────────► LEDs
                               │
                               ▼
                           LogEvent ────► Queue ────► Serial
```

## Konfiguration

Alle Parameter in `include/config.h`:

```cpp
// Timing
constexpr uint32_t IO_PERIOD_MS = 5;    // 200 Hz Abtastrate
constexpr uint32_t DEBOUNCE_MS = 30;    // Entprellzeit

// Verhalten
constexpr bool LATCH_SELECTION = true;  // Auswahl bleibt nach Loslassen

// Debug
constexpr bool LOG_VERBOSE_PER_ID = true;   // Detaillierte Ausgabe
constexpr bool LOG_ON_RAW_CHANGE = false;   // Auch Prellen zeigen
```

## Vorteile der Modularität

| Aspekt | Vorteil |
|--------|---------|
| **Testbarkeit** | Logic-Module ohne Hardware testbar |
| **Wartbarkeit** | Änderungen in einer Schicht betreffen andere nicht |
| **Wiederverwendung** | Debouncer in anderem Projekt nutzbar |
| **Lesbarkeit** | Kleine Dateien, klare Verantwortung |
| **Skalierbarkeit** | Neue Treiber hinzufügen ohne Umbau |

## Build & Upload

```bash
pio run -t upload
pio device monitor
```

## Erwartete Ausgabe

```
========================================
Selection Panel: Modular + FreeRTOS
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
  ...
```

## Nächste Schritte

- [ ] Skalierung auf 100 Buttons/LEDs
