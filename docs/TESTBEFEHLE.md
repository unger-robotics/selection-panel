# Testbefehle Selection Panel

## 1. Server starten

```bash
# Auf dem Pi
ssh rover
cd ~/selection-panel
source venv/bin/activate
python server.py
```

## 2. Serial direkt testen (ohne Server)

```bash
# Terminal-Einstellungen
stty -F /dev/ttyACM0 115200 raw -echo

# Serial-Monitor (Ctrl+C zum Beenden)
cat /dev/ttyACM0

# Befehle senden
echo "PING" > /dev/ttyACM0
echo "STATUS" > /dev/ttyACM0
echo "HELLO" > /dev/ttyACM0
echo "VERSION" > /dev/ttyACM0
```

## 3. LED-Befehle (Serial)

```bash
# Einzelne LED (1-basiert)
echo "LEDSET 001" > /dev/ttyACM0   # LED 1 ein (one-hot)
echo "LEDSET 005" > /dev/ttyACM0   # LED 5 ein (one-hot)
echo "LEDSET 010" > /dev/ttyACM0   # LED 10 ein (one-hot)

# Additiv
echo "LEDON 001" > /dev/ttyACM0    # LED 1 ein (additiv)
echo "LEDON 002" > /dev/ttyACM0    # LED 2 ein (additiv)
echo "LEDOFF 001" > /dev/ttyACM0   # LED 1 aus

# Alle LEDs
echo "LEDALL" > /dev/ttyACM0       # Alle ein
echo "LEDCLR" > /dev/ttyACM0       # Alle aus

# LED-Test (Lauflicht)
echo "TEST" > /dev/ttyACM0
echo "STOP" > /dev/ttyACM0         # Test stoppen
```

## 4. HTTP-Endpoints (Server muss laufen)

```bash
# Status abfragen
curl http://rover:8080/status | jq

# Health-Check
curl http://rover:8080/health | jq

# Tastendruck simulieren (1-basiert)
curl http://rover:8080/test/play/1
curl http://rover:8080/test/play/5
curl http://rover:8080/test/play/10

# Wiedergabe stoppen
curl http://rover:8080/test/stop
```

## 5. WebSocket testen

```bash
# Mit websocat (falls installiert)
websocat ws://rover:8080/ws

# Oder im Browser: http://rover:8080/
# Dashboard öffnen und Debug-Panel aktivieren
```

## 6. Medien prüfen

```bash
# Auf dem Pi
cd ~/selection-panel

# Medien auflisten
ls -la media/

# Medien-Check (erwartet: 001.jpg/mp3 - 010.jpg/mp3)
for i in $(seq -w 1 10); do
    echo -n "$i: "
    [ -f "media/0$i.jpg" ] && echo -n "JPG✓ " || echo -n "JPG✗ "
    [ -f "media/0$i.mp3" ] && echo -n "MP3✓" || echo -n "MP3✗"
    echo
done

# Medien neu generieren (auf Mac)
./scripts/generate_test_media.sh 10
```

## 7. Service-Status (falls konfiguriert)

```bash
# Service-Status
sudo systemctl status selection-panel

# Service starten/stoppen
sudo systemctl start selection-panel
sudo systemctl stop selection-panel
sudo systemctl restart selection-panel

# Logs anzeigen
sudo journalctl -u selection-panel -f
```

## 8. Diagnose

```bash
# USB-Geräte
ls -la /dev/ttyACM*
lsusb | grep -i espressif

# Wer nutzt den Serial-Port?
sudo fuser /dev/ttyACM0

# Port freigeben (falls blockiert)
sudo systemctl stop microros-agent.service

# Netzwerk
ip addr | grep 192.168
hostname -I

# Python-Pakete
pip list | grep -E "aiohttp|pyserial"
```

## 9. Firmware flashen (Mac)

```bash
cd ~/selection-panel/button_panel_firmware

# Build
pio run

# Upload
pio run -t upload

# Serial-Monitor (PlatformIO)
pio device monitor
```

## 10. Git-Status

```bash
# Auf Mac
cd ~/selection-panel
git status
git log --oneline -10

# Auf Pi synchronisieren
ssh rover "cd ~/selection-panel && git pull"
```

---

## Schnelltest (Komplettablauf)

```bash
# 1. Server starten (Terminal 1)
ssh rover
cd ~/selection-panel && source venv/bin/activate && python server.py

# 2. Dashboard öffnen
open http://rover:8080/

# 3. Sound aktivieren (Button im Browser klicken)

# 4. Alle 10 Taster durchdrücken
#    → Jeder Taster sollte Bild + Ton abspielen
#    → LED leuchtet während Wiedergabe

# 5. Status prüfen
curl http://rover:8080/status | jq
```

---

## Erwartete Ausgabe

### Server-Log (erfolgreich)
```
Button 1 gedrueckt
GET /media/001.mp3 HTTP/1.1 206
Wiedergabe 1 beendet -> LEDs aus
```

### Dashboard Debug-Panel
```
RX: {"type": "play", "id": 1}
Audio gestartet: 001
TX: {"type":"ended","id":1}
```

### Serial (cat /dev/ttyACM0)
```
PRESS 001
RELEASE 001
```
