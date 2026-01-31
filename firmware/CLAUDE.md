# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

Siehe auch: `../CLAUDE.md` fuer projektweiten Kontext (Pi-Server, Deployment, Protokoll).

## Build-Befehle

```bash
pio run                       # Kompilieren
pio run -t upload             # Flashen
pio run -t upload -t monitor  # Build + Flash + Monitor
pio device monitor            # Serial-Monitor (115200 Baud)
pio run -e debug              # Debug-Build (CORE_DEBUG_LEVEL=4)

./tools/format.sh             # Code formatieren (clang-format)
./tools/lint.sh               # Statische Analyse (cppcheck)
```

## Architektur

### Schichtenmodell (Abhaengigkeiten von oben nach unten)

| Schicht | Verzeichnis | Verantwortung |
|---------|-------------|---------------|
| Entry | `main.cpp` | Queue-Erstellung, Task-Start |
| App | `src/app/` | FreeRTOS Tasks (io_task @ 200Hz, serial_task) |
| Logic | `src/logic/` | Entprellung (30ms), Auswahllogik |
| Driver | `src/drivers/` | CD4021B (Taster-Input), 74HC595 (LED-Output) |
| HAL | `src/hal/` | SPI-Bus-Abstraktion mit Mutex |

### Kritische Design-Entscheidungen

**Gemeinsamer SPI-Bus**: CD4021B (MODE1, 500kHz) und 74HC595 (MODE0, 1MHz) teilen SCK. `SpiGuard` (RAII) stellt korrekte Transaktionsbehandlung mit Mode-Wechsel sicher.

**First-Bit-Problem**: CD4021B gibt PI-1 sofort nach Parallel-Load aus, vor dem ersten Takt. Loesung: `digitalRead()` erfasst es vor dem SPI-Transfer in `cd4021.cpp`.

**Queue-Entkopplung**: io_task blockiert nie; serial_task kann langsam sein. log_event_t-Queue liefert atomare Snapshots.

**Active-Low Taster**: Pull-up-Widerstaende bedeuten 0 = gedrueckt. Nutze `activeLow_pressed()` aus `bitops.h`.

**LED-Refresh**: Wegen geteiltem SPI-Bus wird LED-Zustand jeden Zyklus aufgefrischt (`LED_REFRESH_EVERY_CYCLE = true`), um Glitches durch CD4021-Reads zu verhindern.

## Protokoll-Referenz

**Wichtig:** Alle IDs sind 1-basiert und 3-stellig formatiert (001-100).

### ESP32 -> Pi
- `READY` - Boot abgeschlossen
- `FW <version>` - Firmware-Version
- `PRESS 001` - Taster gedrueckt
- `RELEASE 001` - Taster losgelassen
- `PONG` - Antwort auf PING
- `OK` - Befehl ausgefuehrt
- `ERROR <msg>` - Fehler

### Pi -> ESP32
- `LEDSET 001` - One-Hot: nur diese LED an
- `LEDON/LEDOFF 001` - Additiv ein/aus
- `LEDCLR` / `LEDALL` - Alle aus/an
- `PING` / `STATUS` / `VERSION` / `HELP`

## Pin-Belegung

| Pin | GPIO | Funktion | Chip |
|-----|------|----------|------|
| D0 | GPIO1 | LED_RCK (Latch) | 74HC595 |
| D1 | GPIO2 | BTN_PS (Parallel/Shift) | CD4021B |
| D2 | GPIO3 | LED_OE (PWM) | 74HC595 |
| D8 | GPIO7 | SCK (gemeinsamer Takt) | Beide |
| D9 | GPIO8 | BTN_MISO | CD4021B |
| D10 | GPIO9 | LED_MOSI | 74HC595 |

Reserviert: GPIO0 (Boot), GPIO43/44 (USB Serial), GPIO19/20 (USB D+/D-)

## Konfiguration

Alle Parameter in `include/config.h`:

```cpp
constexpr uint8_t BTN_COUNT = 10;           // Skalierbar auf 100
constexpr uint8_t LED_COUNT = 10;
constexpr uint32_t IO_PERIOD_MS = 5;        // 200 Hz
constexpr uint32_t DEBOUNCE_MS = 30;
constexpr bool LATCH_SELECTION = true;      // Auswahl bleibt nach Loslassen
constexpr bool SERIAL_PROTOCOL_ONLY = true; // Debug-Logs fuer Pi deaktivieren
```

Fuer 100 Taster: `BTN_COUNT` und `LED_COUNT` aendern. `BTN_BYTES`/`LED_BYTES` berechnen sich automatisch.

## Debugging

Ausfuehrliches Logging in `config.h` aktivieren:
```cpp
constexpr bool LOG_VERBOSE_PER_ID = true;
constexpr bool LOG_ON_RAW_CHANGE = true;
constexpr bool SERIAL_PROTOCOL_ONLY = false;
```

STATUS-Ausgabe verstehen:
```
LEDS 0000100000      <- Bit-Vektor (MSB links), LED 5 an
BTNS 1111111111      <- Active-Low: 1 = losgelassen
CURLED 5             <- Aktuelle LED (1-basiert)
```

## Coding-Standards

### Randbedingungen

- Target: **ESP32-S3 (Seeed XIAO ESP32S3)**
- Build: **PlatformIO**
- Sprachsplit: **C fuer Treiber/HAL**, **C++ fuer Applikation**
- Basis: **BARR-C + MISRA-orientiert**

### Naming Conventions

| Element | Stil | Beispiel |
|---------|------|----------|
| Dateien | snake_case | `spi_bus.cpp` |
| Typen (Struct) | snake_case + `_t` | `log_event_t` |
| Typen (Enum) | snake_case + `_e` | `led_command_e` |
| Funktionen | snake_case | `start_io_task` |
| Klassen | PascalCase | `SpiBus` |
| Private Member | `_` Praefix | `_mtx`, `_spi` |
| Konstanten | UPPER_SNAKE_CASE | `BTN_COUNT` |

### Praktische Regeln

1. **Keine Magic Numbers** - Pins/Zeiten in `config.h`
2. **Kein `strcpy/strcat`** - Buffer-Safety beachten
3. **Keine impliziten Casts** - `stdint.h`-Typen bevorzugen
4. **ISR kurz halten** - Event -> Queue, Arbeit in Task
5. **RAII ja, Exceptions nein** - Fehler ueber Return-Code

### Kommentare (Doxygen)

```cpp
/**
 * @file dateiname.cpp
 * @brief Kurzbeschreibung
 */

/**
 * @brief Funktionsbeschreibung
 * @param name Beschreibung
 * @return Beschreibung
 */
```

## FreeRTOS-Hinweise

- Core 0: Reserviert fuer WiFi/BLE Stack
- Core 1: Anwendung (IO + Serial Tasks)
- IO-Task Prioritaet: 5 (hoch, muss 200Hz jitterfrei halten)
- Serial-Task Prioritaet: 2 (darf drosseln, darf IO nicht blockieren)

## Timing-Budget (5 ms Zyklus)

| Phase | Dauer | Beschreibung |
|-------|-------|--------------|
| LED-Befehle | <10 us | Queue lesen |
| CD4021B Read | ~80 us | 2 Bytes @ 500kHz + First-Bit |
| Debounce | <5 us | 10 Timer pruefen |
| Selection | <2 us | Flanken-Scan |
| HC595 Write | ~16 us | 2 Bytes @ 1MHz |
| Queue Send | <5 us | Log-Event |
| **Gesamt** | ~120 us | 2.4% des Budgets |

Bei 100 Tastern: ~600 us (12% des Budgets).

## Boot-Sequenz

```
setup() -> Queues erstellen -> Tasks starten -> READY senden
```

**WICHTIG:** Host darf erst nach Empfang von `READY` Befehle senden.

## Dokumentation

Detaillierte Informationen in `docs/`:

| Datei | Inhalt |
|-------|--------|
| `overview.md` | Kurzreferenz, Invarianten |
| `architecture.md` | Schichtenmodell, Design-Entscheidungen |
| `ALGORITHM.md` | Debounce, Selection, Bit-Mapping |
| `FLOWCHART.md` | Programmablaufplaene |
| `DEVELOPER.md` | API, Timing, Troubleshooting |
| `HARDWARE.md` | Pinbelegung, Schaltung |
| `CODING_STANDARD.md` | Code-Stil-Details |
