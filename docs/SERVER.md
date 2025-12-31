# SERVER — Raspberry Pi Gateway

> Python-Server als Brücke zwischen ESP32 und Browser.

| Metadaten | Wert |
|-----------|------|
| Version | 2.4.1 |
| Stand | 2025-12-30 |
| Status | ✅ Prototyp funktionsfähig |
| Plattform | Raspberry Pi 5, Pi OS Lite |
| Python | 3.13+ |

---

## 1 Überblick

Der Server empfängt Tastendruck-Events vom ESP32 per USB-Serial und verteilt sie per **WebSocket** an alle Browser. WebSocket ist eine bidirektionale Verbindung – der Server kann jederzeit Nachrichten senden, ohne dass der Client fragt.

```
ESP32 ──USB-Serial──► Server ──WebSocket──► Browser(s)
         115200 Baud           :8080
        PRESS 001-100         play/stop
```

**Technologie:** Der Server nutzt **aiohttp** für HTTP/WebSocket und **os.open + stty** für Serial (statt pyserial, das nicht zuverlässig mit ESP32-S3 USB-CDC funktioniert).

---

## 2 Verzeichnisstruktur

```
/home/pi/selection-panel/
├── venv/                    # Isolierte Python-Umgebung
├── server.py                # Hauptserver (v2.4.1)
├── selection-panel.service  # Autostart-Konfiguration
├── static/                  # Dashboard-Dateien
│   ├── index.html
│   ├── styles.css
│   └── app.js
└── media/                   # Bild/Audio-Dateien (1-basiert!)
    ├── 001.jpg  001.mp3
    ├── 002.jpg  002.mp3
    └── ...      (bis 010 oder 100)
```

**venv:** Eine isolierte Python-Umgebung. Pakete werden nur hier installiert, nicht systemweit – verhindert Versionskonflikte.

**Medien:** 1-basierte Nummerierung! `001.jpg` bis `010.jpg` (Prototyp) oder `100.jpg` (Produktion).

---

## 3 Installation

### 3.1 Python-Umgebung

```bash
# Umgebung erstellen
python3 -m venv ~/selection-panel/venv

# Pakete installieren
~/selection-panel/venv/bin/pip install aiohttp
```

**Hinweis:** `pyserial` wird nicht mehr benötigt! Der Server nutzt `os.open` + `stty` für die Serial-Kommunikation.

### 3.2 Serial-Port

Der ESP32-S3 XIAO erscheint als `/dev/ttyACM0`:

```bash
ls -la /dev/ttyACM*
```

In `server.py`:
```python
SERIAL_PORT = "/dev/ttyACM0"
```

---

## 4 Konfiguration (server.py)

```python
# Skalierung: True = 10 Medien, False = 100 Medien
PROTOTYPE_MODE = True
NUM_MEDIA = 10 if PROTOTYPE_MODE else 100

# Serial (ESP32-S3 USB-CDC)
SERIAL_PORT = "/dev/ttyACM0"
SERIAL_BAUD = 115200

# HTTP
HTTP_HOST = "0.0.0.0"  # Alle Interfaces
HTTP_PORT = 8080
```

---

## 5 Serial-Kommunikation (os.open statt pyserial)

### 5.1 Warum nicht pyserial?

Der ESP32-S3 XIAO nutzt **USB-CDC** (USB Communications Device Class). `pyserial` und `serial_asyncio` funktionieren damit nicht zuverlässig:

```
serial.serialutil.SerialException: device reports readiness to read 
but returned no data (device disconnected or multiple access on port?)
```

### 5.2 Lösung: os.open + stty

```python
import os
import select
import subprocess

# stty konfiguriert den Port
subprocess.run(['stty', '-F', '/dev/ttyACM0', '115200', 'raw', '-echo'])

# os.open für non-blocking I/O
fd_read = os.open('/dev/ttyACM0', os.O_RDONLY | os.O_NONBLOCK)
fd_write = os.open('/dev/ttyACM0', os.O_WRONLY)

# select.select für Event-basiertes Lesen
r, _, _ = select.select([fd_read], [], [], 1.0)
if r:
    data = os.read(fd_read, 1024)
```

### 5.3 Robuster Parser

ESP32 USB-CDC kann Nachrichten fragmentieren. Der Parser erkennt:

| Empfangen | Erkannt als |
|-----------|-------------|
| `PRESS 001` | Button 1 ✓ |
| ` 001` | Button 1 ✓ (fragmentiert) |
| `001` | Button 1 ✓ (fragmentiert) |
| `SE 001` | Release ✓ (fragmentiert) |

```python
def parse_button_id(s: str) -> int | None:
    s = s.strip()
    if not s:
        return None
    try:
        button_id = int(s)
        if 1 <= button_id <= NUM_MEDIA:
            return button_id
    except ValueError:
        pass
    return None
```

---

## 6 Autostart mit systemd

**systemd** ist der Service-Manager von Linux. Er startet den Server automatisch beim Hochfahren und startet ihn bei Absturz neu.

### 6.1 Service-Datei

Datei: `/etc/systemd/system/selection-panel.service`

```ini
[Unit]
Description=Interaktives Auswahlpanel
After=network.target

[Service]
Type=simple
User=pi
WorkingDirectory=/home/pi/selection-panel
ExecStart=/home/pi/selection-panel/venv/bin/python server.py
Restart=on-failure
RestartSec=5

[Install]
WantedBy=multi-user.target
```

### 6.2 Aktivieren

```bash
sudo systemctl daemon-reload
sudo systemctl enable --now selection-panel.service
```

### 6.3 Status prüfen

```bash
sudo systemctl status selection-panel.service
journalctl -u selection-panel.service -f  # Live-Logs
```

---

## 7 HTTP-Endpoints

| Pfad | Funktion |
|------|----------|
| `/` | Dashboard (index.html) |
| `/ws` | WebSocket-Verbindung |
| `/static/*` | CSS, JavaScript |
| `/media/*` | Bilder und Audio |
| `/test/play/{id}` | Wiedergabe simulieren (1-basiert) |
| `/test/stop` | Wiedergabe stoppen |
| `/status` | Server-Status (JSON) |
| `/health` | Health-Check |

### 7.1 Status-Endpoint

```bash
curl http://rover:8080/status | jq
```

```json
{
  "version": "2.4.1",
  "mode": "prototype",
  "num_media": 10,
  "current_button": 5,
  "ws_clients": 1,
  "serial_connected": true,
  "serial_port": "/dev/ttyACM0",
  "media_missing": 0
}
```

---

## 8 Serial-Protokoll (1-basiert!)

### 8.1 ESP32 → Server

| Empfangen | Aktion |
|-----------|--------|
| `PRESS 001` | Button 1 gedrückt → Play |
| `RELEASE 001` | Button 1 losgelassen (ignoriert) |
| `READY` | ESP32 bereit |
| `PONG` | Antwort auf PING |

### 8.2 Server → ESP32

| Gesendet | Bedeutung |
|----------|-----------|
| `LEDSET 001` | LED 1 ein (one-hot) |
| `LEDCLR` | Alle LEDs aus |
| `PING` | Verbindungstest |

### 8.3 Server → Browser (WebSocket)

| Nachricht | Bedeutung |
|-----------|-----------|
| `{"type": "stop"}` | Wiedergabe stoppen |
| `{"type": "play", "id": 1}` | Medien 001 abspielen |

### 8.4 Browser → Server (WebSocket)

| Nachricht | Bedeutung |
|-----------|-----------|
| `{"type": "ended", "id": 1}` | Audio 001 beendet |

---

## 9 Datenfluss bei Tastendruck

```
1. ESP32 sendet:     PRESS 005
2. Server sendet:    LEDSET 005          → LED 5 an
3. Server broadcast: {"type":"stop"}     → Browser stoppt
4. Server broadcast: {"type":"play","id":5} → Browser spielt 005.jpg/mp3
5. Browser meldet:   {"type":"ended","id":5}
6. Server sendet:    LEDCLR              → LED aus
```

**Race-Condition-Schutz:** Wenn zwei Tasten schnell hintereinander kommen, könnte ein `ended`-Event zur falschen ID gehören. Der Server prüft: Ist die ID noch aktuell? Nur dann werden die LEDs gelöscht.

---

## 10 Deployment (Mac → Pi)

```bash
# Dateien synchronisieren
rsync -avz --exclude='venv' --exclude='.git' --exclude='__pycache__' \
    ./selection-panel/ pi@rover:/home/pi/selection-panel/

# Server neu starten
ssh rover 'sudo systemctl restart selection-panel.service'
```

Oder mit Git:

```bash
# Auf dem Pi
cd ~/selection-panel
git pull
sudo systemctl restart selection-panel.service
```

---

## 11 Debugging

### 11.1 Serial direkt testen

```bash
# Port konfigurieren
stty -F /dev/ttyACM0 115200 raw -echo

# Daten empfangen (Ctrl+C zum Beenden)
cat /dev/ttyACM0

# Befehle senden
echo "PING" > /dev/ttyACM0
echo "LEDSET 001" > /dev/ttyACM0
echo "LEDCLR" > /dev/ttyACM0
```

### 11.2 Port blockiert?

```bash
# Wer nutzt den Port?
sudo fuser /dev/ttyACM0

# micro-ROS Agent stoppen (falls installiert)
sudo systemctl stop microros-agent.service
```

---

## 12 Features

- [x] Thread-basierter Serial-Reader (os.open + select)
- [x] Robuster Parser für fragmentierte Serial-Daten
- [x] WebSocket Broadcast an alle Clients
- [x] Preempt-Policy mit Race-Condition-Schutz
- [x] Medien-Validierung beim Start
- [x] Auto-Reconnect bei Serial-Abbruch
- [x] `PROTOTYPE_MODE` für Skalierung 10 ↔ 100
- [x] 1-basierte Nummerierung durchgängig

---

## 13 Bekannte Einschränkungen

| Problem | Status | Workaround |
|---------|--------|------------|
| pyserial funktioniert nicht mit ESP32-S3 USB-CDC | ✅ Behoben | os.open + stty |
| Serial-Daten fragmentiert | ✅ Behoben | Robuster Parser |
| BlockingIOError (EAGAIN) | ✅ Behoben | Wird ignoriert |

---

## 14 Changelog

| Version | Datum | Änderungen |
|---------|-------|------------|
| 2.4.1 | 2025-12-30 | Robuster Parser für fragmentierte Serial-Daten |
| 2.4.0 | 2025-12-30 | 1-basierte Medien (001-010 statt 000-009) |
| 2.3.2 | 2025-12-30 | os.open statt pyserial für USB-CDC |
| 2.3.1 | 2025-12-30 | Threading statt serial_asyncio |
| 2.2.5 | 2025-12-27 | Initiale Version |
