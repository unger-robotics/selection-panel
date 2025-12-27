# COMMANDS — Entwickler-Referenz

> Deployment, Build und Test-Befehle. Protokoll: siehe [SPEC.md](SPEC.md)

| Version | 2.2.5 |
|---------|-------|

---

## 1 Deployment (Mac → Pi)

**rsync** synchronisiert Dateien zwischen Rechnern – nur geänderte Dateien werden übertragen.

```bash
cd ./selection-panel

rsync -avz --delete \
    --exclude='button_panel_firmware' \
    --exclude='venv' \
    --exclude='.git' \
    --exclude='__pycache__' \
    . pi@rover:/home/pi/selection-panel/
```

| Flag | Bedeutung |
|------|-----------|
| `-a` | Archiv-Modus (Rechte, Zeiten erhalten) |
| `-v` | Verbose (zeigt Dateien) |
| `-z` | Komprimiert Übertragung |
| `--delete` | Löscht Dateien auf Ziel, die lokal nicht existieren |

---

## 2 Server-Steuerung

**systemd** ist der Service-Manager von Linux. Er startet Dienste automatisch und überwacht sie.

```bash
# Starten + Logs verfolgen
ssh pi@rover 'sudo systemctl restart selection-panel && journalctl -u selection-panel -f'

# Nur starten/stoppen
ssh pi@rover 'sudo systemctl start selection-panel'
ssh pi@rover 'sudo systemctl stop selection-panel'

# Status prüfen
ssh pi@rover 'sudo systemctl status selection-panel'
```

**journalctl** zeigt systemd-Logs:

```bash
# Live-Logs
journalctl -u selection-panel -f

# Letzte 50 Zeilen
journalctl -u selection-panel -n 50

# Letzte Stunde
journalctl -u selection-panel --since "1 hour ago"
```

---

## 3 Dashboard testen

```bash
# Browser öffnen (Mac)
open http://rover:8080/

# Status (JSON)
curl http://rover:8080/status

# Health-Check
curl http://rover:8080/health

# Wiedergabe simulieren
curl http://rover:8080/test/play/5
curl http://rover:8080/test/stop
```

---

## 4 Firmware flashen

**PlatformIO** ist ein Build-System für Embedded-Entwicklung.

```bash
cd button_panel_firmware

pio run                      # Kompilieren
pio run -t upload            # Flashen
pio device monitor           # Serial-Monitor (115200 Baud)
pio run -t upload -t monitor # Flash + Monitor
```

---

## 5 Serial-Debugging

**screen** ist ein Terminal-Multiplexer – verbindet sich mit seriellen Geräten.

```bash
# Server stoppen (gibt Port frei)
ssh pi@rover 'sudo systemctl stop selection-panel'

# Serial-Port finden
ssh pi@rover 'ls -l /dev/serial/by-id/'

# Verbinden
ssh pi@rover
screen /dev/serial/by-id/usb-Espressif_... 115200
```

**Screen beenden:** `Ctrl+A`, dann `K`, dann `Y`

Befehle: siehe [SPEC.md §6](SPEC.md#6-serial-protokoll-esp32--pi)

---

## 6 Medien verwalten

```bash
# Test-Medien generieren
./generate_test_media.sh      # 10 Stück (Prototyp)
./generate_test_media.sh 100  # 100 Stück (Produktion)

# Medien auf Pi prüfen
ssh pi@rover 'ls ~/selection-panel/media/*.jpg | wc -l'
ssh pi@rover 'ls ~/selection-panel/media/*.mp3 | wc -l'
```

---

## 7 Quick Reference

| Aktion | Befehl |
|--------|--------|
| Deploy | `rsync -avz --delete --exclude=... . pi@rover:...` |
| Server starten | `ssh pi@rover 'sudo systemctl restart selection-panel'` |
| Dashboard | `open http://rover:8080/` |
| Test-Play | `curl http://rover:8080/test/play/5` |
| Firmware | `cd button_panel_firmware && pio run -t upload` |
| Serial-Test | `screen /dev/serial/by-id/usb-Espressif* 115200` |
