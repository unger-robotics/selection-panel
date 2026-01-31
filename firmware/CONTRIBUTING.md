# Contributing

## Build & Test

```bash
pio run                    # Kompilieren
pio run -t upload          # Flashen
pio device monitor         # Serial-Monitor
./tools/format.sh          # Code formatieren
./tools/lint.sh            # Statische Analyse
```

## Dogma

- Kein dynamischer Speicher im Dauerbetrieb
- Kein Serial im IO-Pfad
- Keine blockierenden Delays im Taskpfad (`vTaskDelayUntil`)
- Hardware-Zugriff nur ueber HAL/Driver-Schichten

## Coding-Standards

### Naming Conventions

| Element | Stil | Beispiel |
|---------|------|----------|
| Dateien | snake_case | `spi_bus.cpp` |
| Typen (Struct) | snake_case + `_t` | `log_event_t` |
| Typen (Enum) | snake_case + `_e` | `led_command_e` |
| Funktionen | snake_case | `start_io_task` |
| Klassen | PascalCase | `SpiBus` |
| Private Member | `_` Praefix | `_mtx`, `_raw_prev` |
| Konstanten | UPPER_SNAKE_CASE | `BTN_COUNT` |

### Kommentare (BARR-C/Doxygen)

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

### Sektionen in Dateien

```cpp
// INCLUDES
// TYPES
// MODUL-LOKALE VARIABLEN
// PRIVATE HILFSFUNKTIONEN
// OEFFENTLICHE FUNKTIONEN
// PRIVATE METHODEN
```

## Code Style

- Mapping/Bitlogik zentral in `bitops.h` (kein Copy&Paste)
- Kommentare erklaeren **warum** (Timing, Edge-Cases, Hardware)
- Hardware-Details nur in `drivers/`
- Logik (Debounce/Selection) ohne Pins/SPI
- Keine Magic Numbers - nutze `config.h`
- Keine impliziten Casts - `stdint.h` Typen bevorzugen
- ISR kurz halten: Event -> Queue, Arbeit in Task

## Vor dem Commit

1. `./tools/format.sh` - Code formatieren
2. `pio run` - Kompiliert ohne Fehler
3. `./tools/lint.sh` - Keine HIGH/MEDIUM Warnungen
4. Testen auf Hardware (falls moeglich)

## Pull Requests

- Jede Aenderung hat: (1) Grund, (2) DoD-Test, (3) Doku-Update falls Mapping/Policy betroffen
- Commit-Messages: Imperativ, kurz, aussagekraeftig
- Eine Aenderung pro Commit
