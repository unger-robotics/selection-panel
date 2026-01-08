# Selection Panel: Architektur-Übersicht

Version 2.5.0 | Phase 7 (Raspberry Pi Integration)

## Systemkontext

Das Selection Panel verbindet 10 Taster und 10 LEDs über einen ESP32-S3 mit einem Raspberry Pi 5. Der Pi steuert Multimedia-Wiedergabe, der ESP32 übernimmt die Echtzeit-I/O.

```
┌─────────────────────────────────────────────────────────────────────────────┐
│                              RASPBERRY PI 5                                 │
│                    Python-Server + Web-Dashboard                            │
│                         (Serial: /dev/ttyACM0)                              │
└─────────────────────────────────┬───────────────────────────────────────────┘
                                  │ USB-CDC @ 115200 Baud
              ┌───────────────────┴───────────────────┐
              │         SERIAL PROTOKOLL              │
              │  ESP→Pi: READY, PRESS 001, RELEASE 001│
              │  Pi→ESP: LEDSET 001, LEDON, LEDOFF... │
              └───────────────────┬───────────────────┘
                                  │
┌─────────────────────────────────┴───────────────────────────────────────────┐
│                           ESP32-S3 (XIAO)                                   │
│                      FreeRTOS Dual-Core @ 240 MHz                           │
├─────────────────────────────────────────────────────────────────────────────┤
│  ┌─────────────────┐                      ┌─────────────────┐               │
│  │   io_task       │ ────LogEvent────▶    │  serial_task    │               │
│  │   (Prio 5)      │      Queue           │   (Prio 2)      │               │
│  │   5 ms Periode  │ ◀──LedCallback────── │                 │               │
│  └────────┬────────┘                      └─────────────────┘               │
│           │                                                                 │
│  ┌────────┴────────────────────────────┐                                    │
│  │           LOGIC LAYER               │                                    │
│  │  ┌─────────────┐  ┌─────────────┐   │                                    │
│  │  │  Debouncer  │  │  Selection  │   │                                    │
│  │  │  30 ms/Btn  │─▶│ Last-Press  │   │                                    │
│  │  │  Zeitbasiert│  │ Wins + Latch│   │                                    │
│  │  └─────────────┘  └─────────────┘   │                                    │
│  └────────┬────────────────────────────┘                                    │
│           │                                                                 │
│  ┌────────┴────────────────────────────┐                                    │
│  │          DRIVER LAYER               │                                    │
│  │  ┌─────────────┐  ┌─────────────┐   │                                    │
│  │  │   Cd4021    │  │   Hc595     │   │                                    │
│  │  │ Taster-In   │  │ LED-Out     │   │                                    │
│  │  └──────┬──────┘  └──────┬──────┘   │                                    │
│  └─────────┼────────────────┼──────────┘                                    │
│            │                │                                               │
│  ┌─────────┴────────────────┴──────────┐                                    │
│  │        HAL: SpiBus + SpiGuard       │                                    │
│  │         Mutex-geschützt             │                                    │
│  └─────────────────┬───────────────────┘                                    │
└────────────────────┼────────────────────────────────────────────────────────┘
                     │ SPI-Bus (shared)
┌────────────────────┴────────────────────────────────────────────────────────┐
│                         HARDWARE                                            │
│  ┌─────────────┐                           ┌─────────────┐                  │
│  │  CD4021B    │  ←── 10× Taster           │  74HC595    │ ──→ 10× LEDs    │
│  │  (PISO)     │      (Active-Low)         │  (SIPO)     │     (5 mm)      │
│  └─────────────┘                           └─────────────┘                  │
└─────────────────────────────────────────────────────────────────────────────┘
```

## Schichtenmodell

Die Firmware folgt einem strikten Schichtenmodell. Jede Schicht kennt nur die darunterliegende:

| Schicht | Verzeichnis | Verantwortung |
|---------|-------------|---------------|
| Entry   | `main.cpp`  | Queue erstellen, Tasks starten |
| App     | `src/app/`  | FreeRTOS Tasks (io_task, serial_task) |
| Logic   | `src/logic/`| Debouncing, Selection-Logik |
| Driver  | `src/drivers/` | CD4021B, 74HC595 Ansteuerung |
| HAL     | `src/hal/`  | SPI-Bus Abstraktion mit Mutex |

## Datenfluss

Ein Tastendruck durchläuft folgende Stationen:

```
  Hardware           Driver            Logic              App             Serial
     │                  │                │                  │                │
     │   Parallel Load  │                │                  │                │
  CD4021B ──────────▶ readRaw() ──────▶ raw[2] ────────────┐                │
     │                  │                │                  │                │
     │                  │         Debouncer.update()       │                │
     │                  │                │                  │                │
     │                  │                ▼                  │                │
     │                  │           deb[2] ────────────────┐                │
     │                  │                │                  │                │
     │                  │         Selection.update()       │                │
     │                  │                │                  │                │
     │                  │                ▼                  │                │
     │                  │           activeId ──────────────┼───▶ LogEvent   │
     │                  │                │                  │        │       │
     │                  │                ▼                  │        │       │
     │                  │           led[2] ────────────────┘        │       │
     │                  │                │                          │       │
     │   Latch Pulse    │                │                          ▼       │
  74HC595 ◀──────────write() ◀───────────┘                     xQueueSend   │
     │                  │                                          │       │
     │                  │                                          ▼       │
     │                  │                                    serial_task    │
     │                  │                                          │       │
     │                  │                                          ▼       │
     │                  │                                   "PRESS 001\n" ──▶ Pi
```

## Timing-Diagramm

Die zeitlichen Zusammenhänge im System:

```
Zeit (ms)    0     5    10    15    20    25    30    35    40
             │     │     │     │     │     │     │     │     │
io_task      ├─R─W─┼─R─W─┼─R─W─┼─R─W─┼─R─W─┼─R─W─┼─R─W─┼─R─W─┤
             │     │     │     │     │     │     │     │     │
Taster       ════╗ │     │     │     │     │     │     │     │
gedrückt         ║ │     │     │     │     │     │     │     │
             ════╝ │     │     │     │     │     │     │     │
                   │     │     │     │     │     │     │     │
Debounce     ─────────────────────────────╗                   │
(30 ms)                                   ║                   │
             ─────────────────────────────╝                   │
                                          │                   │
LogEvent                                  ├──▶ Queue          │
                                          │                   │
PRESS 001                                 └──────────────────▶│ Serial

R = CD4021 Read (500 kHz SPI, ~20 µs)
W = HC595 Write (1 MHz SPI, ~16 µs)
```

## Kernkonzepte

### Gemeinsamer SPI-Bus

CD4021B und 74HC595 teilen sich einen SPI-Bus, verwenden aber unterschiedliche Modi:

| Chip | Funktion | SPI-Modus | Taktfrequenz | Bit-Ordnung |
|------|----------|-----------|--------------|-------------|
| CD4021B | Taster einlesen | MODE1 (CPOL=0, CPHA=1) | 500 kHz | MSB-first |
| 74HC595 | LEDs ansteuern | MODE0 (CPOL=0, CPHA=0) | 1 MHz | MSB-first |

Der `SpiGuard` (RAII-Pattern) stellt sicher, dass Transaktionen korrekt beendet werden:

```cpp
{
    SpiGuard g(bus, settings);  // lock() + beginTransaction()
    SPI.transfer(data);
}  // Automatisch: endTransaction() + unlock()
```

### Bit-Adressierung

Die Chips verwenden unterschiedliche Bit-Reihenfolgen. `bitops.h` abstrahiert diese Eigenheit:

| Chip | ID 1 | ID 8 | ID 9 | ID 10 |
|------|------|------|------|-------|
| CD4021B (Taster) | Byte 0, Bit 7 | Byte 0, Bit 0 | Byte 1, Bit 7 | Byte 1, Bit 6 |
| 74HC595 (LEDs) | Byte 0, Bit 0 | Byte 0, Bit 7 | Byte 1, Bit 0 | Byte 1, Bit 1 |

### First-Bit-Problem (CD4021B)

Nach dem Parallel-Load liegt Q8 sofort am Ausgang, bevor der erste Clock kommt. SPI samplet aber erst nach der ersten Flanke. Die Lösung: Das erste Bit wird vor dem SPI-Transfer per `digitalRead()` gesichert.

### Zeitbasiertes Debouncing

Jeder Taster hat einen eigenen Timer. Bei Änderung des Rohwerts wird der Timer zurückgesetzt. Erst wenn der Timer 30 ms abgelaufen ist und der Rohwert stabil bleibt, wird der entprellte Zustand übernommen. Diese Methode ist unabhängig von der Abtastrate und erlaubt schnelle Tastenfolgen.

### Selection-Logik

Die Selection-Logik folgt dem "Last Press Wins"-Prinzip: Wird ein neuer Taster gedrückt, überschreibt er die vorherige Auswahl. Mit `LATCH_SELECTION=true` bleibt die Auswahl nach Loslassen bestehen.

## Konfigurationsparameter

Die wichtigsten Parameter in `config.h`:

| Parameter | Wert | Beschreibung |
|-----------|------|--------------|
| `IO_PERIOD_MS` | 5 | Abtastrate des IO-Tasks (200 Hz) |
| `DEBOUNCE_MS` | 30 | Entprellzeit pro Taster |
| `LATCH_SELECTION` | true | Auswahl bleibt nach Loslassen |
| `PWM_DUTY_PERCENT` | 50 | LED-Helligkeit (0-100%) |
| `LED_REFRESH_EVERY_CYCLE` | true | Kompensiert SPI-Glitches |
| `SERIAL_PROTOCOL_ONLY` | true | Nur Protokoll, keine Debug-Logs |

## Skalierung auf 100 Buttons

Die Architektur ist für 100 Taster/LEDs vorbereitet:

1. `BTN_COUNT` und `LED_COUNT` auf 100 setzen
2. `BTN_BYTES` und `LED_BYTES` werden automatisch auf 13 berechnet
3. Zusätzliche Schieberegister in Daisy-Chain verkabeln
4. Die Logik-Schicht skaliert automatisch (Bit-Arrays)

Bei 100 Buttons erhöht sich die SPI-Transferzeit von ~20 µs auf ~260 µs – noch weit unter dem 5 ms Zyklus.
