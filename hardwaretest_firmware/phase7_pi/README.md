# Selection Panel

**10-100 Taster mit LEDs, gesteuert über ESP32-S3 und Raspberry Pi**

Version 2.5.2 | Phase 7 (Pi-Integration abgeschlossen)

## Überblick

Das Selection Panel ist ein modulares Eingabesystem mit physischen Tastern und LED-Feedback. Ein ESP32-S3 (XIAO) liest Taster über CD4021B-Schieberegister ein und steuert LEDs über 74HC595. Ein Raspberry Pi 5 übernimmt die Anwendungslogik: Medien-Wiedergabe (aiohttp + WebSocket), Web-Dashboard und externe Steuerung.

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
   │ Dashboard│                    │  CD4021B  │   │  74HC595  │
   └─────────┘                     │  Taster   │   │   LEDs    │
                                   └─────┬─────┘   └─────┬─────┘
                                         │               │
                                   ┌─────┴─────┐   ┌─────┴─────┐
                                   │ 10 Taster │   │ 10 LEDs   │
                                   └───────────┘   └───────────┘
```

## Features

- **10 Taster** mit zeitbasierter Entprellung (30 ms)
- **10 LEDs** mit PWM-Helligkeitsregelung (verschiedene Farben)
- **FreeRTOS** auf ESP32-S3 (200 Hz I/O-Zyklus)
- **Serial-Protokoll** für Pi-Integration
- **Web-Dashboard** mit WebSocket-Live-Updates
- **Skalierbar** auf 100 Taster/LEDs

## Schnellstart

### Hardware

| Komponente | Typ | Anzahl |
|------------|-----|--------|
| XIAO ESP32-S3 | Mikrocontroller | 1 |
| Raspberry Pi 5 | SBC + Netzteil + microSD | 1 |
| CD4021B | Schieberegister (Input) | 2 |
| 74HC595 | Schieberegister (Output) | 2 |
| Taster | 6×6 mm Tactile | 10 |
| LED | 5 mm, verschiedene Farben | 10 |
| Widerstand | 330 Ω – 3 kΩ (LED) | 10 |
| Widerstand | 10 kΩ (Pull-up) | 10 |
| Kondensator | 100 nF (Stützkondensator) | 4 |

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
# Stabiler USB-Pfad (by-id)
ls /dev/serial/by-id/usb-Espressif*

# Verbindung testen
screen /dev/serial/by-id/usb-Espressif_USB_JTAG_serial_debug_unit_*-if00 115200
# Taster drücken → PRESS 001
```

### Server starten

```bash
cd pi-server
python3 server.py
# Dashboard: http://rover:8080
```

### Python-Beispiel

```python
import serial

# Stabiler Pfad verwenden
port = "/dev/serial/by-id/usb-Espressif_USB_JTAG_serial_debug_unit_98:3D:AE:EA:08:1C-if00"
ser = serial.Serial(port, 115200)

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
├── firmware/                   # ESP32-S3 Firmware (Hauptverzeichnis)
│   ├── src/
│   │   ├── main.cpp            # Entry Point
│   │   ├── app/                # FreeRTOS Tasks
│   │   │   ├── io_task.cpp/h   # I/O-Zyklus (200 Hz)
│   │   │   └── serial_task.cpp/h
│   │   ├── logic/              # Geschäftslogik
│   │   │   ├── debounce.cpp/h  # Entprellung
│   │   │   └── selection.cpp/h # Auswahllogik
│   │   ├── drivers/            # Hardware-Treiber
│   │   │   ├── cd4021.cpp/h    # Taster-Input
│   │   │   └── hc595.cpp/h     # LED-Output
│   │   └── hal/                # Hardware Abstraction
│   │       └── spi_bus.cpp/h   # SPI-Bus
│   ├── include/
│   │   ├── config.h            # Konfiguration
│   │   ├── types.h             # Datentypen
│   │   └── bitops.h            # Bit-Operationen
│   ├── docs/                   # Firmware-Dokumentation
│   │   ├── 00-overview.md
│   │   ├── 01-wiring.md
│   │   ├── 02-firmware-architecture.md
│   │   ├── 03-debug-playbook.md
│   │   ├── 04-test-plan.md
│   │   └── 05-performance-notes.md
│   ├── platformio.ini
│   └── README.md
├── pi-server/                  # Raspberry Pi Server
│   ├── server.py               # aiohttp + WebSocket
│   ├── static/                 # Web-Dashboard (HTML/JS/CSS)
│   └── media/                  # Medien (001.mp3, 001.jpg, ...)
└── docs/                       # Projekt-Dokumentation
```

## Protokoll-Übersicht

### Serial: ESP32 → Pi

| Nachricht | Beschreibung |
|-----------|--------------|
| `READY` | System bereit |
| `FW <version>` | Firmware-Version |
| `PRESS <id>` | Taster gedrückt (001-100) |
| `RELEASE <id>` | Taster losgelassen |
| `PONG` | Antwort auf PING |

### Serial: Pi → ESP32

| Befehl | Beschreibung |
|--------|--------------|
| `LEDSET <id>` | One-Hot: nur diese LED an |
| `LEDON <id>` | LED einschalten (additiv) |
| `LEDOFF <id>` | LED ausschalten |
| `LEDCLR` | Alle LEDs aus |
| `LEDALL` | Alle LEDs an |
| `PING` | Verbindung prüfen → PONG |
| `STATUS` | Status abfragen |

### WebSocket: Server ↔ Browser

| Richtung | Message | Beschreibung |
|----------|---------|--------------|
| Server → Browser | `{"type":"play","id":3}` | Starte Wiedergabe |
| Server → Browser | `{"type":"stop"}` | Stoppe Wiedergabe |
| Browser → Server | `{"type":"ended","id":3}` | Wiedergabe beendet |

## Konfiguration

Wichtige Parameter in `firmware/include/config.h`:

```cpp
constexpr uint8_t BTN_COUNT = 10;        // Anzahl Taster
constexpr uint8_t LED_COUNT = 10;        // Anzahl LEDs
constexpr uint32_t IO_PERIOD_MS = 5;     // Abtastrate (200 Hz)
constexpr uint32_t DEBOUNCE_MS = 30;     // Entprellzeit
constexpr uint8_t PWM_DUTY_PERCENT = 50; // LED-Helligkeit
constexpr bool LATCH_SELECTION = true;   // Auswahl persistent
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
| 8 | 🔲 | 100-Button |

## Lizenz

MIT License

## Autor & Maintainer

Jan Unger

## Credits

Unterstützt durch KI-Tools (Claude, ChatGPT).
