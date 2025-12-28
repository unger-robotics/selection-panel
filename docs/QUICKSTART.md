# Quickstart: Selection-Panel

Server + Dashboard in 5 Minuten.

---

## Voraussetzungen

- [ ] SSH eingerichtet → [SSH.md](SSH.md)
- [ ] Repository geklont → [GIT.md](GIT.md)

---

## Setup (einmalig)

```bash
ssh rover
cd ~/selection-panel

python3 -m venv venv
venv/bin/pip install -r requirements.txt
```

---

## Server starten

```bash
venv/bin/python server.py
```

**Dashboard:** `http://rover:8080/`

---

## Testen (ohne Hardware)

```bash
curl http://rover:8080/test/play/5   # Wiedergabe
curl http://rover:8080/status        # Status
curl http://rover:8080/test/stop     # Stopp
```

---

## Autostart einrichten

```bash
sudo cp selection-panel.service /etc/systemd/system/
sudo systemctl daemon-reload
sudo systemctl enable --now selection-panel.service
```

**Status prüfen:** `sudo systemctl status selection-panel`

---

## Troubleshooting

| Problem | Lösung |
|:--------|:-------|
| `ModuleNotFoundError` | `venv/bin/pip install -r requirements.txt` |
| `Permission denied: /dev/ttyACM0` | `sudo usermod -aG dialout $USER` → Neu einloggen |
| Kein Ton | "Sound aktivieren"-Button im Browser klicken |
| Server startet nicht | `journalctl -u selection-panel -f` |

---

## Referenz-System

| Komponente | Version |
|:-----------|:--------|
| Raspberry Pi 5 | 4 GB RAM, 128 GB SD |
| Pi OS | Debian 12 (bookworm) |
| Python | 3.12+ |
| aiohttp | 3.11+ |
| pyserial-asyncio | 0.6 |

---

*Stand: Dezember 2025*
