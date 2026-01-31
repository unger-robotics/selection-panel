# Selection Panel

**10-100 Taster mit LEDs, gesteuert über ESP32-S3 und Raspberry Pi**

Version 2.5.3 | Phase 7 (Pi-Integration abgeschlossen)

## Überblick

Das Selection Panel ist ein modulares Eingabesystem mit physischen Tastern und LED-Feedback. Ein ESP32-S3 (XIAO) liest Taster über CD4021B-Schieberegister ein und steuert LEDs über 74HC595. Ein Raspberry Pi 5 übernimmt die Anwendungslogik: Medien-Wiedergabe über ein Web-Dashboard (aiohttp + WebSocket).

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

- **10-100 Taster** mit zeitbasierter Entprellung (30 ms)
- **10-100 LEDs** mit PWM-Helligkeitsregelung
- **FreeRTOS** auf ESP32-S3 (200 Hz I/O-Zyklus)
- **Serial-Protokoll** für Pi-Integration
- **Web-Dashboard** zur Fernsteuerung

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
pio run -t upload -t monitor
```

Erwartete Ausgabe:

```
READY
FW SelectionPanel v2.5.2
```

### Pi-Server starten

```bash
cd ~/selection-panel/server
source ../venv/bin/activate
python server.py

# Web-Dashboard: http://rover:8080/
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

## Protokoll

**Wichtig:** Alle IDs sind 1-basiert und 3-stellig formatiert (001-100).

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
| `PING` | Verbindung prüfen → `PONG` |

## Konfiguration

### Firmware (`firmware/include/config.h`)

```cpp
constexpr uint8_t BTN_COUNT = 10;       // Skalierbar auf 100
constexpr uint8_t LED_COUNT = 10;
constexpr uint32_t IO_PERIOD_MS = 5;    // 200 Hz
constexpr uint32_t DEBOUNCE_MS = 30;
constexpr uint8_t PWM_DUTY_PERCENT = 50;
```

### Server (`server/server.py`)

```python
PROTOTYPE_MODE = True   # True = 10 Medien, False = 100
ESP32_SETS_LED_LOCALLY = True
```

## Dokumentation

| Dokument | Beschreibung |
|----------|--------------|
| [QUICKSTART.md](doc/md/QUICKSTART.md) | Schnelleinstieg |
| [SELECTION-PANEL-ARCHITEKTUR.md](doc/md/SELECTION-PANEL-ARCHITEKTUR.md) | Systemarchitektur |
| [PROTOCOL.md](doc/md/PROTOCOL.md) | Serial + WebSocket Protokoll |
| [PI-INTEGRATION.md](doc/md/PI-INTEGRATION.md) | Raspberry Pi Setup |
| [firmware/docs/HARDWARE.md](firmware/docs/HARDWARE.md) | Schaltpläne, Pin-Belegung |
| [firmware/docs/DEVELOPER.md](firmware/docs/DEVELOPER.md) | Firmware-Entwicklung |

## Entwicklungsphasen

| Phase | Status | Beschreibung |
|-------|--------|--------------|
| 1-6 | ✅ | ESP32 → LEDs → Taster → FreeRTOS → Modular |
| 7 | ✅ | Raspberry Pi Bridge |
| 8 | 🔲 | 100x Taster + LEDs + Multimedia |

## Lizenz

MIT License

## Autor

Jan Unger

## Credits

Unterstützt durch KI-Tools (Claude Code, ChatGPT).
