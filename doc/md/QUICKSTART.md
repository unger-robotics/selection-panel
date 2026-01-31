# Quickstart: Selection-Panel

Server + Dashboard in 5 Minuten.

| Stand | 2026-01-31 |
|-------|------------|
| Version | 2.5.3 |
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
venv/bin/pip install -r server/requirements.txt
```

**Hinweis:** Nur `aiohttp` wird benötigt (kein pyserial mehr).

---

## Server starten

```bash
cd ~/selection-panel/server
source ../venv/bin/activate
python server.py
```

Erwartete Ausgabe:
```
==================================================
Auswahlpanel Server v2.5.3 (PROTOTYPE)
==================================================
Medien: 10 erwartet (IDs: 001-010)
Taster: 1-10 (1-basiert)
Serial: /dev/serial/by-id/usb-Espressif...
HTTP:   http://0.0.0.0:8080/
ESP32 lokale LED: aktiviert
==================================================
Medien-Validierung: 10/10 vollständig
==================================================
Serial verbinde: /dev/serial/by-id/usb-Espressif...
Serial verbunden
```

**Dashboard:** `http://rover:8080/`

---

## Dashboard nutzen

1. Dashboard öffnen: `http://rover:8080/`
2. **"Sound aktivieren"** Button klicken (wichtig!)
3. Warten: "Lade Medien... 5/10" → "Warte auf Tastendruck..."
4. Taster drücken → Bild + Ton werden sofort abgespielt
5. LED leuchtet sofort (< 1ms), Wiedergabe aus Cache (< 50ms)

---

## Testen (ohne Hardware)

```bash
# Wiedergabe simulieren (1-basiert!)
curl http://rover:8080/test/play/1
curl http://rover:8080/test/play/5
curl http://rover:8080/test/play/10

# Status abfragen
curl http://rover:8080/status | jq

# Health-Check
curl http://rover:8080/health | jq

# Wiedergabe stoppen
curl http://rover:8080/test/stop
```

---

## Serial direkt testen

```bash
# Stabilen Port ermitteln
SERIAL_PORT=$(ls /dev/serial/by-id/usb-Espressif* 2>/dev/null | head -1)
echo "Port: $SERIAL_PORT"

# Port konfigurieren
stty -F $SERIAL_PORT 115200 raw -echo

# Daten empfangen (Ctrl+C zum Beenden)
cat $SERIAL_PORT

# Befehle senden
echo "PING" > $SERIAL_PORT
echo "STATUS" > $SERIAL_PORT
echo "LEDSET 001" > $SERIAL_PORT
echo "LEDCLR" > $SERIAL_PORT
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
| Taster nicht erkannt | Serial testen: `cat /dev/serial/by-id/usb-Espressif*` |
| Falsche Medien | Prüfen: `ls server/media/` (001.jpg bis 010.jpg) |
| Preload dauert lange | Medien komprimieren oder Concurrency erhöhen |

---

## Medien-Struktur (1-basiert!)

```
server/media/
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

## Latenz-Budget

| Komponente | Latenz |
|------------|--------|
| ESP32 LED | < 1ms |
| Serial + Server | ~10ms |
| Dashboard (aus Cache) | < 50ms |
| **Gesamt** | **< 70ms** |

---

## Referenz-System

| Komponente | Version |
|:-----------|:--------|
| Raspberry Pi 5 | 4 GB RAM |
| Pi OS | Debian 13 (trixie) |
| Python | 3.13+ |
| aiohttp | 3.9+ |
| ESP32 Firmware | 2.5.3 |
| Server | 2.5.3 |
| Dashboard | 2.5.3 |

---

## Schnellreferenz

| Aktion | Befehl |
|--------|--------|
| Server starten | `cd server && python server.py` |
| Dashboard | `http://rover:8080/` |
| Status | `curl http://rover:8080/status` |
| Health | `curl http://rover:8080/health` |
| Test Play | `curl http://rover:8080/test/play/5` |
| Serial Monitor | `cat /dev/serial/by-id/usb-Espressif*` |
| Service Status | `sudo systemctl status selection-panel` |
| Service Logs | `journalctl -u selection-panel -f` |

---

*Stand: 2026-01-31 | Version 2.5.3*
