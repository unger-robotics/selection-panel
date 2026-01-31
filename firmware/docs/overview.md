# Systemuebersicht

Kurzreferenz fuer das Selection Panel Firmware.

## Ziel / Invarianten

| Regel | Beschreibung |
|-------|--------------|
| **1:1 Mapping** | Taster n → LED n, IDs: `1..BTN_COUNT` |
| **One-Hot** | Maximal eine LED aktiv (oder `0 = alle aus`) |
| **Preempt** | Neuer Tastendruck ueberschreibt sofort die vorherige Auswahl |
| **Non-blocking** | IO-Zyklus bleibt stabil (200 Hz), Serial ist entkoppelt |
| **Latch** | Auswahl bleibt nach Loslassen bestehen (`LATCH_SELECTION=true`) |

## Hardware

### Schieberegister

| Bauteil | Rolle | Logik | SPI |
|---------|-------|-------|-----|
| CD4021B | Taster-Input | Active-Low (0=gedrueckt) | MODE1, 500 kHz |
| 74HC595 | LED-Output | Active-High (1=an) | MODE0, 1 MHz |

### Pin-Belegung

| Pin | Konstante | Funktion |
|-----|-----------|----------|
| D0 | `PIN_LED_RCK` | 74HC595 Latch |
| D1 | `PIN_BTN_PS` | CD4021B Parallel/Shift |
| D2 | `PIN_LED_OE` | 74HC595 Output Enable (PWM) |
| D8 | `PIN_SCK` | SPI Clock (gemeinsam) |
| D9 | `PIN_BTN_MISO` | CD4021B Serial Out |
| D10 | `PIN_LED_MOSI` | 74HC595 Serial In |

### SPI-Bus (shared)

```
              ┌─────────┐
    SCK ──────┤         ├────── CD4021B CLK
              │  ESP32  │
    SCK ──────┤         ├────── 74HC595 SRCLK
              └─────────┘
```

- Gemeinsamer SCK, keine Hardware-CS
- Mode-Switching via `SpiGuard` (RAII)
- CPOL muss 0 bleiben (Mode 0 oder 1)

## Firmware-Architektur

### Tasks

| Task | Prioritaet | Frequenz | Funktion |
|------|------------|----------|----------|
| IO | 5 (hoch) | 200 Hz | Taster lesen, LEDs setzen |
| Serial | 2 (niedrig) | Event-basiert | Protokoll, USB-CDC |

### IO-Task Zyklus (5 ms)

```
1. LED-Befehle vom Pi verarbeiten (Queue)
2. Taster-Rohdaten lesen (CD4021B)
3. Entprellen (30 ms Timer)
4. Auswahl aktualisieren (One-Hot)
5. LED-Zustand schreiben (74HC595)
6. Log-Event senden (Queue, non-blocking)
```

### Serial-Task

- Liest Log-Events aus Queue → sendet `PRESS`/`RELEASE`
- Empfaengt Befehle vom Pi → steuert LEDs via Callback
- Einzige Stelle mit `Serial.*`

## Boot-Sequenz

```
RESET
  │
  ▼
setup()
  │
  ├── Queue erstellen
  ├── IO-Task starten
  └── Serial-Task starten
        │
        ▼
    "READY"        ◄── Host darf erst jetzt Befehle senden
    "FW v2.5.1"
```

## Protokoll

### ESP32 → Pi

| Nachricht | Bedeutung |
|-----------|-----------|
| `READY` | Boot abgeschlossen, bereit fuer Befehle |
| `FW <version>` | Firmware-Version |
| `PRESS 001` | Taster 1 gedrueckt |
| `RELEASE 001` | Taster 1 losgelassen |
| `PONG` | Antwort auf PING |
| `OK` | Befehl ausgefuehrt |
| `ERROR <msg>` | Fehler |

### Pi → ESP32

| Befehl | Bedeutung |
|--------|-----------|
| `PING` | Verbindung pruefen |
| `STATUS` | Status abfragen |
| `VERSION` | Version abfragen |
| `HELP` | Hilfe anzeigen |
| `LEDSET 001` | One-Hot: nur LED 1 an |
| `LEDON 001` | LED 1 einschalten (additiv) |
| `LEDOFF 001` | LED 1 ausschalten |
| `LEDCLR` | Alle LEDs aus |
| `LEDALL` | Alle LEDs an |

**Hinweis:** `LEDON/LEDOFF/LEDALL` sind nicht One-Hot-konform und nur fuer Debug gedacht.

## Konfiguration

Wichtige Parameter in `include/config.h`:

| Parameter | Wert | Beschreibung |
|-----------|------|--------------|
| `BTN_COUNT` | 10 | Anzahl Taster (max 100) |
| `LED_COUNT` | 10 | Anzahl LEDs (max 100) |
| `IO_PERIOD_MS` | 5 | Abtastrate (200 Hz) |
| `DEBOUNCE_MS` | 30 | Entprellzeit |
| `LATCH_SELECTION` | true | Auswahl bleibt nach Loslassen |
| `SERIAL_PROTOCOL_ONLY` | true | Nur Protokoll, keine Debug-Logs |

## Sicherheit

### SPI-Mutex

```cpp
{
    SpiGuard guard(bus, settings);
    SPI.transfer(...);
}  // Automatisch: endTransaction() + unlock()
```

- Mutex schuetzt vor gleichzeitigem Zugriff
- RAII garantiert korrektes Cleanup
- Keine freilaufenden SCK-Flanken

### Queue-Overflow

- Log-Queue: 32 Events (160 ms Puffer)
- LED-Cmd-Queue: 8 Events
- Bei voller Queue: Event wird verworfen (kein Blocking)

## Siehe auch

| Dokument | Inhalt |
|----------|--------|
| `architecture.md` | Schichtenmodell, Design-Entscheidungen |
| `ALGORITHM.md` | Debounce, Selection, Bit-Mapping |
| `FLOWCHART.md` | Programmablaufplaene |
| `DEVELOPER.md` | API, Timing, Troubleshooting |
| `HARDWARE.md` | Pin-Belegung, Schaltung |
