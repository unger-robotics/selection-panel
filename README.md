# Selection Panel v2.4.2

Interaktives Auswahlpanel mit 100 Tastern und LEDs für Multimedia-Wiedergabe.

```
┌─────────────────────────────────────────────────────────────────────┐
│                         ARCHITEKTUR                                 │
├─────────────────────────────────────────────────────────────────────┤
│                                                                     │
│   ┌─────────┐    Serial     ┌─────────────┐   WebSocket   ┌──────┐ │
│   │ ESP32-S3│◄─────────────►│   Pi Server │◄─────────────►│Browser│ │
│   │  XIAO   │   115200 Bd   │  (Python)   │   Port 8080   │      │ │
│   └────┬────┘               └──────┬──────┘               └──────┘ │
│        │                           │                               │
│   ┌────┴────┐                ┌─────┴─────┐                         │
│   │ 100 LEDs│                │   media/  │                         │
│   │ 100 BTN │                │ 001.jpg/mp3│                        │
│   └─────────┘                │ ...       │                         │
│                              │ 100.jpg/mp3│                        │
│                              └───────────┘                         │
└─────────────────────────────────────────────────────────────────────┘
```

## Schnellstart

```bash
# 1. Repository klonen
git clone <repo-url>
cd selection-panel

# 2. Server starten
python3 server.py

# 3. Browser öffnen
# http://rover.local:8080
```

Für Details siehe [docs/QUICKSTART.md](docs/QUICKSTART.md).

## Features

- **Sofortige Reaktion:** LED leuchtet bei Tastendruck in < 1ms (ESP32 lokal)
- **Preempt-Policy:** Neuer Tastendruck unterbricht laufende Wiedergabe sofort
- **One-Hot:** Immer nur eine LED aktiv
- **Parallele Verarbeitung:** Serial- und WebSocket-Kommunikation gleichzeitig
- **Robuste Verbindung:** Auto-Reconnect bei Serial-Unterbrechung
- **Medien-Validierung:** Prüfung aller Dateien beim Start

## Systemkomponenten

| Komponente | Version | Beschreibung |
|------------|---------|--------------|
| ESP32-S3 Firmware | v2.4.1 | Dual-Core FreeRTOS, Taster/LED-Steuerung |
| Pi Server | v2.4.2 | aiohttp WebSocket + Serial Bridge |
| Web Dashboard | v1.0.0 | Bild- und Audio-Wiedergabe |

## Projektstruktur

```
selection-panel/
├── server.py              # Hauptserver (Python)
├── static/
│   └── index.html         # Web-Dashboard
├── media/
│   ├── 001.jpg            # Bilder (1-100)
│   ├── 001.mp3            # Audio (1-100)
│   └── ...
├── button_panel_firmware/ # ESP32 Firmware
│   ├── src/
│   │   ├── main.cpp
│   │   └── shift_register.cpp
│   └── include/
│       ├── config.h
│       └── shift_register.h
└── docs/                  # Dokumentation
    ├── QUICKSTART.md
    ├── SPEC.md
    ├── FIRMWARE.md
    ├── SERVER.md
    ├── DASHBOARD.md
    └── ...
```

## Konfiguration

### Server (server.py)

```python
# Build-Modus
PROTOTYPE_MODE = True      # True = 10, False = 100 Medien

# Serial-Verbindung
SERIAL_PORT = "/dev/ttyACM0"
SERIAL_BAUD = 115200

# HTTP-Server
HTTP_HOST = "0.0.0.0"
HTTP_PORT = 8080

# Optimierung (ESP32 v2.4.1+)
ESP32_SETS_LED_LOCALLY = True  # ESP32 setzt LED selbst
```

### Firmware (config.h)

```cpp
// Build-Modus
#define PROTOTYPE_MODE     // Auskommentieren für Produktion
```

## Serial-Protokoll

### ESP32 → Server

| Nachricht | Bedeutung |
|-----------|-----------|
| `PRESS 001` | Taster 1 gedrückt (001-100) |
| `RELEASE 001` | Taster 1 losgelassen |
| `READY` | Controller bereit |
| `OK` | Befehl ausgeführt |
| `ERROR msg` | Fehler aufgetreten |

### Server → ESP32

| Befehl | Funktion |
|--------|----------|
| `LEDSET n` | LED n einschalten (one-hot) |
| `LEDCLR` | Alle LEDs aus |
| `PING` | Verbindungstest |

## HTTP-Endpoints

| Endpoint | Methode | Beschreibung |
|----------|---------|--------------|
| `/` | GET | Web-Dashboard |
| `/ws` | WS | WebSocket-Verbindung |
| `/status` | GET | Server-Status (JSON) |
| `/health` | GET | Health-Check |
| `/test/play/{id}` | GET | Tastendruck simulieren |
| `/test/stop` | GET | Wiedergabe stoppen |
| `/media/{file}` | GET | Mediendateien |
| `/static/{file}` | GET | Statische Dateien |

### Status-Response

```json
{
  "version": "2.4.2",
  "mode": "prototype",
  "num_media": 10,
  "current_button": 3,
  "ws_clients": 2,
  "serial_connected": true,
  "serial_port": "/dev/ttyACM0",
  "media_missing": 0,
  "esp32_local_led": true
}
```

## WebSocket-Protokoll

### Server → Browser

```json
{"type": "stop"}              // Wiedergabe stoppen
{"type": "play", "id": 3}     // Medien-ID 3 abspielen
```

### Browser → Server

```json
{"type": "ended", "id": 3}    // Wiedergabe von ID 3 beendet
{"type": "ping"}              // Heartbeat
```

## Medien-Anforderungen

Alle Medien im Verzeichnis `media/` mit 3-stelliger ID (001-100):

| Datei | Format | Beschreibung |
|-------|--------|--------------|
| `001.jpg` | JPEG | Bild für Taster 1 |
| `001.mp3` | MP3 | Audio für Taster 1 |
| ... | ... | ... |
| `100.jpg` | JPEG | Bild für Taster 100 |
| `100.mp3` | MP3 | Audio für Taster 100 |

Der Server prüft beim Start, ob alle Dateien vorhanden sind.

## Dokumentation

| Dokument | Inhalt |
|----------|--------|
| [QUICKSTART](docs/QUICKSTART.md) | Schnelleinstieg |
| [SPEC](docs/SPEC.md) | Technische Spezifikation |
| [FIRMWARE](docs/FIRMWARE.md) | ESP32 Firmware Details |
| [SERVER](docs/SERVER.md) | Python Server Details |
| [DASHBOARD](docs/DASHBOARD.md) | Web-Interface |
| [HARDWARE](docs/HARDWARE.md) | Schaltplan, Verkabelung |
| [LOETEN](docs/LOETEN.md) | Löt-Anleitung |
| [COMMANDS](docs/COMMANDS.md) | Befehlsreferenz |
| [RUNBOOK](docs/RUNBOOK.md) | Betriebshandbuch |
| [SSH](docs/SSH.md) | Remote-Zugriff |
| [GIT](docs/GIT.md) | Versionskontrolle |
| [CHANGELOG](docs/CHANGELOG.md) | Änderungshistorie |
| [ROADMAP](docs/ROADMAP.md) | Geplante Features |
| [VORAUSSETZUNGEN](docs/VORAUSSETZUNGEN.md) | Systemanforderungen |

## Troubleshooting

### Server startet nicht

```bash
# Port bereits belegt?
sudo lsof -i :8080

# Serial-Berechtigung?
sudo usermod -a -G dialout $USER
# Neu einloggen erforderlich!
```

### Keine Serial-Verbindung

```bash
# Port vorhanden?
ls -la /dev/ttyACM*

# ESP32 flashen
cd button_panel_firmware
pio run -t upload
```

### Medien fehlen

```bash
# Prüfen
ls media/*.jpg | wc -l  # Sollte 10 oder 100 sein
ls media/*.mp3 | wc -l

# Status-Endpoint
curl http://localhost:8080/status | jq .missing_files
```

## Changelog

### v2.4.2 (2025-01-01)

- Parallele Ausführung von Serial + WebSocket (asyncio.gather)
- LEDSET wird nur gesendet wenn ESP32 LED nicht selbst gesetzt hat
- Minimale Latenz durch parallele Broadcasts

### v2.4.1 (2025-01-01)

- ESP32: LED reagiert sofort bei Tastendruck (< 1ms)
- Redundantes LEDSET wird ignoriert

### v2.3.1 (2025-01-01)

- CD4021 Timing-Fixes

### v2.3.0 (2025-12-30)

- Bit-Mapping für Prototyp
- 1-basierte Nummerierung

## Lizenz

MIT License

## Autor

Jan Unger - Selection Panel Projekt
