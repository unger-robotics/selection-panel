# SERVER — Raspberry Pi Gateway

> Python-Server als Brücke zwischen ESP32 und Browser.

| Metadaten | Wert |
|-----------|------|
| Version | 2.2.5 |
| Plattform | Raspberry Pi 5, Pi OS Lite |
| Python | 3.11+ |

---

## 1 Überblick

Der Server empfängt Tastendruck-Events vom ESP32 per USB-Serial und verteilt sie per **WebSocket** an alle Browser. WebSocket ist eine bidirektionale Verbindung – der Server kann jederzeit Nachrichten senden, ohne dass der Client fragt.

```
ESP32 ──USB-Serial──► Server ──WebSocket──► Browser(s)
         115200 Baud           :8080
```

**Technologie:** Der Server nutzt **aiohttp** – eine Python-Bibliothek für asynchrone HTTP/WebSocket-Server. Asynchron bedeutet: Der Server wartet nicht blockierend auf Antworten, sondern bearbeitet mehrere Anfragen parallel.

---

## 2 Verzeichnisstruktur

```
/home/pi/selection-panel/
├── venv/                    # Isolierte Python-Umgebung
├── server.py                # Hauptserver
├── selection-panel.service  # Autostart-Konfiguration
├── static/                  # Dashboard-Dateien
│   ├── index.html
│   ├── styles.css
│   └── app.js
└── media/                   # Bild/Audio-Dateien
    ├── 000.jpg  000.mp3
    └── ...
```

**venv:** Eine isolierte Python-Umgebung. Pakete werden nur hier installiert, nicht systemweit – verhindert Versionskonflikte.

---

## 3 Installation

### 3.1 Python-Umgebung

```bash
# Umgebung erstellen
python3 -m venv ~/selection-panel/venv

# Pakete installieren
~/selection-panel/venv/bin/pip install aiohttp pyserial-asyncio
```

**pyserial-asyncio:** Asynchroner Serial-Treiber – liest vom ESP32, ohne den Server zu blockieren.

### 3.2 Serial-Port ermitteln

```bash
ls -l /dev/serial/by-id/
```

Den Pfad in `server.py` eintragen:
```python
SERIAL_PORT = "/dev/serial/by-id/usb-Espressif_..."
```

---

## 4 Konfiguration (server.py)

```python
# Skalierung: True = 10 Medien, False = 100 Medien
PROTOTYPE_MODE = True
NUM_MEDIA = 10 if PROTOTYPE_MODE else 100

# Serial
SERIAL_PORT = "/dev/serial/by-id/usb-Espressif_..."
SERIAL_BAUD = 115200

# HTTP
HTTP_HOST = "0.0.0.0"  # Alle Interfaces
HTTP_PORT = 8080
```

---

## 5 Autostart mit systemd

**systemd** ist der Service-Manager von Linux. Er startet den Server automatisch beim Hochfahren und startet ihn bei Absturz neu.

### 5.1 Service-Datei

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

### 5.2 Aktivieren

```bash
sudo systemctl daemon-reload
sudo systemctl enable --now selection-panel.service
```

### 5.3 Status prüfen

```bash
sudo systemctl status selection-panel.service
journalctl -u selection-panel.service -f  # Live-Logs
```

---

## 6 HTTP-Endpoints

| Pfad | Funktion |
|------|----------|
| `/` | Dashboard (index.html) |
| `/ws` | WebSocket-Verbindung |
| `/media/*` | Bilder und Audio |
| `/test/play/{id}` | Wiedergabe simulieren |
| `/status` | Server-Status (JSON) |
| `/health` | Health-Check |

---

## 7 Datenfluss bei Tastendruck

1. ESP32 sendet `PRESS 042`
2. Server sendet `LEDSET 042` zurück
3. Server broadcastet `{"type":"stop"}` an alle Browser
4. Server broadcastet `{"type":"play","id":42}`
5. Browser meldet `{"type":"ended","id":42}` nach Audio-Ende
6. Server sendet `LEDCLR`

**Race-Condition-Schutz:** Wenn zwei Tasten schnell hintereinander kommen, könnte ein `ended`-Event zur falschen ID gehören. Der Server prüft: Ist die ID noch aktuell? Nur dann werden die LEDs gelöscht.

---

## 8 Deployment (Mac → Pi)

```bash
rsync -avz --exclude='venv' --exclude='.git' \
    ./selection-panel/ pi@rover:/home/pi/selection-panel/

ssh pi@rover 'sudo systemctl restart selection-panel.service'
```

---

## 9 Features

- [x] Asynchroner Serial-Reader
- [x] WebSocket Broadcast an alle Clients
- [x] Preempt-Policy mit Race-Condition-Schutz
- [x] Medien-Validierung beim Start
- [x] Auto-Reconnect bei Serial-Abbruch
- [x] `PROTOTYPE_MODE` für Skalierung 10 ↔ 100
