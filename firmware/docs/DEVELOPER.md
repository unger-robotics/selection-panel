# Selection Panel Firmware - Entwickler-Dokumentation

Technische Details fuer Firmware-Entwickler.

## Datenfluss

```
┌─────────────────────────────────────────────────────────────────────────────┐
│                              IO-Task (200 Hz)                               │
│                                                                             │
│  ┌──────────┐    ┌───────────┐    ┌───────────┐    ┌──────────┐            │
│  │ CD4021B  │───▶│ Debouncer │───▶│ Selection │───▶│  HC595   │            │
│  │ readRaw  │    │  update   │    │  update   │    │  write   │            │
│  └──────────┘    └───────────┘    └───────────┘    └──────────┘            │
│       │               │                │                                    │
│       ▼               ▼                ▼                                    │
│  _btn_raw[]      _btn_debounced[]   _active_id                             │
│                                                                             │
│                         │                                                   │
│                         ▼                                                   │
│                  ┌─────────────┐                                            │
│                  │ log_event_t │                                            │
│                  └──────┬──────┘                                            │
│                         │ xQueueSend (non-blocking)                         │
└─────────────────────────┼───────────────────────────────────────────────────┘
                          │
                          ▼
                   ┌─────────────┐
                   │   Queue     │
                   │ (32 Events) │
                   └──────┬──────┘
                          │
┌─────────────────────────┼───────────────────────────────────────────────────┐
│                         ▼                    Serial-Task                    │
│                  ┌─────────────┐                                            │
│                  │ log_event_t │                                            │
│                  └──────┬──────┘                                            │
│                         │                                                   │
│           ┌─────────────┼─────────────┐                                     │
│           ▼             ▼             ▼                                     │
│     active_changed?  PRESS 001   RELEASE 001                                │
│                         │             │                                     │
│                         ▼             ▼                                     │
│                      USB-CDC Serial (115200)                                │
│                              │                                              │
│                              ▼                                              │
│                      Raspberry Pi                                           │
└─────────────────────────────────────────────────────────────────────────────┘
```

## Timing

### IO-Task Zyklus (5 ms = 200 Hz)

```
Zeit (us)    Aktion
─────────────────────────────────────────
    0        vTaskDelayUntil() kehrt zurueck
   10        LED-Befehle aus Queue verarbeiten
   50        CD4021B Parallel-Load (5 us Hold)
   60        CD4021B digitalRead (First-Bit)
  100        CD4021B SPI Transfer (2 Bytes @ 500 kHz = 32 us)
  150        Debouncer.update()
  200        Selection.update()
  250        HC595 SPI Transfer (2 Bytes @ 1 MHz = 16 us)
  280        HC595 Latch
  300        log_event_t erstellen und senden
  350        Zustaende fuer naechsten Zyklus kopieren
 ~400        Zyklus Ende (< 1 ms, 4.6 ms Reserve)
```

### Debounce-Timing

```
Rohwert:     ─┐ ┌─┐ ┌─┐ ┌───────────────────────
              └─┘ └─┘ └─┘
              |<--Prellen-->|

Timer:       ──────────────┤ 30 ms ├────────────
                           Reset    Expired

Debounced:   ─────────────────────────┐
                                      └─────────
                                      ^
                                      Uebernahme
```

## Speicher-Layout

### Taster (CD4021B) - MSB-first

```
Byte 0:  [T1][T2][T3][T4][T5][T6][T7][T8]
          b7  b6  b5  b4  b3  b2  b1  b0

Byte 1:  [T9][T10][ ][ ][ ][ ][ ][ ]
          b7  b6  b5  b4  b3  b2  b1  b0

Active-Low: 0 = gedrueckt, 1 = losgelassen
```

### LEDs (74HC595) - LSB-first

```
Byte 0:  [L8][L7][L6][L5][L4][L3][L2][L1]
          b7  b6  b5  b4  b3  b2  b1  b0

Byte 1:  [ ][ ][ ][ ][ ][ ][L10][L9]
          b7  b6  b5  b4  b3  b2  b1  b0

Active-High: 1 = an, 0 = aus
```

### Bit-Zugriff

```cpp
// Taster ID 1-10 -> Byte/Bit
uint8_t btn_byte(uint8_t id) { return (id - 1) / 8; }
uint8_t btn_bit(uint8_t id)  { return 7 - ((id - 1) % 8); }  // MSB-first

// LED ID 1-10 -> Byte/Bit
uint8_t led_byte(uint8_t id) { return (id - 1) / 8; }
uint8_t led_bit(uint8_t id)  { return (id - 1) % 8; }        // LSB-first
```

## SPI-Protokoll

### CD4021B Lesevorgang

```
PS:   ────┐     ┌──────────────────────────────
          └─────┘
          Load   Shift

Q8:   ────X─────[B1][B2][B3][B4][B5][B6][B7]───
          ^
          digitalRead() hier!

CLK:  ────────────┐ ┐ ┐ ┐ ┐ ┐ ┐ ┐ ┐ ┐ ┐ ┐ ┐ ┐
                  └┘└┘└┘└┘└┘└┘└┘└┘└┘└┘└┘└┘└┘└┘

Mode: SPI_MODE1 (CPOL=0, CPHA=1)
```

### 74HC595 Schreibvorgang

```
SER:  [B16][B15]...[B2][B1]────────────────────
      MSB first, letztes Byte zuerst senden

CLK:  ┐ ┐ ┐ ┐ ┐ ┐ ┐ ┐ ┐ ┐ ┐ ┐ ┐ ┐ ┐ ┐────────
      └┘└┘└┘└┘└┘└┘└┘└┘└┘└┘└┘└┘└┘└┘└┘└┘

RCK:  ──────────────────────────────────┐  ┌──
      Latch nach Transfer              └──┘

Mode: SPI_MODE0 (CPOL=0, CPHA=0)
```

## Klassen-API

### SpiBus

```cpp
class SpiBus {
    void begin(int sck, int miso, int mosi);  // Initialisierung
    void lock();                               // Mutex nehmen
    void unlock();                             // Mutex freigeben
};
```

### SpiGuard (RAII)

```cpp
{
    SpiGuard guard(bus, SPISettings(1000000, MSBFIRST, SPI_MODE0));
    SPI.transfer(data);
}  // Automatisch: endTransaction() + unlock()
```

### Cd4021

```cpp
class Cd4021 {
    void init();                              // GPIO Setup
    void readRaw(SpiBus& bus, uint8_t* out);  // Liest BTN_BYTES
};
```

### Hc595

```cpp
class Hc595 {
    void init();                              // GPIO + PWM Setup
    void setBrightness(uint8_t percent);      // 0-100%
    void write(SpiBus& bus, uint8_t* state);  // Schreibt LED_BYTES
};
```

### Debouncer

```cpp
class Debouncer {
    void init();                                              // Reset
    bool update(uint32_t now_ms, const uint8_t* raw,         // true wenn
                uint8_t* deb);                                // geaendert
};
```

### Selection

```cpp
class Selection {
    void init();                                              // Reset
    bool update(const uint8_t* deb_now, uint8_t& active_id); // true wenn
};                                                            // geaendert
```

## Erweiterung auf 100 Taster

### Aenderungen in config.h

```cpp
constexpr uint8_t BTN_COUNT = 100;
constexpr uint8_t LED_COUNT = 100;
// BTN_BYTES/LED_BYTES werden automatisch 13
```

### Hardware-Erweiterung

```
CD4021B Kette:  IC0 -> IC1 -> ... -> IC12 (13 Chips)
                Q8 -> DS    Q8 -> DS

74HC595 Kette:  IC12 <- ... <- IC1 <- IC0
                SER <- Q7'   SER <- Q7'
```

### Performance bei 100 Tastern

| Metrik | 10 Taster | 100 Taster |
|--------|-----------|------------|
| SPI Bytes | 2 | 13 |
| CD4021 Transfer | 32 us | 208 us |
| HC595 Transfer | 16 us | 104 us |
| Zyklus gesamt | ~400 us | ~800 us |
| Reserve (5 ms) | 4.6 ms | 4.2 ms |

## Troubleshooting

### Taster 1 reagiert nicht

**Ursache:** First-Bit-Problem - digitalRead() vor SPI fehlt oder falsch.

**Loesung:** In `cd4021.cpp` pruefen:
```cpp
const uint8_t first_bit = digitalRead(PIN_BTN_MISO) ? 1u : 0u;
```

### LEDs flackern

**Ursache:** CD4021-Read schiebt Nullen durch HC595 Shift-Register.

**Loesung:** `LED_REFRESH_EVERY_CYCLE = true` in `config.h`.

### Phantom-Tastendrucke

**Ursache:** `SERIAL_PROTOCOL_ONLY = false` - Debug-Ausgaben werden als Befehle interpretiert.

**Loesung:** Fuer Pi-Betrieb `SERIAL_PROTOCOL_ONLY = true` setzen.

### USB-CDC fragmentiert Nachrichten

**Ursache:** USB-CDC sendet in 64-Byte Paketen, printf kann splitten.

**Loesung:** `send_line()` in serial_task.cpp nutzt atomisches write + flush + delay.

### Debounce zu langsam/schnell

**Symptom:** Taster reagiert traege oder prellt durch.

**Loesung:** `DEBOUNCE_MS` in `config.h` anpassen (Standard: 30 ms).

### Queue laeuft voll

**Symptom:** Events gehen verloren bei vielen schnellen Tastendruecken.

**Loesung:** `LOG_QUEUE_LEN` in `config.h` erhoehen (Standard: 32).

## Test-Befehle

```bash
# Verbindung pruefen
echo "PING" > /dev/ttyUSB0

# LED-Test
echo "LEDALL" > /dev/ttyUSB0   # Alle an
echo "LEDCLR" > /dev/ttyUSB0   # Alle aus
echo "LEDSET 005" > /dev/ttyUSB0  # Nur LED 5

# Status abfragen
echo "STATUS" > /dev/ttyUSB0
```
