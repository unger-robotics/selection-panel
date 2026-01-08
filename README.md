# Selection Panel

**10-100 Taster mit LEDs, gesteuert über ESP32-S3 und Raspberry Pi**

Version 2.5.2 | Phase 7 (Pi-Integration abgeschlossen)

## Überblick

Das Selection Panel ist ein modulares Eingabesystem mit physischen Tastern und LED-Feedback. Ein ESP32-S3 (XIAO) liest Taster über CD4021B-Schieberegister ein und steuert LEDs über 74HC595. Ein Raspberry Pi 5 übernimmt die Anwendungslogik: Medien-Wiedergabe über ein Web-Dashboard (aiohttp + WebSocket) und externe Steuerung.

```
┌───────────────────┐     USB-CDC      ┌───────────────────┐
│   Raspberry Pi 5  │◀────────────────▶│   ESP32-S3 XIAO   │
│                   │   115200 Baud    │                   │
│  • Media Player   │                  │  • 200 Hz I/O     │
│  • Web Dashboard  │   PRESS 001      │  • Entprellung    │
│  • Python API     │   LEDSET 001     │  • LED-PWM        │
└───────────────────┘                  └─────────┬─────────┘
                                                 │ SPI
                              ┌──────────────────┴──────────────────┐
                              │                                     │
                        ┌─────┴─────┐                         ┌─────┴─────┐
                        │  CD4021B  │                         │  74HC595  │
                        │  Taster   │                         │   LEDs    │
                        └─────┬─────┘                         └─────┬─────┘
                              │                                     │
                        ┌─────┴─────┐                         ┌─────┴─────┐
                        │ 10 Taster │                         │ 10 LEDs   │
                        └───────────┘                         └───────────┘
```

## Features

- **10 Taster** mit zeitbasierter Entprellung (30 ms)
- **10 LEDs** mit PWM-Helligkeitsregelung
- **FreeRTOS** auf ESP32-S3 (200 Hz I/O-Zyklus)
- **Serial-Protokoll** für Pi-Integration
- **Web-Dashboard** zur Fernsteuerung
- **Skalierbar** auf 100 Taster/LEDs

## Schnellstart

### Hardware

| Komponente | Typ | Anzahl |
|------------|-----|--------|
| XIAO ESP32-S3 | Mikrocontroller | 1 |
| CD4021B | Schieberegister (Input) | 2 |
| 74HC595 | Schieberegister (Output) | 2 |
| Taster | 6×6 mm | 10 |
| LED | 5 mm rot | 10 |
| Widerstand | 220 Ω (LED) | 10 |
| Widerstand | 10 kΩ (Pull-up) | 10 |

### Firmware flashen

```bash
cd firmware
pio run -t upload
pio device monitor
```

Erwartete Ausgabe:

```
READY
FW SelectionPanel v2.5.2
```

### Pi-Verbindung

```bash
# Stabilen USB-Pfad ermitteln
ls /dev/serial/by-id/

# Server starten
cd selection-panel
python3 server.py

# Web-Dashboard öffnen
# http://rover.local:8080/
# oder http://192.168.1.24:8080/
```

### Minimales Python-Beispiel

```python
import serial

ser = serial.Serial("/dev/ttyACM0", 115200)

while True:
    line = ser.readline().decode().strip()
    if line.startswith("PRESS"):
        btn_id = line.split()[1]
        print(f"Button {btn_id} pressed")
        ser.write(f"LEDSET {btn_id}\n".encode())
```

## Projektstruktur

```
selection-panel/
├── firmware/               # ESP32-S3 Firmware
│   ├── src/
│   │   ├── main.cpp        # Entry Point
│   │   ├── app/            # FreeRTOS Tasks
│   │   ├── logic/          # Debounce, Selection
│   │   ├── drivers/        # CD4021, HC595
│   │   └── hal/            # SPI-Abstraktion
│   └── include/
│       ├── config.h        # Konfiguration
│       ├── types.h         # Datentypen
│       └── bitops.h        # Bit-Operationen
├── server.py               # Python-Server
├── static/                 # Web-Dashboard
├── media/                  # Sound/Bilder (001.mp3, 001.jpg)
└── docs/                   # Dokumentation
```

## Dokumentation

| Dokument | Beschreibung |
|----------|--------------|
| [selection-panel-architektur.md](docs/selection-panel-architektur.md) | Systemarchitektur, Schichtenmodell |
| [HARDWARE.md](docs/HARDWARE.md) | Schaltpläne, Pin-Belegung |
| [firmware-code-guide.md](docs/firmware-code-guide.md) | Firmware-Struktur, Module |
| [PI-INTEGRATION.md](docs/PI-INTEGRATION.md) | Raspberry Pi Setup, WebSocket-API |
| [PROTOCOL.md](docs/PROTOCOL.md) | Serial + WebSocket Protokoll |
| [usb-port-verwaltung.md](docs/usb-port-verwaltung.md) | Port-Sharing mit AMR-Projekt |

## Protokoll-Übersicht

### ESP32 → Pi

| Nachricht | Beschreibung |
|-----------|--------------|
| `READY` | System bereit |
| `FW <version>` | Firmware-Version |
| `PRESS <id>` | Taster gedrückt (001-100) |
| `RELEASE <id>` | Taster losgelassen |

### Pi → ESP32

| Befehl | Beschreibung |
|--------|--------------|
| `LEDSET <id>` | One-Hot: nur diese LED an |
| `LEDON <id>` | LED einschalten (additiv) |
| `LEDOFF <id>` | LED ausschalten |
| `LEDCLR` | Alle LEDs aus |
| `LEDALL` | Alle LEDs an |
| `PING` | Verbindung prüfen |

## Konfiguration

Wichtige Parameter in `include/config.h`:

```cpp
constexpr uint8_t BTN_COUNT = 10;       // Anzahl Taster
constexpr uint8_t LED_COUNT = 10;       // Anzahl LEDs
constexpr uint32_t IO_PERIOD_MS = 5;    // Abtastrate (200 Hz)
constexpr uint32_t DEBOUNCE_MS = 30;    // Entprellzeit
constexpr uint8_t PWM_DUTY_PERCENT = 50; // LED-Helligkeit
```

## Entwicklungsphasen

| Phase | Status | Beschreibung |
|-------|--------|--------------|
| 1 | ✅ | ESP32 Grundtest |
| 2 | ✅ | LED-SPI (74HC595) |
| 3 | ✅ | Button-SPI (CD4021B) |
| 4 | ✅ | Combined SPI |
| 5 | ✅ | FreeRTOS Integration |
| 6 | ✅ | Modulare Architektur |
| 7 | ✅ | Raspberry Pi Bridge |
| 8 | 🔲 | 100x Button + LEDs + Multimedia |

## Lizenz

MIT License

## Autor & Maintainer

Jan Unger

## Credits

Unterstützt durch KI-Tools (Claude 4.5, ChatGPT 5.2).
