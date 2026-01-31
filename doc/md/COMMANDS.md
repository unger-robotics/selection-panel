# COMMANDS – Entwickler-Referenz

> Deployment, Build, Test und Diagnose-Befehle.

| Version | 2.5.3 |
|---------|-------|
| Stand | 2026-01-31 |

---

## 1 Schnellreferenz

| Aktion | Befehl |
|--------|--------|
| Server starten | `cd server && python server.py` |
| Dashboard | `http://rover:8080/` |
| Status | `curl http://rover:8080/status \| jq` |
| Health | `curl http://rover:8080/health` |
| Test-Play | `curl http://rover:8080/test/play/5` |
| Serial-Monitor | `cat /dev/serial/by-id/usb-Espressif*` |
| Firmware flashen | `pio run -t upload` |
| Service Status | `sudo systemctl status selection-panel` |
| Service Logs | `journalctl -u selection-panel -f` |

---

## 2 Server starten

> **URL-Hinweis:** `rover` funktioniert auf allen Geräten (Mac, iPhone, iPad).
> Alternative: IP-Adresse direkt verwenden (`http://192.168.1.24:8080/`)

```bash
# Auf dem Pi
ssh rover
cd ~/selection-panel/server
source ../venv/bin/activate
python server.py
```

**Erwartete Ausgabe:**

```
==================================================
Auswahlpanel Server v2.5.3 (PROTOTYPE)
==================================================
Medien: 10 erwartet (IDs: 001-010)
Serial: /dev/serial/by-id/usb-Espressif_USB_JTAG_serial_debug_unit_...
HTTP:   http://0.0.0.0:8080/
ESP32 lokale LED: aktiviert
==================================================
Serial verbunden
```

---

## 3 Serial direkt testen

### 3.1 Serial-Port finden (by-id empfohlen)

```bash
# Stabiler Pfad (empfohlen)
ls -la /dev/serial/by-id/usb-Espressif*

# Fallback
ls -la /dev/ttyACM*
```

### 3.2 Empfangen (ohne Server)

```bash
# Port konfigurieren (by-id Pfad verwenden)
SERIAL_PORT=$(ls /dev/serial/by-id/usb-Espressif* 2>/dev/null | head -1)
stty -F $SERIAL_PORT 115200 raw -echo

# Serial-Monitor (Ctrl+C zum Beenden)
cat $SERIAL_PORT
```

### 3.3 Befehle senden

```bash
# Verbindungstest
echo "PING" > $SERIAL_PORT
echo "STATUS" > $SERIAL_PORT
echo "HELLO" > $SERIAL_PORT
echo "VERSION" > $SERIAL_PORT
echo "HELP" > $SERIAL_PORT
```

### 3.4 LED-Befehle (1-basiert!)

```bash
# Einzelne LED (one-hot)
echo "LEDSET 001" > $SERIAL_PORT   # LED 1 ein
echo "LEDSET 005" > $SERIAL_PORT   # LED 5 ein
echo "LEDSET 010" > $SERIAL_PORT   # LED 10 ein

# Additiv
echo "LEDON 001" > $SERIAL_PORT    # LED 1 ein (additiv)
echo "LEDON 002" > $SERIAL_PORT    # LED 2 ein (additiv)
echo "LEDOFF 001" > $SERIAL_PORT   # LED 1 aus

# Alle LEDs
echo "LEDALL" > $SERIAL_PORT       # Alle ein
echo "LEDCLR" > $SERIAL_PORT       # Alle aus

# LED-Test (Lauflicht)
echo "TEST" > $SERIAL_PORT
echo "STOP" > $SERIAL_PORT         # Test stoppen
```

### 3.5 Diagnose-Befehle

```bash
# Queue-Statistik zurücksetzen
echo "QRESET" > $SERIAL_PORT

# Status zeigt CURLED (aktuelle LED)
echo "STATUS" > $SERIAL_PORT
# Ausgabe:
# LEDS 0000100000
# CURLED 5          ← Aktuelle LED (1-basiert)
# BTNS 0000000000
# HEAP 372756
# QOVFL 0
# MODE PROTOTYPE
# MAPPING 15,12,13,11,10,9,8,14,7,4
```

### 3.6 Screen (interaktiv)

```bash
screen $SERIAL_PORT 115200
```

**Beenden:** `Ctrl+A`, dann `K`, dann `Y`

---

## 4 HTTP-Endpoints

```bash
# Status (JSON)
curl http://rover:8080/status | jq

# Health-Check (200 = healthy, 503 = degraded)
curl -w "%{http_code}" http://rover:8080/health

# Tastendruck simulieren (1-basiert!)
curl http://rover:8080/test/play/1
curl http://rover:8080/test/play/5
curl http://rover:8080/test/play/10

# Wiedergabe stoppen
curl http://rover:8080/test/stop
```

### Status-Response (v2.5.3)

```json
{
  "version": "2.5.3",
  "mode": "prototype",
  "num_media": 10,
  "current_button": 5,
  "ws_clients": 1,
  "serial_connected": true,
  "serial_port": "/dev/serial/by-id/usb-Espressif_USB_JTAG_serial_debug_unit_...",
  "media_missing": 0,
  "missing_files": [],
  "esp32_local_led": true
}
```

---

## 5 Deployment (Mac → Pi)

### 5.1 Mit rsync

```bash
cd ~/selection-panel

rsync -avz --delete \
    --exclude='firmware' \
    --exclude='hardwaretest' \
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

### 5.2 Mit Git (empfohlen)

```bash
# Auf dem Mac
git add -A && git commit -m "..." && git push

# Auf dem Pi
ssh rover "cd ~/selection-panel && git pull && sudo systemctl restart selection-panel"
```

---

## 6 Server-Steuerung (systemd)

```bash
# Starten
sudo systemctl start selection-panel

# Stoppen
sudo systemctl stop selection-panel

# Neu starten
sudo systemctl restart selection-panel

# Status prüfen
sudo systemctl status selection-panel

# Autostart aktivieren
sudo systemctl enable selection-panel

# Autostart deaktivieren
sudo systemctl disable selection-panel
```

### 6.1 Logs (journalctl)

```bash
# Live-Logs
journalctl -u selection-panel -f

# Letzte 50 Zeilen
journalctl -u selection-panel -n 50

# Letzte Stunde
journalctl -u selection-panel --since "1 hour ago"

# Heute
journalctl -u selection-panel --since today
```

---

## 7 Firmware flashen (Mac)

```bash
cd ~/selection-panel/firmware

# Kompilieren
pio run

# Flashen
pio run -t upload

# Serial-Monitor (PlatformIO)
pio device monitor

# Flash + Monitor
pio run -t upload -t monitor

# Clean Build
pio run -t clean
```

---

## 8 Medien verwalten

### 8.1 Prüfen (1-basiert: 001-010)

```bash
# Auf dem Pi
cd ~/selection-panel

# Medien auflisten
ls -la server/media/

# Medien-Check
for i in $(seq -w 1 10); do
    echo -n "0$i: "
    [ -f "server/media/0$i.jpg" ] && echo -n "JPG✓ " || echo -n "JPG✗ "
    [ -f "server/media/0$i.mp3" ] && echo -n "MP3✓" || echo -n "MP3✗"
    echo
done

# Anzahl prüfen
ls server/media/*.jpg 2>/dev/null | wc -l
ls server/media/*.mp3 2>/dev/null | wc -l
```

### 8.2 Generieren

```bash
# Test-Medien generieren (auf Mac)
./scripts/generate_test_media.sh 10    # Prototyp
./scripts/generate_test_media.sh 100   # Produktion
```

---

## 9 Diagnose

### 9.1 USB / Serial

```bash
# USB-Geräte
ls -la /dev/serial/by-id/usb-Espressif*
lsusb | grep -i espressif

# Wer nutzt den Serial-Port?
sudo fuser /dev/serial/by-id/usb-Espressif*

# Port freigeben (falls blockiert)
sudo systemctl stop selection-panel
sudo systemctl stop microros-agent.service  # falls vorhanden
```

### 9.2 Netzwerk

```bash
# IP-Adressen
ip addr | grep 192.168
hostname -I

# Hostname
hostname
```

### 9.3 Python

```bash
# Python-Version
python3 --version

# Pakete im venv
~/selection-panel/venv/bin/pip list

# Detailliert
~/selection-panel/venv/bin/pip freeze
```

### 9.4 System-Info

```bash
# Pi Hardware-Modell
cat /proc/device-tree/model

# Pi OS Version
cat /etc/os-release

# Kernel
uname -r

# Speicherplatz
df -h /

# RAM
free -h

# Alles auf einmal
echo "=== Pi Modell ===" && cat /proc/device-tree/model && \
echo -e "\n=== OS ===" && cat /etc/rpi-issue
```

---

## 10 Schnelltest (Komplettablauf)

```bash
# 1. Server starten (Terminal 1)
ssh rover
cd ~/selection-panel/server && source ../venv/bin/activate && python server.py

# 2. Dashboard öffnen (Mac)
open http://rover:8080/

# 3. Sound aktivieren (Button im Browser klicken)
#    → Medien werden vorgeladen ("Lade Medien... 5/10")

# 4. Alle 10 Taster durchdrücken
#    → Jeder Taster sollte Bild + Ton abspielen
#    → LED leuchtet sofort (< 1ms)
#    → Wiedergabe startet aus Cache (< 50ms)

# 5. Status prüfen
curl http://rover:8080/status | jq
```

---

## 11 Erwartete Ausgaben

### Server-Log (erfolgreich)

```
Button 1 gedrückt
GET /media/001.mp3 HTTP/1.1 206
Wiedergabe 1 beendet -> LEDs aus
```

### Dashboard Debug-Panel

```
Preloading 10 Medien...
Preload abgeschlossen: 10/10 OK (1823ms)
RX: {"type": "play", "id": 1}
Bild aus Cache: 001 (instant)
Audio aus Cache gestartet: 001 (12ms)
Audio beendet: 1
TX: {"type":"ended","id":1}
```

### Serial (cat $SERIAL_PORT)

```
PRESS 001
RELEASE 001
```

### Status-Endpoint

```json
{
  "version": "2.5.3",
  "mode": "prototype",
  "num_media": 10,
  "current_button": null,
  "ws_clients": 1,
  "serial_connected": true,
  "esp32_local_led": true
}
```
