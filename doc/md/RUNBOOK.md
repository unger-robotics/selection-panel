# RUNBOOK – Betrieb und Troubleshooting

> Debugging-Checklisten und Testprozeduren. Befehle: siehe [COMMANDS.md](COMMANDS.md)

| Version | 2.5.3 |
|---------|-------|
| Stand | 2026-01-31 |

---

## 1 Schnellstart

```bash
open http://rover:8080/              # Dashboard
curl http://rover:8080/status | jq   # Status
curl http://rover:8080/health        # Health (200/503)
curl http://rover:8080/test/play/5   # Test (1-basiert!)
```

---

## 2 Hardware-Debugging

### CD4021B (Buttons)

| Symptom | Ursache | Lösung |
|---------|---------|--------|
| Keine Events | P/S-Logik invertiert | HIGH = Load, LOW = Shift |
| Falsche Bits | Kaskadierung falsch | Q8 → DS prüfen |
| Instabil | Fehlende Kondensatoren | 100 nF an VDD/VSS |
| Zufällige Trigger | DS floatet (letzter IC) | Pin 11 → VCC |
| Falsche Taster-Zuordnung | Verdrahtung | BTN 1 → PI-8 (Pin 1) |
| First-Bit fehlt | SPI-Timing | First-Bit-Rescue in Firmware |

### 74HC595 (LEDs)

| Symptom | Ursache | Lösung |
|---------|---------|--------|
| Keine LEDs | OE nicht auf GND/PWM | Pin 13 → D2 oder GND |
| Alle LEDs an | SRCLR auf LOW | Pin 10 → VCC |
| Falsche LED | Kaskadierung | QH' → SER prüfen |

### ESP32-S3 XIAO

| Symptom | Ursache | Lösung |
|---------|---------|--------|
| Keine Antwort | USB-Port belegt | Server stoppen: `sudo systemctl stop selection-panel` |
| Kein Upload | Falscher Port | `ls /dev/serial/by-id/usb-Espressif*` |
| Reboot-Schleife | Watchdog | Firmware prüfen |
| Fragmentierte Serial-Daten | USB-CDC Timing | Firmware v2.5.3 mit `Serial.flush()` |

---

## 3 Serial-Debugging

### Port ermitteln (stabil)

```bash
# Stabilen by-id Pfad verwenden (empfohlen)
SERIAL_PORT=$(ls /dev/serial/by-id/usb-Espressif* 2>/dev/null | head -1)
echo "Port: $SERIAL_PORT"

# Port konfigurieren
stty -F $SERIAL_PORT 115200 raw -echo

# Daten empfangen (Ctrl+C zum Beenden)
cat $SERIAL_PORT

# Erwartete Ausgabe bei Tastendruck:
# PRESS 001
# RELEASE 001
```

### Befehle senden

```bash
echo "PING" > $SERIAL_PORT      # → PONG
echo "STATUS" > $SERIAL_PORT    # → active=N leds=0xNN
echo "VERSION" > $SERIAL_PORT   # → FW SelectionPanel v2.5.3
echo "LEDSET 001" > $SERIAL_PORT  # LED 1 ein
echo "LEDCLR" > $SERIAL_PORT    # Alle LEDs aus
```

### STATUS-Ausgabe (v2.5.3)

```
STATUS active=5 leds=0x10
LEDS 0000100000      ← Bit-Vektor (MSB links)
BTNS 1111111111      ← Active-Low: 1 = losgelassen
```

### Häufige Serial-Probleme

| Symptom | Ursache | Lösung |
|---------|---------|--------|
| "Permission denied" | Fehlende Rechte | `sudo usermod -aG dialout $USER` → Neu einloggen |
| "Device busy" | Port belegt | `sudo fuser /dev/ttyACM0` → Prozess beenden |
| Fragmentierte Daten | USB-CDC | Firmware v2.5.3 verwenden |
| Keine Daten | Falscher Port | `ls /dev/serial/by-id/usb-Espressif*` prüfen |
| Port ändert sich | Instabiler Pfad | by-id Pfad verwenden |

---

## 4 Browser-Debugging

### Kein Ton

1. "Sound aktivieren"-Button klicken (Autoplay-Sperre)
2. Warten: "Lade Medien... 5/10" → "Warte auf Tastendruck..."
3. Audio-Status muss **grün** werden
4. DevTools → Console auf Fehler prüfen

> **iOS/Safari:** Falls Unlock fehlschlägt, Safari komplett schließen und neu öffnen.
> Dashboard v2.5.1 verwendet AudioContext API mit Preloading.

### WebSocket-Verbindung

DevTools → Network → WS → Messages

| Status-Punkt | Grün | Rot |
|--------------|------|-----|
| WebSocket | Verbunden | Getrennt |
| Audio | Entsperrt | Gesperrt |

### Debug-Panel

Button unten rechts oder `Ctrl+D`:

```
Preloading 10 Medien...
Preload abgeschlossen: 10/10 OK (1823ms)
RX: {"type": "play", "id": 5}
Bild aus Cache: 005 (instant)
Audio aus Cache gestartet: 005 (12ms)
Audio beendet: 5
TX: {"type":"ended","id":5}
```

### Shortcuts

| Taste | Funktion |
|-------|----------|
| `Space` | Play/Pause |
| `Ctrl+D` | Debug-Panel |

---

## 5 Server-Debugging

```bash
# Logs live
journalctl -u selection-panel -f

# Letzte 10 Minuten
journalctl -u selection-panel --since "10 min ago"

# Service-Status
sudo systemctl status selection-panel

# Server manuell starten (für Debugging)
cd ~/selection-panel
source venv/bin/activate
python server.py
```

### Häufige Server-Probleme

| Symptom | Ursache | Lösung |
|---------|---------|--------|
| "Serial verbinde..." hängt | Port belegt | `sudo fuser /dev/ttyACM0` |
| Taster nicht erkannt | Serial-Fragment | Parser prüfen, Firmware v2.5.3 |
| Medien fehlen | Falscher Pfad | `ls media/` (001.jpg - 010.jpg) |
| WebSocket-Fehler | Client-Absturz | Browser neu laden |
| Health 503 | Serial getrennt | ESP32 Verbindung prüfen |

---

## 6 End-to-End Tests

### Latenz-Test

1. Button drücken
2. **LED-Reaktion:** < 1ms (lokal auf ESP32)
3. **Dashboard-Wiedergabe:** < 50ms (aus Cache)
4. **Gesamt:** < 70ms

### Preempt-Test

1. Button 5 drücken → LED 5 an, Audio läuft
2. Nach 1 s: Button 3 drücken
3. **Erwartet:** Audio 5 stoppt sofort, LED 3 an, Audio 3 läuft

### One-hot Test

1. `curl http://rover:8080/test/play/5`
2. `curl http://rover:8080/test/play/3`
3. **Erwartet:** Nur LED 3 leuchtet

### Ende-Test

1. Audio abspielen lassen
2. Warten bis Ende
3. **Erwartet:** Alle LEDs aus

### Race-Condition Test

1. Button 3 drücken, Audio läuft
2. Kurz vor Ende: Button 5 drücken
3. Audio 3 endet
4. **Erwartet:** LED 5 bleibt an (nicht aus!)

### Cache-Test

1. Sound aktivieren → Preload abwarten
2. Button drücken
3. **Erwartet:** Bild + Audio aus Cache (< 50ms)
4. Debug-Panel zeigt "aus Cache"

### Alle-Taster-Test (Prototyp)

```bash
# Jeden Taster einzeln drücken
# Erwartete Zuordnung (1-basiert):
# Taster 1 → Bild 001 + Ton 001
# Taster 2 → Bild 002 + Ton 002
# ...
# Taster 10 → Bild 010 + Ton 010
```

### Stresstest

```bash
for i in {1..200}; do
    curl -s "http://rover:8080/test/play/$((RANDOM % 10 + 1))"
    sleep 0.2
done
```

---

## 7 Deployment-Checkliste

### Hardware
- [ ] ESP32 mit Firmware v2.5.3 geflasht
- [ ] 10 Taster verdrahtet und funktionsfähig
- [ ] 10 LEDs verdrahtet und funktionsfähig
- [ ] USB-Kabel ESP32 ↔ Pi verbunden (Daten, nicht nur Laden!)

### Software
- [ ] Repository auf Pi geklont
- [ ] venv erstellt: `python3 -m venv venv`
- [ ] Pakete installiert: `pip install aiohttp`
- [ ] server.py vorhanden (v2.5.3)

### Medien
- [ ] 10 Bilder: `media/001.jpg` - `media/010.jpg`
- [ ] 10 Audio: `media/001.mp3` - `media/010.mp3`

### Tests
- [ ] Serial-Test: `cat /dev/serial/by-id/usb-Espressif*` zeigt PRESS/RELEASE
- [ ] Server startet ohne Fehler
- [ ] Dashboard öffnet sich
- [ ] "Sound aktivieren" funktioniert
- [ ] Preload abgeschlossen ("10/10 OK")
- [ ] Alle 10 Taster erkannt
- [ ] Zuordnung Taster → Medien korrekt
- [ ] LED-Reaktion < 1ms ✓
- [ ] Dashboard-Latenz < 50ms ✓
- [ ] Preempt-Test bestanden

### Autostart (optional)
- [ ] systemd-Service kopiert
- [ ] Service aktiviert: `sudo systemctl enable selection-panel`
- [ ] Reboot-Test bestanden

---

## 8 Nützliche Einzeiler

```bash
# Medien zählen
ls ~/selection-panel/media/*.jpg 2>/dev/null | wc -l
ls ~/selection-panel/media/*.mp3 2>/dev/null | wc -l

# Service-Status
systemctl is-active selection-panel

# Health-Check
curl -s http://rover:8080/health | jq -r '.status'

# Port-Nutzung prüfen
sudo fuser /dev/ttyACM0

# USB-Geräte
lsusb | grep -i espressif

# Serial-Port finden (stabil)
ls /dev/serial/by-id/usb-Espressif*

# Firewall öffnen (falls nötig)
sudo ufw allow 8080/tcp

# Server-Version
grep -m1 "VERSION" ~/selection-panel/server.py

# Firmware-Version (über Serial)
SERIAL_PORT=$(ls /dev/serial/by-id/usb-Espressif* | head -1)
echo "VERSION" > $SERIAL_PORT && sleep 0.5 && cat $SERIAL_PORT
```

---

## 9 Bekannte Probleme und Lösungen

| Problem | Status | Lösung |
|---------|--------|--------|
| iOS Audio-Unlock fehlgeschlagen | ✅ Behoben | Dashboard v2.5.1 mit AudioContext API |
| ESP32-S3 USB-CDC fragmentiert | ✅ Behoben | Firmware v2.5.3 mit `Serial.flush()` |
| pyserial funktioniert nicht | ✅ Behoben | Server v2.5.3 mit `os.open` |
| CD4021B First-Bit-Problem | ✅ Behoben | First-Bit-Rescue in Firmware |
| CD4021B Timing | ✅ Behoben | 2µs Load-Pulse |
| 0-basiert vs 1-basiert | ✅ Behoben | Durchgängig 1-basiert |
| LED-Latenz durch Roundtrip | ✅ Behoben | Firmware v2.5.3 (lokal < 1ms) |
| Dashboard-Latenz | ✅ Behoben | Dashboard v2.5.1 (Preload < 50ms) |
| Serial-Pfad instabil | ✅ Behoben | by-id Pfad verwenden |

---

*Stand: 2026-01-08 | Version 2.5.3*
