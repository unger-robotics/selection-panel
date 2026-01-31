# Selection Panel: Raspberry Pi Integration (Phase 7)

Version 2.5.3 | Raspberry Pi 5 + ESP32-S3 | Stand: 2026-01-31

## Übersicht

In Phase 7 wird der ESP32-S3 zum reinen I/O-Controller. Der Raspberry Pi 5 übernimmt die Anwendungslogik: Er empfängt Taster-Events über Serial, sendet sie per WebSocket an das Web-Dashboard und steuert bei Bedarf die LEDs.

```
┌─────────────────────────────────────────────────────────────────────────────┐
│                           RASPBERRY PI 5                                    │
│                                                                             │
│  ┌─────────────────────────────────────────────────────────────────────┐    │
│  │                      server.py (aiohttp)                            │    │
│  │                                                                     │    │
│  │   Serial-Thread ◀────────────▶ Asyncio Event Loop                   │    │
│  │        │                              │                             │    │
│  │        ▼                              ▼                             │    │
│  │   PRESS 001 ──────────────▶ WebSocket Broadcast                     │    │
│  │                           {"type":"play","id":1}                    │    │
│  └─────────────────────────────────────┬───────────────────────────────┘    │
│                                        │                                    │
│              ┌─────────────────────────┼─────────────────────────┐          │
│              │                         │                         │          │
│              ▼                         ▼                         ▼          │
│         /static/               /ws (WebSocket)              /media/         │
│         app.js, CSS            Browser-Clients              001.jpg/.mp3    │
│                                                                             │
└─────────────────────────────────────────┬───────────────────────────────────┘
                                          │
                      USB-CDC @ 115200 Baud (by-id Pfad)
                                          │
┌─────────────────────────────────────────┴───────────────────────────────────┐
│                           ESP32-S3 (XIAO)                                   │
│                                                                             │
│   PRESS/RELEASE Events ──────────────▶ Serial TX                            │
│   LED-Steuerung ◀──────────────────── Serial RX (LEDSET, optional)          │
│                                                                             │
│   Hinweis: ESP32 setzt LED bei Tastendruck selbst (ESP32_SETS_LED_LOCALLY)  │
└─────────────────────────────────────────────────────────────────────────────┘
```

## Serial-Verbindung

### Stabiler Device-Pfad (by-id)

Der ESP32-S3 erscheint als USB-CDC-Gerät. Der Pfad `/dev/ttyACM0` kann sich nach Reboot ändern. Die Lösung: Verwende den stabilen by-id Pfad:

```bash
# Stabilen Pfad ermitteln
ls -la /dev/serial/by-id/
# → usb-Espressif_USB_JTAG_serial_debug_unit_98:3D:AE:EA:08:1C-if00 -> ../../ttyACM0

# Dieser Pfad bleibt stabil
SERIAL_PORT="/dev/serial/by-id/usb-Espressif_USB_JTAG_serial_debug_unit_98:3D:AE:EA:08:1C-if00"
```

### Berechtigungen

```bash
# Benutzer zur dialout-Gruppe hinzufügen
sudo usermod -aG dialout $USER
# Ausloggen und wieder einloggen!

# Verbindung testen
screen $SERIAL_PORT 115200
# → READY
# → FW SelectionPanel v2.5.2
```

## Server-Architektur

### Komponenten

Der Python-Server (`server/server.py`) verwendet **aiohttp** für asynchrone HTTP/WebSocket-Verarbeitung:

| Komponente | Technologie | Funktion |
|------------|-------------|----------|
| HTTP-Server | aiohttp | Statische Dateien, API-Endpoints |
| WebSocket | aiohttp | Echtzeit-Kommunikation mit Browser |
| Serial-Reader | Threading | Liest ESP32-Events im Hintergrund |
| Media-Validator | Startup | Prüft ob alle Medien vorhanden |

### HTTP-Endpoints

| Endpoint | Methode | Beschreibung |
|----------|---------|--------------|
| `/` | GET | Web-Dashboard (index.html) |
| `/ws` | WebSocket | Echtzeit-Events |
| `/static/` | GET | JavaScript, CSS, Favicon |
| `/media/` | GET | Bilder und Audio (001.jpg, 001.mp3) |
| `/status` | GET | Server-Status als JSON |
| `/health` | GET | Health-Check (für Monitoring) |
| `/test/play/{id}` | GET | Simuliert Tastendruck |
| `/test/stop` | GET | Stoppt Wiedergabe |

### Konfiguration

```python
# server/server.py - Wichtige Einstellungen

VERSION = "2.5.3"

# Build-Modus
PROTOTYPE_MODE = True   # True = 10 Medien, False = 100 Medien
NUM_MEDIA = 10 if PROTOTYPE_MODE else 100

# Serial-Port (stabiler by-id Pfad!)
SERIAL_PORT = "/dev/serial/by-id/usb-Espressif_USB_JTAG_serial_debug_unit_98:3D:AE:EA:08:1C-if00"
SERIAL_BAUD = 115200

# HTTP-Server
HTTP_HOST = "0.0.0.0"
HTTP_PORT = 8080

# ESP32 setzt LED selbst bei Tastendruck
ESP32_SETS_LED_LOCALLY = True
```

## WebSocket-Protokoll

Die Kommunikation zwischen Server und Browser erfolgt über WebSocket:

### Server → Browser

```json
// Wiedergabe starten
{"type": "play", "id": 3}

// Wiedergabe stoppen
{"type": "stop"}
```

### Browser → Server

```json
// Audio beendet (für LED-Clear)
{"type": "ended", "id": 3}

// Heartbeat
{"type": "ping"}
```

## Serial-Protokoll (ESP32 ↔ Pi)

### ESP32 → Pi

| Nachricht | Bedeutung | Beispiel |
|-----------|-----------|----------|
| `READY` | ESP32 bereit | `READY` |
| `FW <version>` | Firmware-Version | `FW SelectionPanel v2.5.2` |
| `PRESS <id>` | Taster gedrückt | `PRESS 001` |
| `RELEASE <id>` | Taster losgelassen | `RELEASE 001` |
| `PONG` | Antwort auf PING | `PONG` |
| `OK` | Befehl ausgeführt | `OK` |

### Pi → ESP32

| Befehl | Bedeutung | Wann gesendet |
|--------|-----------|---------------|
| `LEDSET <id>` | One-Hot LED | Nur wenn `ESP32_SETS_LED_LOCALLY=False` |
| `LEDCLR` | Alle LEDs aus | Nach Audio-Ende |
| `PING` | Verbindungstest | Bei Bedarf |

**Wichtig:** Mit `ESP32_SETS_LED_LOCALLY=True` setzt der ESP32 die LED bei Tastendruck selbst. Der Server muss dann nur noch `LEDCLR` nach Audio-Ende senden.

## Datenfluss

```
Taster 3 gedrückt
       │
       ▼
┌──────────────┐
│   ESP32-S3   │
│ • Entprellt  │
│ • LED 3 an   │ ←── ESP32 setzt LED selbst!
│ • Serial TX  │
└──────┬───────┘
       │ "PRESS 003\n"
       ▼
┌──────────────┐
│  server.py   │
│ Serial-Thread│
└──────┬───────┘
       │ asyncio Queue
       ▼
┌──────────────┐
│ Event Loop   │
│ • Broadcast  │
└──────┬───────┘
       │ {"type":"play","id":3}
       ▼
┌──────────────┐
│   Browser    │
│ • Bild laden │
│ • Audio play │
└──────────────┘
       │
       │ Audio endet
       ▼
┌──────────────┐
│   Browser    │
│ • ended senden│
└──────┬───────┘
       │ {"type":"ended","id":3}
       ▼
┌──────────────┐
│  server.py   │
│ • LEDCLR TX  │
└──────┬───────┘
       │ "LEDCLR\n"
       ▼
┌──────────────┐
│   ESP32-S3   │
│ • Alle LEDs  │
│   aus        │
└──────────────┘
```

## Web-Dashboard

### Features

Das Dashboard (`index.html` + `app.js`) bietet:

- **Audio-Unlock**: Button zum Entsperren der Browser-Autoplay-Policy
- **Medien-Preload**: Lädt alle Bilder/Audio nach Unlock vor
- **Echtzeit-Anzeige**: Aktuelles Bild + Audio-Fortschritt
- **Keyboard-Shortcuts**: Space = Play/Pause, Ctrl+D = Debug
- **Debug-Panel**: Zeigt alle Events (ausklappbar)

### Zugriff

```bash
# Server starten
cd /home/pi/selection-panel/server
python3 server.py

# Browser öffnen
# Lokal:   http://localhost:8080/
# LAN:     http://rover:8080/
# IP:      http://192.168.1.24:8080/
```

### Status-API

```bash
# Server-Status abfragen
curl http://rover:8080/status | jq

{
  "version": "2.5.2",
  "mode": "prototype",
  "num_media": 10,
  "current_button": null,
  "ws_clients": 1,
  "serial_connected": true,
  "serial_port": "/dev/serial/by-id/...",
  "media_missing": 0,
  "esp32_local_led": true
}
```

## USB-Port-Verwaltung (AMR-Koexistenz)

Auf dem Pi läuft auch das AMR-Projekt, das denselben ESP32-Port nutzen kann. Ein **flock-basiertes Locking** verhindert Konflikte:

### Lock-Mechanismus

```bash
# Lock-Datei
/var/lock/esp32-serial.lock

# Selection Panel: Non-blocking (startet nicht wenn belegt)
flock -n /var/lock/esp32-serial.lock python3 server.py

# AMR Agent: Blocking (wartet bis frei)
flock /var/lock/esp32-serial.lock micro_ros_agent ...
```

### Schneller Wechsel

```bash
# → Selection Panel Modus
sudo systemctl stop selection-panel.service  # Falls AMR läuft
sudo systemctl start selection-panel.service
sudo journalctl -u selection-panel.service -f

# → AMR Modus
sudo systemctl stop selection-panel.service
cd /home/pi/amr/docker
sudo docker compose -p docker up -d microros_agent
```

### Kontrolle

```bash
# Wer hält den USB-Port?
sudo fuser -v /dev/ttyACM0

# Wer hält den Lock?
sudo lslocks | grep esp32-serial
```

## systemd-Service

### Service-Datei

```ini
# /etc/systemd/system/selection-panel.service
[Unit]
Description=Selection Panel Server
After=network.target

[Service]
Type=simple
User=pi
WorkingDirectory=/home/pi/selection-panel/server
# flock -n: Startet nur wenn Lock frei
ExecStart=/usr/bin/flock -n /var/lock/esp32-serial.lock /home/pi/selection-panel/venv/bin/python /home/pi/selection-panel/server/server.py
Restart=on-failure
RestartSec=5

[Install]
WantedBy=multi-user.target
```

### Aktivierung

```bash
# Service registrieren
sudo systemctl daemon-reload

# Manueller Start
sudo systemctl start selection-panel.service

# Autostart aktivieren
sudo systemctl enable selection-panel.service

# Status prüfen
sudo systemctl status selection-panel.service

# Logs verfolgen
journalctl -u selection-panel.service -f
```

## Medien-Struktur

```
server/media/
├── 001.jpg    # Bild für Taster 1
├── 001.mp3    # Audio für Taster 1
├── 002.jpg
├── 002.mp3
├── ...
├── 010.jpg
└── 010.mp3
```

### Validierung beim Start

Der Server prüft beim Start ob alle erwarteten Medien vorhanden sind:

```
2026-01-08 [INFO] Medien-Validierung: 10/10 vollständig

# Oder bei fehlenden Dateien:
2026-01-08 [WARNING] Fehlende Medien: 2 Dateien
2026-01-08 [WARNING]   - 005.jpg
2026-01-08 [WARNING]   - 005.mp3
```

### Test-Medien generieren

```bash
# Dummy-Medien für Tests erstellen
cd /home/pi/selection-panel
./scripts/generate_test_media.sh
```

## Troubleshooting

### ESP32 nicht erkannt

```bash
# USB-Geräte auflisten
lsusb | grep Espressif
# → Espressif USB JTAG/serial debug unit

# Device-Pfad prüfen
ls -la /dev/serial/by-id/
ls /dev/ttyACM*

# Falls Pfad geändert: In server.py anpassen
```

### Keine READY-Nachricht

```bash
# Serial direkt testen
screen /dev/ttyACM0 115200
# ESP32 per Reset-Taste neu starten
# → READY sollte erscheinen
```

### WebSocket verbindet nicht

```bash
# Server läuft?
sudo systemctl status selection-panel.service

# Port offen?
ss -tlnp | grep 8080

# Firewall?
sudo ufw allow 8080/tcp
```

### Audio spielt nicht

1. "Sound aktivieren" Button im Browser klicken
2. Browser-Konsole auf Fehler prüfen (F12)
3. Medien-Dateien vorhanden? → `/status` Endpoint prüfen

### Port-Konflikt mit AMR

```bash
# Wer nutzt den Port?
sudo fuser -v /dev/ttyACM0

# Selection Panel stoppen
sudo systemctl stop selection-panel.service

# Lock prüfen
sudo lslocks | grep esp32-serial
```

## Latenz-Analyse

| Station | Typische Latenz |
|---------|-----------------|
| Taster → ESP32 (Debounce) | 30 ms |
| ESP32 → Serial TX | < 1 ms |
| Serial → Server | < 1 ms |
| Server → WebSocket | < 1 ms |
| Browser → Audio Start | 5-50 ms (gecached: ~5 ms) |
| **Gesamt (Preloaded)** | **~40 ms** |
| **Gesamt (Nicht gecached)** | **~200 ms** |

Das Medien-Preloading im Browser reduziert die Latenz erheblich.

---

*Stand: 2026-01-31 | Version 2.5.3*
