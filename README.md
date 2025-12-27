# Interaktives Auswahlpanel

> **100 Taster × 100 LEDs × 100 Bild/Audio-Sets**

Ein interaktives Panel: Tastendruck → Bild + Audio. Der letzte Tastendruck gewinnt.

| Version | Status | Datum |
|---------|--------|-------|
| 2.2.5 | Phase 6 (Integration) | 2025-12-27 |

---

## Quick Start

```bash
# Dashboard öffnen
open http://rover:8080/

# Wiedergabe testen
curl http://rover:8080/test/play/5

# Status
curl http://rover:8080/status
```

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

| Dokument | Inhalt |
|----------|--------|
| [SPEC.md](SPEC.md) | **Single Source of Truth** — Protokoll, Pinbelegung, Policy |
| [HARDWARE.md](HARDWARE.md) | Stückliste, IC-Pinbelegungen |
| [FIRMWARE.md](FIRMWARE.md) | ESP32 Architektur, Build |
| [SERVER.md](SERVER.md) | Raspberry Pi Setup |
| [DASHBOARD.md](DASHBOARD.md) | Web-Frontend |
| [COMMANDS.md](COMMANDS.md) | Deployment, Befehle |
| [RUNBOOK.md](RUNBOOK.md) | Betrieb, Troubleshooting |
| [ROADMAP.md](ROADMAP.md) | Phasen, Status |
| [CHANGELOG.md](CHANGELOG.md) | Versionshistorie |

---

## Verzeichnisstruktur

```
selection-panel/
├── button_panel_firmware/    # ESP32 PlatformIO-Projekt
├── static/                   # Web-Dashboard
├── media/                    # Bild/Audio (000–099)
├── server.py                 # Python Server
└── docs/                     # Dokumentation
```

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
