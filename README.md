# Interaktives Auswahlpanel

**100 Taster × 100 LEDs × 100 Bild/Audio-Sets**

Tastendruck → Bild + Audio. Der letzte Tastendruck gewinnt.

| Version | Status | Datum |
|:--------|:-------|:------|
| 2.2.5 | Phase 6 (Integration) | 2025-12-27 |

---

## Quick Start

```bash
ssh rover
cd ~/selection-panel
venv/bin/python server.py
```

**Dashboard:** `http://rover:8080/`

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
|:---------|:-------|
| [VORAUSSETZUNGEN.md](docs/VORAUSSETZUNGEN.md) | Hardware, Software-Installation |
| [SSH.md](docs/SSH.md) | Pi einrichten, SSH-Zugang |
| [GIT.md](docs/GIT.md) | Repository, Commits, Deployment |
| [QUICKSTART.md](docs/QUICKSTART.md) | Server + Dashboard starten |
| [LOETEN.md](docs/LOETEN.md) | Bleifreies Löten (SAC305) |

### Referenz

| Dokument | Inhalt |
|:---------|:-------|
| [SPEC.md](docs/SPEC.md) | **Single Source of Truth** — Protokoll, Pinbelegung |
| [HARDWARE.md](docs/HARDWARE.md) | Stückliste, IC-Pinbelegungen |
| [FIRMWARE.md](docs/FIRMWARE.md) | ESP32 Architektur, Build |
| [SERVER.md](docs/SERVER.md) | Raspberry Pi Server |
| [DASHBOARD.md](docs/DASHBOARD.md) | Web-Frontend |

### Betrieb

| Dokument | Inhalt |
|:---------|:-------|
| [RUNBOOK.md](docs/RUNBOOK.md) | Troubleshooting |
| [CHANGELOG.md](docs/CHANGELOG.md) | Versionshistorie |

---

## Verzeichnisstruktur

```
selection-panel/
├── button_panel_firmware/    # ESP32 PlatformIO-Projekt
├── docs/                     # Dokumentation
├── media/                    # Bild/Audio (000–099)
├── static/                   # Web-Dashboard
├── server.py                 # Python Server
├── requirements.txt          # Python-Abhängigkeiten
└── selection-panel.service   # systemd-Service
```

---

## Referenz-System

| Komponente | Version |
|:-----------|:--------|
| Raspberry Pi 5 | 4 GB RAM |
| Seeed XIAO ESP32-S3 | |
| Pi OS | Debian 12 (bookworm) |
| Python | 3.12+ |

---

## Policy

| Regel | Bedeutung |
|:------|:----------|
| **One-hot** | Maximal eine LED leuchtet |
| **Preempt** | Neuer Tastendruck unterbricht sofort |

---

## Skalierung

| Datei | Prototyp | Produktion |
|:------|:---------|:-----------|
| `config.h` | `#define PROTOTYPE_MODE` | auskommentieren |
| `server.py` | `PROTOTYPE_MODE = True` | `False` |

---

## Lizenz

MIT License — Jan Unger, 2025
