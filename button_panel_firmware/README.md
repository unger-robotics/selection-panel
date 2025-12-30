# Button Panel Firmware v2.3.0

ESP32-S3 XIAO Firmware für das Selection Panel Projekt mit 100 Tastern und 100 LEDs.

## Übersicht

```
┌─────────────────────────────────────────────────────────────┐
│                    Raspberry Pi 5                           │
│                  (Python Server)                            │
│                        │                                    │
│                   USB-Serial                                │
│                        │                                    │
│                        ▼                                    │
│  ┌─────────────────────────────────────────────────────┐   │
│  │              ESP32-S3 XIAO                          │   │
│  │  ┌─────────────┐         ┌─────────────┐           │   │
│  │  │   Core 0    │         │   Core 1    │           │   │
│  │  │ ButtonTask  │──Queue──│ Arduino Loop│           │   │
│  │  │  (10ms)     │         │ Serial+LED  │           │   │
│  │  └─────────────┘         └─────────────┘           │   │
│  └─────────────────────────────────────────────────────┘   │
│           │                         │                       │
│      CD4021BE ×13              74HC595 ×13                  │
│      (Input)                   (Output)                     │
│           │                         │                       │
│      100 Taster                100 LEDs                     │
└─────────────────────────────────────────────────────────────┘
```

## Hardware

### Pinbelegung ESP32-S3 XIAO

| Signal | Pin | Ziel-IC | Funktion |
|--------|-----|---------|----------|
| LED Data | D0 | 74HC595 SER | Serielle Daten |
| LED Clock | D1 | 74HC595 SRCLK | Schiebetakt |
| LED Latch | D2 | 74HC595 RCLK | Ausgabe-Latch |
| Btn Data | D3 | CD4021 Q8 | Serielle Daten (Input!) |
| Btn Clock | D4 | CD4021 CLK | Schiebetakt |
| Btn Load | D5 | CD4021 P/S | Parallel Load |

### Schieberegister-Verkettung

**74HC595 (LED-Ausgabe):**

```
ESP32 D0 → IC#0 SER → IC#0 QH' → IC#1 SER → ... → IC#12 SER
ESP32 D1 → Alle SRCLK (parallel)
ESP32 D2 → Alle RCLK (parallel)
Alle OE → GND
Alle SRCLR → VCC
```

**CD4021BE (Taster-Eingabe):**

```
ESP32 D3 ← IC#0 Q8
ESP32 D4 → Alle CLK (parallel)
ESP32 D5 → Alle P/S (parallel)
IC#0 SER ← IC#1 Q8 ← IC#2 Q8 ← ... ← IC#12 Q8 ← VCC (LETZTER!)
```

### Wichtige Hinweise

1. **CD4021 Load-Logik (invertiert!):**
   - P/S = HIGH → Parallel Load
   - P/S = LOW → Serial Shift

2. **CMOS-Eingänge:**
   - Alle unbenutzten Eingänge auf VCC legen!
   - SER am letzten IC muss auf VCC!

3. **Pull-Ups:**
   - 10kΩ von jedem Taster-Eingang nach VCC
   - Taster schalten nach GND (Active-Low)

## Build-Modi

### Prototyp (10 Taster/LEDs)

```cpp
// In config.h
#define PROTOTYPE_MODE
```

- 2× Schieberegister (16 Bits)
- Bit-Mapping für Breadboard-Verdrahtung aktiv
- Mapping in `config.h` anpassen falls nötig

### Produktion (100 Taster/LEDs)

```cpp
// In config.h
// #define PROTOTYPE_MODE  ← auskommentieren
```

- 13× Schieberegister (104 Bits, 100 genutzt)
- Lineare Verdrahtung, kein Mapping

## Serial-Protokoll (115200 Baud)

### ESP32 → Raspberry Pi

| Nachricht | Bedeutung |
|-----------|-----------|
| `PRESS 001` | Taster 1 gedrückt (001-100) |
| `RELEASE 001` | Taster 1 losgelassen |
| `READY` | Controller bereit |
| `OK` | Befehl ausgeführt |
| `ERROR msg` | Fehler aufgetreten |
| `PONG` | Antwort auf PING |

### Raspberry Pi → ESP32

| Befehl | Funktion |
|--------|----------|
| `LEDSET n` | LED n einschalten (one-hot, 1-100) |
| `LEDON n` | LED n einschalten (additiv) |
| `LEDOFF n` | LED n ausschalten |
| `LEDCLR` | Alle LEDs aus |
| `LEDALL` | Alle LEDs ein |
| `TEST` | LED-Lauflicht starten |
| `STOP` | LED-Test stoppen |
| `PING` | Verbindungstest |
| `STATUS` | Zustand abfragen |
| `VERSION` | Firmware-Version |
| `HELLO` | Startup-Nachricht |
| `HELP` | Hilfe anzeigen |

### Beispiel-Session

```
# ESP32 startet
READY

# Verbindungstest
> PING
PONG

# LED 5 einschalten
> LEDSET 5
OK

# Taster 3 wird gedrückt
PRESS 003

# Taster 3 wird losgelassen
RELEASE 003

# Status abfragen
> STATUS
LEDS 0000010000
BTNS 0000000000
HEAP 372756
CORE 1
QFREE 16
MODE PROTOTYPE
MAPPING ON
```

## Build & Flash

### Voraussetzungen

- [PlatformIO](https://platformio.org/) (VS Code Extension oder CLI)
- USB-C Kabel (Daten-fähig!)

### Kompilieren und Flashen

```bash
cd button_panel_firmware
pio run -t upload -t monitor
```

### Nur kompilieren

```bash
pio run
```

### Nur Monitor

```bash
pio device monitor
```

## Projektstruktur

```
button_panel_firmware/
├── platformio.ini          # PlatformIO Konfiguration
├── include/
│   ├── config.h            # Hardware-Konfiguration & Bit-Mapping
│   └── shift_register.h    # Klassen-Definitionen
└── src/
    ├── main.cpp            # Hauptprogramm & Serial-Protokoll
    └── shift_register.cpp  # Schieberegister-Implementation
```

## Architektur

### Dual-Core FreeRTOS

| Core | Task | Funktion |
|------|------|----------|
| Core 0 | ButtonTask | Taster-Scanning (10ms), Debouncing, Events → Queue |
| Core 1 | Arduino Loop | Serial-Kommunikation, LED-Steuerung, Events ← Queue |

### Thread-Sicherheit

- Mutex-geschützter Zugriff auf Schieberegister
- Queue für Button-Events (16 Slots)
- Watchdog-Überwachung (5s Timeout)

## Bit-Mapping (Prototyp)

Falls die Breadboard-Verdrahtung nicht linear ist, kann das Mapping in `config.h` angepasst werden:

```cpp
constexpr uint8_t BUTTON_BIT_MAP[10] = {
    15,  // Taster 1 → Bit 15
    12,  // Taster 2 → Bit 12
    13,  // Taster 3 → Bit 13
    11,  // Taster 4 → Bit 11
    10,  // Taster 5 → Bit 10
    9,   // Taster 6 → Bit 9
    8,   // Taster 7 → Bit 8
    14,  // Taster 8 → Bit 14
    7,   // Taster 9 → Bit 7
    4    // Taster 10 → Bit 4
};
```

### Mapping ermitteln

1. Test-Firmware `phase3_diagnose` flashen (siehe `selection-panel-tests/`)
2. Jeden Taster einzeln drücken
3. Bit-Position notieren
4. Mapping-Tabelle in `config.h` aktualisieren

## Troubleshooting

### Keine Serial-Ausgabe

1. USB-C Kabel prüfen (muss Daten unterstützen)
2. Nach Flash kurz warten (USB-Enumeration)
3. `HELLO` eingeben um Startup-Nachricht anzuzeigen

### Watchdog-Reset

**Symptom:** ESP32 startet alle ~5 Sekunden neu

**Lösung:** Ist bereits im Code behoben (`vTaskDelay` in loop)

### Taster zeigen Phantom-Drücke

**Ursache:** Floating CMOS-Eingänge

**Lösung:** Alle unbenutzten CD4021-Eingänge auf VCC legen

### Falsche Taster-Nummerierung

1. `phase3_diagnose` flashen
2. Bit-Mapping ermitteln
3. `BUTTON_BIT_MAP` in `config.h` anpassen

## Changelog

### v2.3.0 (2025-12-30)

- Bit-Mapping für Prototyp-Verdrahtung
- 1-basierte Nummerierung (PRESS 001-100)
- HELLO-Befehl hinzugefügt
- CD4021 Timing korrigiert

### v2.2.0 (2025-12-26)

- Dual-Core FreeRTOS Architektur
- Thread-sichere Schieberegister-Klassen
- Queue-basierte Event-Verarbeitung

## Lizenz

MIT License

## Autor

Jan - Selection Panel Projekt
