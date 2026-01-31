# Selection Panel Firmware

**ESP32-S3 Firmware fuer 10-100 Taster mit LED-Feedback**

Version 2.5.3 | Phase 7 (Pi-Integration abgeschlossen)

## Ueberblick

Das Selection Panel ist ein modulares Eingabesystem mit physischen Tastern und LED-Feedback. Ein ESP32-S3 (XIAO) liest Taster ueber CD4021B-Schieberegister ein und steuert LEDs ueber 74HC595. Ein Raspberry Pi 5 uebernimmt die Anwendungslogik.

```
┌───────────────────┐     USB-CDC      ┌───────────────────┐
│   Raspberry Pi 5  │◀────────────────▶│   ESP32-S3 XIAO   │
│                   │   115200 Baud    │                   │
│  • aiohttp Server │                  │  • 200 Hz I/O     │
│  • WebSocket      │   PRESS 001      │  • Entprellung    │
│  • Web Dashboard  │   LEDSET 001     │  • LED-PWM        │
└───────────────────┘                  └─────────┬─────────┘
        │                                        │ SPI
        ▼ http://rover:8080              ┌───────┴───────┐
   ┌─────────┐                           │               │
   │ Browser │                     ┌─────┴─────┐   ┌─────┴─────┐
   │Dashboard│                     │  CD4021B  │   │  74HC595  │
   └─────────┘                     │  Taster   │   │   LEDs    │
                                   └─────┬─────┘   └─────┬─────┘
                                         │               │
                                   ┌─────┴─────┐   ┌─────┴─────┐
                                   │ 10 Taster │   │ 10 LEDs   │
                                   └───────────┘   └───────────┘
```

## Features

- **10 Taster** mit zeitbasierter Entprellung (30 ms)
- **10 LEDs** mit PWM-Helligkeitsregelung
- **FreeRTOS** auf ESP32-S3 (200 Hz I/O-Zyklus)
- **Serial-Protokoll** fuer Pi-Integration
- **Skalierbar** auf 100 Taster/LEDs

## Schnellstart

### Voraussetzungen

- [PlatformIO](https://platformio.org/) (CLI oder IDE)
- Seeed XIAO ESP32-S3

### Build & Flash

```bash
pio run                       # Kompilieren
pio run -t upload             # Flashen
pio device monitor            # Serial-Monitor (115200 Baud)
pio run -t upload -t monitor  # Alles in einem
```

### Erwartete Ausgabe

```
READY
FW selection-panel v2.5.3
```

### Entwicklung

```bash
./tools/format.sh             # Code formatieren (clang-format)
./tools/lint.sh               # Statische Analyse (cppcheck)
pio check                     # PlatformIO Check
```

## Architektur

### Schichtenmodell

| Schicht | Verzeichnis | Verantwortung |
|---------|-------------|---------------|
| Entry | `main.cpp` | Queue-Erstellung, Task-Start |
| App | `src/app/` | FreeRTOS Tasks (io_task, serial_task) |
| Logic | `src/logic/` | Entprellung, Auswahllogik |
| Driver | `src/drivers/` | CD4021B, 74HC595 |
| HAL | `src/hal/` | SPI-Bus mit Mutex |

### Projektstruktur

```
firmware/
├── include/
│   ├── config.h          # Konfiguration (Pins, Timing)
│   ├── types.h           # Gemeinsame Datentypen
│   └── bitops.h          # Bit-Operationen
├── src/
│   ├── main.cpp          # Entry Point
│   ├── app/              # FreeRTOS Tasks
│   │   ├── io_task.*     # I/O-Zyklus (200 Hz)
│   │   └── serial_task.* # Serial-Kommunikation
│   ├── logic/            # Geschaeftslogik
│   │   ├── debounce.*    # Zeitbasierte Entprellung
│   │   └── selection.*   # One-Hot Auswahllogik
│   ├── drivers/          # Hardware-Treiber
│   │   ├── cd4021.*      # Taster-Input
│   │   └── hc595.*       # LED-Output
│   └── hal/              # Hardware Abstraction
│       └── spi_bus.*     # SPI-Bus
├── docs/                 # Dokumentation
│   ├── overview.md       # Kurzreferenz
│   ├── architecture.md   # Schichtenmodell
│   ├── ALGORITHM.md      # Algorithmen
│   ├── FLOWCHART.md      # Ablaufdiagramme
│   ├── DEVELOPER.md      # Entwickler-Guide
│   ├── HARDWARE.md       # Hardware-Dokumentation
│   └── CODING_STANDARD.md# Code-Stil
├── tools/                # Hilfsskripte
│   ├── format.sh         # clang-format
│   └── lint.sh           # cppcheck
├── platformio.ini
├── CLAUDE.md             # KI-Assistenz Kontext
├── CONTRIBUTING.md       # Beitragsrichtlinien
└── README.md
```

## Konfiguration

Wichtige Parameter in `include/config.h`:

```cpp
constexpr uint8_t BTN_COUNT = 10;        // Anzahl Taster (max 100)
constexpr uint8_t LED_COUNT = 10;        // Anzahl LEDs (max 100)
constexpr uint32_t IO_PERIOD_MS = 5;     // Abtastrate (200 Hz)
constexpr uint32_t DEBOUNCE_MS = 30;     // Entprellzeit
constexpr uint8_t PWM_DUTY_PERCENT = 50; // LED-Helligkeit
constexpr bool LATCH_SELECTION = true;   // Auswahl persistent
```

### Skalierung auf 100 Taster

```cpp
constexpr uint8_t BTN_COUNT = 100;
constexpr uint8_t LED_COUNT = 100;
```

`BTN_BYTES`/`LED_BYTES` berechnen sich automatisch.

## Protokoll

### ESP32 → Pi

| Nachricht | Beschreibung |
|-----------|--------------|
| `READY` | System bereit |
| `FW <version>` | Firmware-Version |
| `PRESS <id>` | Taster gedrueckt (001-100) |
| `RELEASE <id>` | Taster losgelassen |
| `PONG` | Antwort auf PING |
| `OK` | Befehl ausgefuehrt |
| `ERROR <msg>` | Fehler |

### Pi → ESP32

| Befehl | Beschreibung |
|--------|--------------|
| `LEDSET <id>` | One-Hot: nur diese LED an |
| `LEDON <id>` | LED einschalten (additiv) |
| `LEDOFF <id>` | LED ausschalten |
| `LEDCLR` | Alle LEDs aus |
| `LEDALL` | Alle LEDs an |
| `PING` | Verbindung pruefen |
| `STATUS` | Status abfragen |
| `VERSION` | Version abfragen |
| `HELP` | Hilfe anzeigen |

**Wichtig:** Alle IDs sind 1-basiert und 3-stellig formatiert (001-100).

## Hardware

### Pin-Belegung (XIAO ESP32-S3)

| Pin | GPIO | Funktion | Chip |
|-----|------|----------|------|
| D0 | GPIO1 | LED_RCK (Latch) | 74HC595 |
| D1 | GPIO2 | BTN_PS (Parallel/Shift) | CD4021B |
| D2 | GPIO3 | LED_OE (PWM) | 74HC595 |
| D8 | GPIO7 | SCK (gemeinsamer Takt) | Beide |
| D9 | GPIO8 | BTN_MISO | CD4021B |
| D10 | GPIO9 | LED_MOSI | 74HC595 |

### Komponenten

| Komponente | Typ | Anzahl |
|------------|-----|--------|
| XIAO ESP32-S3 | Mikrocontroller | 1 |
| CD4021B | Schieberegister (Input) | 2 |
| 74HC595 | Schieberegister (Output) | 2 |
| Taster | 6×6 mm Tactile | 10 |
| LED | 5 mm | 10 |
| Widerstand | 330Ω-3kΩ (LED) | 10 |
| Widerstand | 10kΩ (Pull-up) | 10 |
| Kondensator | 100nF (Abblockkondensator) | 4 |

## Debugging

Verbose-Logging in `config.h` aktivieren:

```cpp
constexpr bool LOG_VERBOSE_PER_ID = true;
constexpr bool LOG_ON_RAW_CHANGE = true;
constexpr bool SERIAL_PROTOCOL_ONLY = false;
```

## Dokumentation

| Dokument | Inhalt |
|----------|--------|
| [overview.md](docs/overview.md) | Kurzreferenz, Invarianten, Boot-Sequenz |
| [architecture.md](docs/architecture.md) | Schichtenmodell, Datenfluss, Design-Entscheidungen |
| [ALGORITHM.md](docs/ALGORITHM.md) | Debounce, Selection, Bit-Mapping im Detail |
| [FLOWCHART.md](docs/FLOWCHART.md) | Programmablaufplaene (ASCII) |
| [DEVELOPER.md](docs/DEVELOPER.md) | API, Timing, Memory, Troubleshooting |
| [HARDWARE.md](docs/HARDWARE.md) | Pin-Belegung, Schaltung, Erweiterung |
| [CODING_STANDARD.md](docs/CODING_STANDARD.md) | Namenskonventionen, Code-Stil |

## Lizenz

MIT License

## Autor

Jan Unger
