# Interaktives Auswahlpanel

**100 Taster × 100 LEDs × 100 Bild/Audio-Sets**

Tastendruck → Bild + Audio. Der letzte Tastendruck gewinnt.

| Version | Status | Datum |
|:--------|:-------|:------|
| 2.4.1 | ✅ Prototyp funktionsfähig | 2025-12-30 |

---

## Quick Start

```bash
ssh rover
cd ~/selection-panel
source venv/bin/activate
python server.py
```

**Dashboard:** `http://rover:8080/`

→ Details: [docs/QUICKSTART.md](docs/QUICKSTART.md)

---

## Architektur

```
┌─────────────┐     Serial      ┌─────────────┐    WebSocket    ┌─────────────┐
│  ESP32-S3   │ ──────────────► │ Raspberry   │ ◄─────────────► │   Browser   │
│    XIAO     │    115200 Bd    │    Pi 5     │     :8080       │  Dashboard  │
│             │  PRESS 001-100  │             │                 │             │
│ 100 Buttons │  LEDSET 001-100 │  Gateway    │                 │ Fullscreen  │
│ 100 LEDs    │                 │  + Server   │                 │ Bild+Audio  │
└─────────────┘                 └─────────────┘                 └─────────────┘
     v2.4.0                          v2.4.1                        v2.2.4
```

---

## Aktueller Stand (Prototyp)

| Komponente | Status | Details |
|:-----------|:-------|:--------|
| Hardware | ✅ | 10 Taster, 10 LEDs, Breadboard |
| Firmware | ✅ v2.4.0 | Serial.flush() für USB-CDC |
| Server | ✅ v2.4.1 | os.open statt pyserial |
| Dashboard | ✅ v2.2.4 | 1-basierte Medien (001-010) |
| Zuordnung | ✅ | Taster 1 → Bild/Ton 001 |

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
│   └── src/main.cpp          # Firmware v2.4.0
├── docs/                     # Dokumentation
├── media/                    # Bild/Audio (001–100, 1-basiert!)
├── static/                   # Web-Dashboard
│   ├── index.html
│   ├── styles.css
│   └── app.js
├── scripts/                  # Hilfsskripte
│   └── generate_test_media.sh
├── server.py                 # Python Server v2.4.1
├── requirements.txt          # Python-Abhängigkeiten
└── selection-panel.service   # systemd-Service
```

---

## Nummerierung (1-basiert)

| Schicht | Format | Beispiel |
|:--------|:-------|:---------|
| Taster (physisch) | 1-100 | Taster 1, Taster 10 |
| Serial-Protokoll | 001-100 | `PRESS 001`, `LEDSET 010` |
| Medien-Dateien | 001-100 | `001.jpg`, `010.mp3` |
| Dashboard-Anzeige | 001-100 | "001", "010" |

---

## Referenz-System

| Komponente | Version |
|:-----------|:--------|
| Raspberry Pi 5 | 4 GB RAM |
| Seeed XIAO ESP32-S3 | |
| Pi OS | Debian 12 (bookworm) |
| Python | 3.13+ |

---

## Bekannte Einschränkungen

| Problem | Workaround |
|:--------|:-----------|
| ESP32-S3 USB-CDC fragmentiert | Firmware: `Serial.flush()` nach jedem Event |
| pyserial funktioniert nicht | Server: `os.open` + `stty` statt pyserial |

---

## Policy

| Regel | Bedeutung |
|:------|:----------|
| **One-hot** | Maximal eine LED leuchtet |
| **Preempt** | Neuer Tastendruck unterbricht sofort |

---

## Skalierung

| Datei | Prototyp (10) | Produktion (100) |
|:------|:--------------|:-----------------|
| `config.h` | `#define PROTOTYPE_MODE` | auskommentieren |
| `server.py` | `PROTOTYPE_MODE = True` | `False` |
| Hardware | 2× Shift Register | 26× Shift Register |

---

## Nächste Schritte

- [ ] Systemd-Service für Autostart
- [ ] Echte Medien hinzufügen
- [ ] PCB-Design für 100 Buttons
- [ ] Gehäuse

---

## Lizenz

MIT License — Jan Unger, 2025
