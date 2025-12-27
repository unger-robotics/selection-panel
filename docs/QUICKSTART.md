# Quickstart: Selection-Panel

> Server + Dashboard in 5 Minuten

## Referenz-System

| Hardware | Version |
|----------|---------|
| Board | Raspberry Pi 5 Model B Rev 1.1 |
| RAM | 8 GB |
| Speicher | 128 GB SD |

| Software | Version |
|----------|---------|
| Pi OS | Debian 13 (trixie) |
| Pi OS Build | 2025-12-04 |
| Kernel | 6.12.47+rpt-rpi-2712 |
| Python | 3.13.5 |

| Python-Paket | Version |
|--------------|---------|
| aiohttp | 3.13.2 |
| pyserial-asyncio | 0.6 |

## Voraussetzungen

- SSH-Zugang zum Pi (siehe [SSH.md](SSH.md))
- ESP32-S3 via USB angeschlossen (optional für ersten Test)

## Setup

```bash
# 1. Verbinden
ssh rover

# 2. Klonen
cd ~
git clone git@github.com:unger-robotics/selection-panel.git
cd selection-panel

# 3. Python-Umgebung
python3 -m venv venv
venv/bin/pip install -r requirements.txt

# 4. Starten
venv/bin/python server.py
```

## Dashboard öffnen

```
http://rover:8080/
```

## Testen (ohne Hardware)

```bash
curl http://rover:8080/test/play/5   # Wiedergabe
curl http://rover:8080/status        # Status
curl http://rover:8080/test/stop     # Stopp
```

## Autostart einrichten

```bash
sudo cp selection-panel.service /etc/systemd/system/
sudo systemctl daemon-reload
sudo systemctl enable --now selection-panel.service
```

## Troubleshooting

| Problem | Lösung |
|---------|--------|
| `ModuleNotFoundError` | `venv/bin/pip install -r requirements.txt` |
| `Permission denied: /dev/ttyACM0` | `sudo usermod -aG dialout $USER` → Neu einloggen |
| Kein Ton | "Sound aktivieren"-Button im Browser klicken |
