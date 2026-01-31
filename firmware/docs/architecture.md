# Firmware-Architektur

Schichtenmodell und Datenfluss der Selection Panel Firmware.

## Schichtenmodell

```
┌─────────────────────────────────────────────────────────────────┐
│                         ENTRY POINT                             │
│                          main.cpp                               │
│                   Queue-Erstellung, Task-Start                  │
└───────────────────────────┬─────────────────────────────────────┘
                            │
┌───────────────────────────┴─────────────────────────────────────┐
│                         APP LAYER                               │
│                         src/app/                                │
│  ┌─────────────────────┐       ┌─────────────────────┐          │
│  │     io_task.cpp     │◄─────►│  serial_task.cpp    │          │
│  │  200 Hz I/O-Zyklus  │ Queue │  Serial-Protokoll   │          │
│  └──────────┬──────────┘       └──────────┬──────────┘          │
└─────────────┼──────────────────────────────┼────────────────────┘
              │                              │
┌─────────────┴──────────────────────────────┴────────────────────┐
│                        LOGIC LAYER                              │
│                        src/logic/                               │
│  ┌─────────────────────┐       ┌─────────────────────┐          │
│  │   debounce.cpp      │       │   selection.cpp     │          │
│  │ Zeitbasierte        │       │ One-Hot Auswahl     │          │
│  │ Entprellung         │       │ Last-Press-Wins     │          │
│  └─────────────────────┘       └─────────────────────┘          │
└─────────────────────────────────────────────────────────────────┘
              │                              │
┌─────────────┴──────────────────────────────┴────────────────────┐
│                       DRIVER LAYER                              │
│                       src/drivers/                              │
│  ┌─────────────────────┐       ┌─────────────────────┐          │
│  │    cd4021.cpp       │       │     hc595.cpp       │          │
│  │ Taster-Input        │       │ LED-Output          │          │
│  │ SPI MODE1, 500 kHz  │       │ SPI MODE0, 1 MHz    │          │
│  └──────────┬──────────┘       └──────────┬──────────┘          │
└─────────────┼──────────────────────────────┼────────────────────┘
              │                              │
┌─────────────┴──────────────────────────────┴────────────────────┐
│                         HAL LAYER                               │
│                         src/hal/                                │
│  ┌─────────────────────────────────────────────────────────┐    │
│  │                     spi_bus.cpp                         │    │
│  │              SPI-Bus mit Mutex-Schutz                   │    │
│  │                   SpiGuard (RAII)                       │    │
│  └─────────────────────────────────────────────────────────┘    │
└─────────────────────────────────────────────────────────────────┘
              │
┌─────────────┴───────────────────────────────────────────────────┐
│                      CONFIG / TYPES                             │
│                        include/                                 │
│  ┌──────────────┐  ┌──────────────┐  ┌──────────────┐           │
│  │  config.h    │  │   types.h    │  │  bitops.h    │           │
│  │ Pins, Timing │  │ log_event_t  │  │ Bit-Zugriff  │           │
│  └──────────────┘  └──────────────┘  └──────────────┘           │
└─────────────────────────────────────────────────────────────────┘
```

## Abhaengigkeitsregeln

```
ERLAUBT:
    main.cpp     → app/, config.h, types.h
    app/         → logic/, drivers/, hal/, config.h, types.h, bitops.h
    logic/       → config.h, bitops.h
    drivers/     → hal/, config.h
    hal/         → (nur Arduino/ESP-IDF)

VERBOTEN:
    logic/       → drivers/, hal/     (keine Hardware-Abhaengigkeit)
    drivers/     → app/, logic/       (keine Aufwaerts-Abhaengigkeit)
    hal/         → drivers/, logic/   (keine Aufwaerts-Abhaengigkeit)
```

## Moduluebersicht

### Entry Point

| Datei | Verantwortung |
|-------|---------------|
| `main.cpp` | Arduino setup()/loop(), Queue-Erstellung, Task-Start |

### App Layer

| Datei | Verantwortung |
|-------|---------------|
| `io_task.cpp` | 200 Hz Hauptschleife, koordiniert alle Module |
| `serial_task.cpp` | USB-CDC Protokoll, PRESS/RELEASE senden |

### Logic Layer

| Datei | Verantwortung |
|-------|---------------|
| `debounce.cpp` | Zeitbasierte Entprellung (30 ms) |
| `selection.cpp` | One-Hot Auswahl, Last-Press-Wins |

### Driver Layer

| Datei | Verantwortung |
|-------|---------------|
| `cd4021.cpp` | CD4021B Treiber, First-Bit-Korrektur |
| `hc595.cpp` | 74HC595 Treiber, PWM-Helligkeit |

### HAL Layer

| Datei | Verantwortung |
|-------|---------------|
| `spi_bus.cpp` | SPI-Bus Abstraktion, Mutex, SpiGuard |

### Config / Types

| Datei | Verantwortung |
|-------|---------------|
| `config.h` | Pins, Timing, Compile-Zeit-Konstanten |
| `types.h` | log_event_t, gemeinsame Typen |
| `bitops.h` | Bit-Zugriff fuer Taster/LEDs |

## Datenfluss

### Taster → LED (lokal)

```
CD4021B ──► _btn_raw[] ──► Debouncer ──► _btn_debounced[]
                                              │
                                              ▼
                                         Selection
                                              │
                                              ▼
                                         _active_id
                                              │
                                              ▼
                              build_one_hot_led(_active_id)
                                              │
                                              ▼
                                         _led_state[]
                                              │
                                              ▼
                                           HC595
```

### Taster → Pi (Protokoll)

```
_btn_debounced[] ──► Selection ──► active_changed?
                                         │
                                         ▼
                                   log_event_t
                                         │
                                         ▼ xQueueSend
                                   ┌───────────┐
                                   │   Queue   │
                                   └─────┬─────┘
                                         │
                                         ▼ xQueueReceive
                                   Serial-Task
                                         │
                                         ▼
                                 "PRESS 001" / "RELEASE 001"
                                         │
                                         ▼
                                      USB-CDC
                                         │
                                         ▼
                                   Raspberry Pi
```

### Pi → LED (Remote)

```
Raspberry Pi
      │
      ▼
   USB-CDC
      │
      ▼
 "LEDSET 001"
      │
      ▼
Serial-Task ──► led_control_callback()
                        │
                        ▼
                  led_cmd_event_t
                        │
                        ▼ xQueueSend
                  ┌───────────┐
                  │   Queue   │
                  └─────┬─────┘
                        │
                        ▼ xQueueReceive
                    IO-Task
                        │
                        ▼
               process_led_commands()
                        │
                        ▼
                   _led_state[]
                        │
                        ▼
                      HC595
```

## FreeRTOS Tasks

### Task-Konfiguration

| Task | Prioritaet | Core | Stack | Funktion |
|------|------------|------|-------|----------|
| IO | 5 (hoch) | 1 | 8 KB | io_task_function |
| Serial | 2 (niedrig) | 1 | 8 KB | serial_task_function |
| Arduino loop | 1 | 1 | - | vTaskDelay(MAX) |

### Warum diese Prioritaeten?

```
IO-Task (Prio 5):
    - Muss 200 Hz halten (5 ms Periode)
    - Darf nicht durch Serial blockiert werden
    - Nutzt vTaskDelayUntil() fuer praezises Timing

Serial-Task (Prio 2):
    - Kann warten (xQueueReceive mit Timeout)
    - USB-CDC kann langsam sein
    - Darf IO-Task nie blockieren
```

### Task-Interaktion

```
                    ┌─────────────┐
                    │   IO-Task   │
                    │   Prio 5    │
                    └──────┬──────┘
                           │
         ┌─────────────────┼─────────────────┐
         │                 │                 │
         ▼                 │                 ▼
┌─────────────────┐        │        ┌─────────────────┐
│   Log-Queue     │        │        │  LED-Cmd-Queue  │
│   32 Events     │        │        │    8 Events     │
└────────┬────────┘        │        └────────┬────────┘
         │                 │                 │
         │    ┌────────────┴────────────┐    │
         │    │                         │    │
         ▼    ▼                         ▼    ▼
         ┌─────────────────────────────────────┐
         │            Serial-Task              │
         │              Prio 2                 │
         └─────────────────────────────────────┘
```

## Design-Entscheidungen

### 1. Gemeinsamer SPI-Bus

**Problem:** CD4021B und 74HC595 brauchen unterschiedliche SPI-Modi.

**Entscheidung:** Ein Bus mit Mode-Switching via SpiGuard.

**Begruendung:**
- Spart GPIOs (nur ein SCK)
- SpiGuard garantiert korrektes Transaction-Handling
- Mode-Switch dauert < 1 us

### 2. Queue-Entkopplung

**Problem:** Serial kann blockieren, IO muss konstant 200 Hz halten.

**Entscheidung:** Zwei separate Queues, non-blocking Sends.

**Begruendung:**
- IO-Task blockiert nie
- Events werden bei voller Queue verworfen (akzeptabel)
- 32 Events = 160 ms Puffer

### 3. Logic ohne Hardware

**Problem:** Debounce/Selection sollen testbar sein.

**Entscheidung:** Logic-Layer hat keine Hardware-Abhaengigkeiten.

**Begruendung:**
- Kann mit Unit-Tests geprueft werden
- Algorithmus unabhaengig von Pins
- Einfacher zu verstehen

### 4. First-Bit vor SPI

**Problem:** CD4021B gibt erstes Bit vor Clock aus.

**Entscheidung:** digitalRead() vor SPI.transfer().

**Begruendung:**
- Hardware-Eigenheit des CD4021B
- Kann nicht mit SPI allein geloest werden
- Bit-Korrektur ist deterministisch

### 5. LED-Refresh jeden Zyklus

**Problem:** CD4021-Read kann HC595 Shift-Register stoeren.

**Entscheidung:** LED-Zustand jeden Zyklus neu schreiben.

**Begruendung:**
- Verhindert Glitches
- Zusaetzliche 16 us pro Zyklus (akzeptabel)
- Selbstheilend bei Stoerungen

## Erweiterbarkeit

### Neue Taster/LEDs hinzufuegen

1. `BTN_COUNT` und `LED_COUNT` in `config.h` aendern
2. Hardware erweitern (mehr Schieberegister)
3. Fertig - Bit-Mapping skaliert automatisch

### Neues Protokoll hinzufuegen

1. Neue Befehle in `serial_task.cpp` parsen
2. Neue `led_command_e` Werte hinzufuegen
3. Handler in `process_led_commands()` ergaenzen

### Neue Logik hinzufuegen

1. Neue Klasse in `src/logic/` erstellen
2. In `io_task.cpp` instanziieren und aufrufen
3. Keine Hardware-Abhaengigkeiten!

## Siehe auch

- `FLOWCHART.md` - Programmablaufplaene
- `ALGORITHM.md` - Algorithmen im Detail
- `DEVELOPER.md` - API und Timing
- `HARDWARE.md` - Pin-Belegung und Schaltung
