# Quickstart: Selection-Panel

Server + Dashboard in 5 Minuten.

| Stand | 2025-12-30 |
|-------|------------|
| Status | ✅ Prototyp funktionsfähig |

---

## Voraussetzungen

- [ ] SSH eingerichtet → [SSH.md](SSH.md)
- [ ] Repository geklont → [GIT.md](GIT.md)
- [ ] ESP32 geflasht → [FIRMWARE.md](FIRMWARE.md)

---

## Setup (einmalig)

```bash
ssh rover
cd ~/selection-panel

python3 -m venv venv
venv/bin/pip install -r requirements.txt
```

**Hinweis:** Nur `aiohttp` wird benötigt (kein pyserial mehr).

---

## Server starten

```bash
cd ~/selection-panel
source venv/bin/activate
python server.py
```

Erwartete Ausgabe:
```
==================================================
Auswahlpanel Server v2.4.1 (PROTOTYPE)
==================================================
Medien: 10 erwartet (IDs: 001-010)
Taster: 1-10 (1-basiert)
Serial: /dev/ttyACM0
HTTP:   http://0.0.0.0:8080/
==================================================
Medien-Validierung: 10/10 vollstaendig
==================================================
Serial verbinde: /dev/ttyACM0
Serial verbunden
```

**Dashboard:** `http://rover:8080/`

---

## Dashboard nutzen

1. Dashboard öffnen: `http://rover:8080/`
2. **"Sound aktivieren"** Button klicken (wichtig!)
3. Taster drücken → Bild + Ton werden abgespielt
4. LED leuchtet während Wiedergabe

---

## Testen (ohne Hardware)

```bash
# Wiedergabe simulieren (1-basiert!)
curl http://rover:8080/test/play/1
curl http://rover:8080/test/play/5
curl http://rover:8080/test/play/10

# Status abfragen
curl http://rover:8080/status | jq

# Wiedergabe stoppen
curl http://rover:8080/test/stop
```

---

## Serial direkt testen

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

---

## Autostart einrichten

```bash
sudo cp selection-panel.service /etc/systemd/system/
sudo systemctl daemon-reload
sudo systemctl enable --now selection-panel.service
```

**Status prüfen:** `sudo systemctl status selection-panel`

**Logs:** `journalctl -u selection-panel -f`

---

## Troubleshooting

| Problem | Lösung |
|:--------|:-------|
| `ModuleNotFoundError` | `venv/bin/pip install aiohttp` |
| `Permission denied: /dev/ttyACM0` | `sudo usermod -aG dialout $USER` → Neu einloggen |
| Port blockiert | `sudo fuser /dev/ttyACM0` → Prozess beenden |
| Kein Ton | "Sound aktivieren"-Button im Browser klicken |
| Server startet nicht | `journalctl -u selection-panel -f` |
| Taster nicht erkannt | Serial testen: `cat /dev/ttyACM0` |
| Falsche Medien | Prüfen: `ls media/` (001.jpg bis 010.jpg) |

---

## Medien-Struktur (1-basiert!)

```
media/
├── 001.jpg  001.mp3
├── 002.jpg  002.mp3
├── ...
└── 010.jpg  010.mp3
```

**Test-Medien generieren:**
```bash
./scripts/generate_test_media.sh 10
```

---

## Referenz-System

| Komponente | Version |
|:-----------|:--------|
| Raspberry Pi 5 | 4 GB RAM |
| Pi OS | Debian 12 (bookworm) |
| Python | 3.13+ |
| aiohttp | 3.11+ |
| ESP32 Firmware | 2.4.0 |
| Server | 2.4.1 |
| Dashboard | 2.2.4 |

---

## Schnellreferenz

| Aktion | Befehl |
|--------|--------|
| Server starten | `python server.py` |
| Dashboard | `http://rover:8080/` |
| Status | `curl http://rover:8080/status` |
| Test Play | `curl http://rover:8080/test/play/5` |
| Serial Monitor | `cat /dev/ttyACM0` |
| Service Status | `sudo systemctl status selection-panel` |
| Service Logs | `journalctl -u selection-panel -f` |

---

*Stand: Dezember 2025*
