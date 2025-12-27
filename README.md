# Interaktives Auswahlpanel

> **100 Taster × 100 LEDs × 100 Bild/Audio-Sets**

Ein interaktives Panel: Tastendruck → Bild + Audio. Der letzte Tastendruck gewinnt.

| Version | Status | Datum |
|---------|--------|-------|
| 2.2.5 | Phase 6 (Integration) | 2025-12-27 |

---

## Quick Start

```bash
# Verbinden
ssh rover

# Setup (einmalig)
cd ~/selection-panel
python3 -m venv venv
venv/bin/pip install -r requirements.txt

# Starten
venv/bin/python server.py
```

```bash
# Dashboard öffnen
open http://rover:8080/

# Wiedergabe testen
curl http://rover:8080/test/play/5
```

→ Details: [docs/QUICKSTART.md](docs/QUICKSTART.md)

---

## Architektur

```
┌─────────────┐     Serial      ┌─────────────┐    WebSocket    ┌─────────────┐
│  ESP32-S3   │ ──────────────► │ Raspberry   │ ◄─────────────► │   Browser   │
│    XIAO     │    115200 Bd    │    Pi 5     │     :8080       │  Dashboard  │
│             │                 │             │                 │             │
│ 100 Buttons │                 │  Gateway    │                 │ Fullscreen  │
│ 100 LEDs    │                 │  + Server   │                 │ Bild+Audio  │
└─────────────┘                 └─────────────┘                 └─────────────┘
```

---

## Dokumentation

### Setup

| Dokument | Inhalt |
|----------|--------|
| [QUICKSTART.md](docs/QUICKSTART.md) | Server + Dashboard starten |
| [SSH.md](docs/SSH.md) | SSH-Zugang Mac/Windows/Pi |
| [GIT.md](docs/GIT.md) | Git-Workflow, Commits |

### Referenz

| Dokument | Inhalt |
|----------|--------|
| [SPEC.md](docs/SPEC.md) | **Single Source of Truth** — Protokoll, Pinbelegung, Policy |
| [HARDWARE.md](docs/HARDWARE.md) | Stückliste, IC-Pinbelegungen |
| [FIRMWARE.md](docs/FIRMWARE.md) | ESP32 Architektur, Build |
| [SERVER.md](docs/SERVER.md) | Raspberry Pi Setup |
| [DASHBOARD.md](docs/DASHBOARD.md) | Web-Frontend |

### Betrieb

| Dokument | Inhalt |
|----------|--------|
| [COMMANDS.md](docs/COMMANDS.md) | Deployment, Befehle |
| [RUNBOOK.md](docs/RUNBOOK.md) | Betrieb, Troubleshooting |
| [ROADMAP.md](docs/ROADMAP.md) | Phasen, Status |
| [CHANGELOG.md](docs/CHANGELOG.md) | Versionshistorie |

---

## Verzeichnisstruktur

```
selection-panel/
├── button_panel_firmware/    # ESP32 PlatformIO-Projekt
├── docs/                     # Dokumentation
├── media/                    # Bild/Audio (000–099)
├── scripts/                  # Hilfsskripte
├── static/                   # Web-Dashboard
├── server.py                 # Python Server
├── requirements.txt          # Python-Abhängigkeiten
└── selection-panel.service   # systemd-Service
```

---

## Referenz-System

| Hardware | Version |
|----------|---------|
| Board | Raspberry Pi 5 Model B Rev 1.1 |
| Microcontroller | Seeed XIAO ESP32-S3 |

| Software | Version |
|----------|---------|
| Pi OS | Debian 13 (trixie), Build 2025-12-04 |
| Python | 3.13.5 |
| aiohttp | 3.13.2 |

---

## Policy

**One-hot:** Maximal eine LED leuchtet.

**Preempt:** Neuer Tastendruck unterbricht sofort.

---

## Skalierung

`PROTOTYPE_MODE` schaltet zwischen 10 und 100 Kanälen:

| Datei | Prototyp | Produktion |
|-------|----------|------------|
| config.h | `#define PROTOTYPE_MODE` | auskommentieren |
| server.py | `PROTOTYPE_MODE = True` | `False` |

---

## Lizenz

MIT License — Jan Unger, 2025
