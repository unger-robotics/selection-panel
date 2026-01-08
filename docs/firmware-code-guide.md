# Selection Panel: Firmware Code Guide

Version 2.5.0 | ESP32-S3 (XIAO) + FreeRTOS

## Projektstruktur

```
firmware/
├── include/
│   ├── bitops.h        # Bit-Adressierung für Taster/LEDs
│   ├── config.h        # Zentrale Konfiguration
│   └── types.h         # Gemeinsame Datentypen
├── src/
│   ├── main.cpp        # Entry Point
│   ├── app/
│   │   ├── io_task.cpp/.h      # I/O-Verarbeitung
│   │   └── serial_task.cpp/.h  # Protokoll-Handler
│   ├── drivers/
│   │   ├── cd4021.cpp/.h       # Taster-Schieberegister
│   │   └── hc595.cpp/.h        # LED-Schieberegister
│   ├── hal/
│   │   └── spi_bus.cpp/.h      # SPI-Abstraktion
│   └── logic/
│       ├── debounce.cpp/.h     # Entprellung
│       └── selection.cpp/.h    # Auswahl-Logik
└── platformio.ini
```

## Schichtenmodell

Die Firmware folgt einem strikten Schichtenmodell. Abhängigkeiten zeigen nur nach unten:

```
┌─────────────────────────────────────────────────────┐
│                    main.cpp                         │
│           Queue erstellen, Tasks starten            │
└─────────────────────────┬───────────────────────────┘
                          │
┌─────────────────────────┴───────────────────────────┐
│                    app/                             │
│        io_task.cpp          serial_task.cpp         │
│     (Hardware-Zyklus)     (Protokoll-Handler)       │
└─────────────────────────┬───────────────────────────┘
                          │
┌─────────────────────────┴───────────────────────────┐
│                   logic/                            │
│      debounce.cpp              selection.cpp        │
│   (Zeitbasierte Entprellung)  (One-Hot Auswahl)     │
└─────────────────────────┬───────────────────────────┘
                          │
┌─────────────────────────┴───────────────────────────┐
│                  drivers/                           │
│        cd4021.cpp               hc595.cpp           │
│    (Taster-Schieberegister) (LED-Schieberegister)   │
└─────────────────────────┬───────────────────────────┘
                          │
┌─────────────────────────┴───────────────────────────┐
│                    hal/                             │
│                  spi_bus.cpp                        │
│           (Mutex-geschützte SPI-Abstraktion)        │
└─────────────────────────────────────────────────────┘
```

## Modul-Referenz

### config.h – Zentrale Konfiguration

Alle Hardware- und Timing-Parameter an einer Stelle:

```cpp
// Anzahl der Ein-/Ausgänge
constexpr uint8_t BTN_COUNT = 10;
constexpr uint8_t LED_COUNT = 10;

// Bytes für Bit-Arrays (aufrunden: 10 Bits → 2 Bytes)
constexpr size_t BTN_BYTES = (BTN_COUNT + 7) / 8;
constexpr size_t LED_BYTES = (LED_COUNT + 7) / 8;

// Pin-Zuordnung
constexpr int PIN_SCK = D8;       // SPI-Takt
constexpr int PIN_BTN_PS = D1;    // CD4021B: Parallel/Serial Select
constexpr int PIN_BTN_MISO = D9;  // CD4021B: Daten (Q8)
constexpr int PIN_LED_MOSI = D10; // 74HC595: Daten (SER)
constexpr int PIN_LED_RCK = D0;   // 74HC595: Latch (STCP)
constexpr int PIN_LED_OE = D2;    // 74HC595: Output Enable (PWM)

// Timing
constexpr uint32_t IO_PERIOD_MS = 5;   // 200 Hz Abtastrate
constexpr uint32_t DEBOUNCE_MS = 30;   // Entprellzeit

// SPI-Einstellungen
constexpr uint32_t SPI_HZ_BTN = 500000UL;  // 500 kHz
constexpr uint32_t SPI_HZ_LED = 1000000UL; // 1 MHz
constexpr uint8_t SPI_MODE_BTN = SPI_MODE1;
constexpr uint8_t SPI_MODE_LED = SPI_MODE0;
```

### bitops.h – Bit-Adressierung

Abstrahiert die unterschiedlichen Bit-Reihenfolgen der Schieberegister:

```cpp
// CD4021B: MSB-first (ID 1 → Bit 7)
static inline uint8_t btn_byte(uint8_t id) { return (id - 1) / 8; }
static inline uint8_t btn_bit(uint8_t id)  { return 7 - ((id - 1) % 8); }

// 74HC595: LSB-first (ID 1 → Bit 0)
static inline uint8_t led_byte(uint8_t id) { return (id - 1) / 8; }
static inline uint8_t led_bit(uint8_t id)  { return (id - 1) % 8; }

// Active-Low Hilfsfunktionen (Taster: 0 = gedrückt)
static inline bool activeLow_pressed(const uint8_t* arr, uint8_t id);
static inline void activeLow_setPressed(uint8_t* arr, uint8_t id, bool pressed);

// LED-Hilfsfunktionen
static inline bool led_on(const uint8_t* arr, uint8_t id);
static inline void led_set(uint8_t* arr, uint8_t id, bool on);
```

### types.h – Gemeinsame Datentypen

```cpp
struct LogEvent {
    uint32_t ms;                // Zeitstempel
    uint8_t raw[BTN_BYTES];     // Rohzustand der Taster
    uint8_t deb[BTN_BYTES];     // Entprellter Zustand
    uint8_t led[LED_BYTES];     // LED-Ausgabezustand
    uint8_t activeId;           // Aktive Auswahl (0 = keine, 1-10 = ID)
    bool rawChanged;            // Flag: Raw hat sich geändert
    bool debChanged;            // Flag: Debounced hat sich geändert
    bool activeChanged;         // Flag: Auswahl hat sich geändert
};
```

### spi_bus.h – HAL-Schicht

```cpp
class SpiBus {
public:
    void begin(int sck, int miso, int mosi);
    void lock();    // Mutex nehmen
    void unlock();  // Mutex freigeben
private:
    SemaphoreHandle_t mtx_;
};

// RAII Guard für automatisches Cleanup
class SpiGuard {
public:
    SpiGuard(SpiBus& bus, const SPISettings& settings);
    ~SpiGuard();  // Automatisch: endTransaction() + unlock()
};
```

### cd4021.h – Taster-Treiber

```cpp
class Cd4021 {
public:
    void init();
    void readRaw(SpiBus& bus, uint8_t* out);  // Liest alle Taster
private:
    SPISettings spi_{SPI_HZ_BTN, MSBFIRST, SPI_MODE_BTN};
};
```

Der Treiber löst das First-Bit-Problem: Nach Parallel-Load liegt Q8 sofort an, bevor der erste Clock kommt. Das erste Bit wird daher per `digitalRead()` vor dem SPI-Transfer gesichert.

### hc595.h – LED-Treiber

```cpp
class Hc595 {
public:
    void init();
    void setBrightness(uint8_t percent);  // PWM via OE
    void write(SpiBus& bus, uint8_t* state);
private:
    void maskUnused(uint8_t* state);  // Ghost-LEDs verhindern
    void latch();  // Shift → Output Register
    SPISettings spi_{SPI_HZ_LED, MSBFIRST, SPI_MODE_LED};
};
```

### debounce.h – Entprellung

```cpp
class Debouncer {
public:
    void init();
    bool update(uint32_t nowMs, const uint8_t* raw, uint8_t* deb);
private:
    uint8_t rawPrev_[BTN_BYTES];
    uint32_t lastChange_[BTN_COUNT];  // Timer pro Taster
};
```

Der Algorithmus ist zeitbasiert:

1. Bei Rohwert-Änderung: Timer zurücksetzen
2. Wenn Timer ≥ 30 ms UND Rohwert ≠ entprellt: übernehmen

### selection.h – Auswahl-Logik

```cpp
class Selection {
public:
    void init();
    bool update(const uint8_t* debNow, uint8_t& activeId);
private:
    uint8_t debPrev_[BTN_BYTES];
};
```

Prinzip: "Last Press Wins" – der zuletzt gedrückte Taster wird aktiv. Mit `LATCH_SELECTION=true` bleibt die Auswahl nach Loslassen bestehen.

### io_task.h – I/O-Zyklus

```cpp
void start_io_task(QueueHandle_t logQueue);
```

Der Task läuft mit 200 Hz und führt pro Zyklus aus:

1. Taster lesen (CD4021B)
2. Entprellen
3. Auswahl aktualisieren
4. LED-Zustand schreiben (74HC595)
5. Bei Änderung: LogEvent in Queue senden

### serial_task.h – Protokoll-Handler

```cpp
enum LedCommand : uint8_t {
    LED_CMD_SET,    // One-hot
    LED_CMD_ON,     // Additiv ein
    LED_CMD_OFF,    // Additiv aus
    LED_CMD_CLEAR,  // Alle aus
    LED_CMD_ALL     // Alle an
};

typedef void (*LedControlCallback)(LedCommand cmd, uint8_t id);

void start_serial_task(QueueHandle_t logQueue);
void set_led_callback(LedControlCallback callback);
```

## FreeRTOS-Konfiguration

| Task | Core | Priorität | Periode | Funktion |
|------|------|-----------|---------|----------|
| io_task | 1 | 5 | 5 ms | Hardware-I/O |
| serial_task | 1 | 2 | Event-driven | Protokoll |

Core 0 bleibt für WiFi/BLE reserviert (falls später benötigt).

## Datenfluss im Detail

```cpp
// io_task.cpp – Hauptschleife (vereinfacht)
void io_loop() {
    TickType_t lastWake = xTaskGetTickCount();

    while (true) {
        uint32_t now = millis();

        // 1. Taster einlesen
        cd4021.readRaw(bus, raw);
        bool rawChg = memcmp(raw, rawPrev, BTN_BYTES) != 0;

        // 2. Entprellen
        bool debChg = debouncer.update(now, raw, deb);

        // 3. Auswahl aktualisieren
        bool selChg = selection.update(deb, activeId);

        // 4. LEDs setzen (activeId → One-Hot)
        memset(led, 0, LED_BYTES);
        if (activeId > 0) led_set(led, activeId, true);
        hc595.write(bus, led);

        // 5. Bei Änderung: Event senden
        if (debChg || selChg) {
            LogEvent ev = {now, raw, deb, led, activeId, ...};
            xQueueSend(queue, &ev, 0);
        }

        // 6. Auf nächsten Zyklus warten
        vTaskDelayUntil(&lastWake, pdMS_TO_TICKS(IO_PERIOD_MS));
    }
}
```

## Design-Entscheidungen

### Warum Queue statt direktem Serial-Aufruf?

Die Queue entkoppelt I/O von Serial-Ausgabe:

- io_task blockiert nie (5 ms Deadline)
- serial_task kann langsam sein (USB-Puffer voll)
- Atomare Snapshots (keine Race-Conditions)

### Warum SpiGuard (RAII)?

Garantiert korrektes Cleanup auch bei frühem Return:

```cpp
{
    SpiGuard g(bus, settings);
    if (error) return;  // endTransaction() + unlock() trotzdem!
    SPI.transfer(data);
}
```

### Warum zeitbasiertes Debouncing?

- Unabhängig von Abtastrate
- Jeder Taster hat eigenen Timer
- Schnelle Tastenfolgen möglich

### Warum LED_REFRESH_EVERY_CYCLE?

Beim CD4021-Read werden Nullen durch den 74HC595 getaktet. Ein glitchender Latch-Pin könnte LEDs kurz ausschalten. Der Refresh kompensiert dies.

## Erweiterung: 100 Buttons

Die Architektur skaliert durch Änderung zweier Konstanten:

```cpp
constexpr uint8_t BTN_COUNT = 100;
constexpr uint8_t LED_COUNT = 100;
// BTN_BYTES und LED_BYTES werden automatisch auf 13 berechnet
```

Alle Schleifen und Bit-Arrays passen sich automatisch an. Die SPI-Transferzeit steigt von ~20 µs auf ~260 µs – weit unter dem 5 ms Budget.

## Build und Upload

```bash
# PlatformIO
cd firmware
pio run -t upload
pio device monitor

# Serial-Ausgabe
# READY
# FW SelectionPanel v2.5.0
# PRESS 001
# RELEASE 001
```

## Debugging

### Log-Level aktivieren

In `config.h`:

```cpp
constexpr bool LOG_VERBOSE_PER_ID = true;   // Details pro Taster
constexpr bool LOG_ON_RAW_CHANGE = true;    // Rohwert-Änderungen
constexpr bool SERIAL_PROTOCOL_ONLY = false; // Debug-Ausgabe erlauben
```
