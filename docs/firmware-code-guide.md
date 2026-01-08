# Selection Panel: Firmware Code Guide

Version 2.5.2 | ESP32-S3 (XIAO) + FreeRTOS

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
┌─────────────────────────┴───────────────────────────┘
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

// Pin-Zuordnung (gemeinsamer SPI-Bus)
constexpr int PIN_SCK      = D8;   // SPI-Takt (shared)
constexpr int PIN_BTN_PS   = D1;   // CD4021B: Parallel/Serial Select
constexpr int PIN_BTN_MISO = D9;   // CD4021B: Daten (Q8)
constexpr int PIN_LED_MOSI = D10;  // 74HC595: Daten (SER)
constexpr int PIN_LED_RCK  = D0;   // 74HC595: Latch (RCLK)
constexpr int PIN_LED_OE   = D2;   // 74HC595: Output Enable (PWM)

// Timing
constexpr uint32_t IO_PERIOD_MS = 5;   // 200 Hz Abtastrate
constexpr uint32_t DEBOUNCE_MS = 30;   // Entprellzeit

// SPI-Einstellungen
constexpr uint32_t SPI_HZ_BTN = 500000UL;  // 500 kHz (CD4021B)
constexpr uint32_t SPI_HZ_LED = 1000000UL; // 1 MHz (74HC595)
constexpr uint8_t SPI_MODE_BTN = SPI_MODE1; // CPOL=0, CPHA=1
constexpr uint8_t SPI_MODE_LED = SPI_MODE0; // CPOL=0, CPHA=0
```

### bitops.h – Bit-Adressierung

Abstrahiert die Hardware-Verdrahtung der Schieberegister:

```cpp
// ============================================================================
// CD4021B Button Bit-Mapping
// ============================================================================
// Hardware-Verdrahtung: BTN 1 → PI-8 (Pin 1), BTN 8 → PI-1 (Pin 7)
// CD4021B gibt MSB-first aus: PI-1 zuerst, PI-8 zuletzt
// Resultat im empfangenen Byte:
//   - BTN 1 (PI-8) erscheint als Bit 0
//   - BTN 8 (PI-1) erscheint als Bit 7
//
// Formel: btn_bit(id) = (id - 1) % 8
//   btn_bit(1) = 0  ← BTN 1 → Bit 0
//   btn_bit(8) = 7  ← BTN 8 → Bit 7

static inline uint8_t btn_byte(uint8_t id) { return (id - 1) / 8; }
static inline uint8_t btn_bit(uint8_t id)  { return (id - 1) % 8; }

// ============================================================================
// 74HC595 LED Bit-Mapping
// ============================================================================
// Hardware-Verdrahtung: LED 1 → QA (Bit 0), LED 8 → QH (Bit 7)
// 74HC595 erwartet LSB-first wenn wir MSBFIRST SPI verwenden
//
// Formel: led_bit(id) = (id - 1) % 8
//   led_bit(1) = 0  ← LED 1 → Bit 0
//   led_bit(8) = 7  ← LED 8 → Bit 7

static inline uint8_t led_byte(uint8_t id) { return (id - 1) / 8; }
static inline uint8_t led_bit(uint8_t id)  { return (id - 1) % 8; }

// ============================================================================
// Active-Low Hilfsfunktionen (Taster: 0 = gedrückt, Pull-up!)
// ============================================================================
static inline bool activeLow_pressed(const uint8_t* arr, uint8_t id) {
    return !((arr[btn_byte(id)] >> btn_bit(id)) & 1);
}

static inline void activeLow_setPressed(uint8_t* arr, uint8_t id, bool pressed) {
    uint8_t mask = 1 << btn_bit(id);
    if (pressed) arr[btn_byte(id)] &= ~mask;  // 0 = gedrückt
    else         arr[btn_byte(id)] |= mask;   // 1 = losgelassen
}

// ============================================================================
// LED-Hilfsfunktionen
// ============================================================================
static inline bool led_on(const uint8_t* arr, uint8_t id) {
    return (arr[led_byte(id)] >> led_bit(id)) & 1;
}

static inline void led_set(uint8_t* arr, uint8_t id, bool on) {
    uint8_t mask = 1 << led_bit(id);
    if (on) arr[led_byte(id)] |= mask;
    else    arr[led_byte(id)] &= ~mask;
}
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

**First-Bit-Problem:** Nach Parallel-Load (P/S HIGH→LOW) liegt das erste Bit (PI-1) bereits an Q8 an – bevor der erste Clock kommt! Die Lösung:

```cpp
void Cd4021::readRaw(SpiBus& bus, uint8_t* out) {
    // 1. Parallel Load
    digitalWrite(PIN_BTN_PS, HIGH);
    delayMicroseconds(2);
    digitalWrite(PIN_BTN_PS, LOW);
    delayMicroseconds(2);

    // 2. First Bit Rescue: PI-1 liegt bereits an Q8!
    uint8_t firstBit = digitalRead(PIN_BTN_MISO);

    // 3. SPI Transfer (restliche Bits)
    SpiGuard g(bus, spi_);
    SPI.transfer(out, BTN_BYTES);

    // 4. First Bit einsetzen (MSB von Byte 0)
    out[0] = (out[0] >> 1) | (firstBit << 7);
}
```

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

Beim CD4021B-Read werden Nullen durch den 74HC595 getaktet (gemeinsamer SCK). Ein glitchender Latch-Pin könnte LEDs kurz ausschalten. Der Refresh nach jedem Zyklus kompensiert dies.

## Erweiterung: 100 Buttons

Die Architektur skaliert durch Änderung zweier Konstanten:

```cpp
constexpr uint8_t BTN_COUNT = 100;
constexpr uint8_t LED_COUNT = 100;
// BTN_BYTES und LED_BYTES werden automatisch auf 13 berechnet
```

Alle Schleifen und Bit-Arrays passen sich automatisch an. Die SPI-Transferzeit steigt von ~40 µs auf ~320 µs – weit unter dem 5 ms Budget.

## Build und Upload

```bash
# PlatformIO
cd firmware
pio run -t upload
pio device monitor

# Serial-Ausgabe
# READY
# FW SelectionPanel v2.5.2
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

### Bit-Mapping verifizieren

```bash
# STATUS-Befehl zeigt Bit-Arrays:
echo "STATUS" > /dev/serial/by-id/usb-Espressif*

# Ausgabe:
# LEDS 0000000001    ← LED 1 an (Bit 0)
# BTNS 1111111110    ← BTN 1 gedrückt (Active-Low: Bit 0 = 0)
```
