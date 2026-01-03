# Voraussetzungen

Hardware und Software für das Selection-Panel-Projekt.

---

## 1. Hardware

| Komponente | Zweck | Bezugsquelle | Preis |
|:-----------|:------|:-------------|------:|
| Raspberry Pi 5, 4GB RAM | Server, Media-Ausgabe | BerryBase (RPI5-4GB) | 71,50 € |
| Raspberry Pi Active Cooler | Kühlung | BerryBase (RPI5-ACOOL) | 5,90 € |
| Raspberry Pi 27W USB-C Netzteil | Stromversorgung | BerryBase (RPI5NT5AW) | 12,40 € |
| SanDisk Extreme microSDXC 128GB | Betriebssystem | BerryBase (A2 UHS-I U3 V30) | ~18 € |
| HDMI Adapter Micro-D → A | Monitor | BerryBase (8007067) | 1,10 € |
| Seeed XIAO ESP32-S3 | Taster/LEDs | Reichelt | ~9 € |

> **Bezugsquelle:** [BerryBase.de](https://www.berrybase.de) – Preise Stand Januar 2026

---

## 2. Software (Entwicklungsrechner)

### macOS

```bash
/bin/bash -c "$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/HEAD/install.sh)"
brew install python git vim
```

### Windows

1. **Git:** [git-scm.com/download/win](https://git-scm.com/download/win)
2. **Python:** [python.org/downloads](https://www.python.org/downloads/) – "Add Python to PATH" ✓

### Linux (Ubuntu/Debian)

```bash
sudo apt update && sudo apt install python3 python3-pip python3-venv git vim
```

---

## 3. VS Code + PlatformIO

1. **VS Code:** [code.visualstudio.com](https://code.visualstudio.com/)
2. **Extension:** `PlatformIO IDE` installieren

### ESP32 flashen

| Aktion | Befehl |
|:-------|:-------|
| Kompilieren | `pio run` |
| Flashen | `pio run -t upload` |
| Serial-Monitor | `pio device monitor` |
| Flash + Monitor | `pio run -t upload -t monitor` |

---

## 4. Referenz-System

| Hardware | Version |
|----------|---------|
| Board | Raspberry Pi 5 Model B Rev 1.1 |
| Microcontroller | Seeed XIAO ESP32-S3 |

| Software | Version |
|----------|---------|
| Pi OS | Debian 13 (trixie), Build 2025-12-04 |
| Python | 3.13+ |
| aiohttp | 3.9+ |
| PlatformIO | 6.x |

---

## 5. Checkliste

### Hardware

- [ ] Raspberry Pi 5 + Active Cooler
- [ ] microSD-Karte (128 GB)
- [ ] ESP32-S3 XIAO
- [ ] USB-C Kabel (Daten, nicht nur Laden)
- [ ] Multimeter

### Software

- [ ] Git: `git --version`
- [ ] Python: `python3 --version`
- [ ] VS Code + PlatformIO
- [ ] SSH-Zugang zum Pi

---

## Nächste Schritte

| Schritt | Dokument |
|:--------|:---------|
| Pi einrichten + SSH | → [SSH.md](SSH.md) |
| Repository klonen | → [GIT.md](GIT.md) |
| Server starten | → [QUICKSTART.md](QUICKSTART.md) |
| Löten | → [LOETEN.md](LOETEN.md) |

---

*Stand: Januar 2026*
