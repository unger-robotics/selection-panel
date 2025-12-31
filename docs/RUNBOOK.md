# RUNBOOK — Betrieb und Troubleshooting

> Debugging-Checklisten und Testprozeduren. Befehle: siehe [COMMANDS.md](COMMANDS.md)

| Version | 2.4.1 |
|---------|-------|
| Stand | 2025-12-30 |

---

## 1 Schnellstart

```bash
open http://rover:8080/              # Dashboard
curl http://rover:8080/status | jq   # Status
curl http://rover:8080/test/play/5   # Test (1-basiert!)
```

---

## 2 Hardware-Debugging

### CD4021BE (Buttons)

| Symptom | Ursache | Lösung |
|---------|---------|--------|
| Keine Events | P/S-Logik invertiert | HIGH = Load, LOW = Shift |
| Falsche Bits | Kaskadierung falsch | Q8 → SER prüfen |
| Instabil | Fehlende Kondensatoren | 100 nF an VDD/VSS |
| Zufällige Trigger | SER floatet (letzter IC) | Pin 11 → VCC |
| Falsche Taster-Zuordnung | Bit-Mapping | `BUTTON_BIT_MAP` in config.h prüfen |

### 74HC595 (LEDs)

| Symptom | Ursache | Lösung |
|---------|---------|--------|
| Keine LEDs | OE nicht auf GND | Pin 13 → GND |
| Alle LEDs an | SRCLR auf LOW | Pin 10 → VCC |
| Falsche LED | Kaskadierung | QH' → SER prüfen |

### ESP32-S3 XIAO

| Symptom | Ursache | Lösung |
|---------|---------|--------|
| Keine Antwort | USB-Port belegt | Server stoppen: `sudo systemctl stop selection-panel` |
| Kein Upload | Falscher Port | `ls /dev/ttyACM*` |
| Reboot-Schleife | Watchdog | Firmware prüfen |
| Fragmentierte Serial-Daten | USB-CDC Timing | Firmware v2.4.0 mit `Serial.flush()` |

---

## 3 Serial-Debugging

### Port testen

```bash
# Port konfigurieren
stty -F /dev/ttyACM0 115200 raw -echo

# Daten empfangen (Ctrl+C zum Beenden)
cat /dev/ttyACM0

# Erwartete Ausgabe bei Tastendruck:
# PRESS 001
# RELEASE 001
```

### Befehle senden

```bash
echo "PING" > /dev/ttyACM0      # → PONG
echo "STATUS" > /dev/ttyACM0    # → LEDS, BTNS, HEAP
echo "LEDSET 001" > /dev/ttyACM0  # LED 1 ein
echo "LEDCLR" > /dev/ttyACM0    # Alle LEDs aus
```

### Häufige Serial-Probleme

| Symptom | Ursache | Lösung |
|---------|---------|--------|
| "Permission denied" | Fehlende Rechte | `sudo usermod -aG dialout $USER` → Neu einloggen |
| "Device busy" | Port belegt | `sudo fuser /dev/ttyACM0` → Prozess beenden |
| Fragmentierte Daten | USB-CDC | Firmware v2.4.0 verwenden |
| Keine Daten | Falscher Port | `ls /dev/ttyACM*` prüfen |

---

## 4 Browser-Debugging

### Kein Ton

1. „Sound aktivieren"-Button klicken (Autoplay-Sperre)
2. Audio-Status muss **grün** werden
3. DevTools → Console auf Fehler prüfen

### WebSocket-Verbindung

DevTools → Network → WS → Messages

| Status-Punkt | Grün | Rot |
|--------------|------|-----|
| WebSocket | Verbunden | Getrennt |
| Audio | Entsperrt | Gesperrt |

### Debug-Panel

Button unten rechts oder `Ctrl+D`:

```
RX: {"type": "play", "id": 5}
PLAY: 5
Audio gestartet: 005
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
| Taster nicht erkannt | Serial-Fragment | Parser prüfen, Firmware v2.4.0 |
| Medien fehlen | Falscher Pfad | `ls media/` (001.jpg - 010.jpg) |
| WebSocket-Fehler | Client-Absturz | Browser neu laden |

---

## 6 End-to-End Tests

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
- [ ] ESP32 mit Firmware v2.4.0 geflasht
- [ ] 10 Taster verdrahtet und funktionsfähig
- [ ] 10 LEDs verdrahtet und funktionsfähig
- [ ] USB-Kabel ESP32 ↔ Pi verbunden

### Software
- [ ] Repository auf Pi geklont
- [ ] venv erstellt: `python3 -m venv venv`
- [ ] Pakete installiert: `pip install aiohttp`
- [ ] server.py vorhanden (v2.4.1)

### Medien
- [ ] 10 Bilder: `media/001.jpg` - `media/010.jpg`
- [ ] 10 Audio: `media/001.mp3` - `media/010.mp3`

### Tests
- [ ] Serial-Test: `cat /dev/ttyACM0` zeigt PRESS/RELEASE
- [ ] Server startet ohne Fehler
- [ ] Dashboard öffnet sich
- [ ] "Sound aktivieren" funktioniert
- [ ] Alle 10 Taster erkannt
- [ ] Zuordnung Taster → Medien korrekt
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

# Port-Nutzung prüfen
sudo fuser /dev/ttyACM0

# USB-Geräte
lsusb | grep -i espressif

# Firewall öffnen (falls nötig)
sudo ufw allow 8080/tcp

# Server-Version
grep -m1 "VERSION" ~/selection-panel/server.py

# Firmware-Version (über Serial)
echo "VERSION" > /dev/ttyACM0 && sleep 0.5 && cat /dev/ttyACM0
```

---

## 9 Bekannte Probleme und Lösungen

| Problem | Status | Lösung |
|---------|--------|--------|
| ESP32-S3 USB-CDC fragmentiert | ✅ Behoben | Firmware v2.4.0 mit `Serial.flush()` |
| pyserial funktioniert nicht | ✅ Behoben | Server v2.4.1 mit `os.open` |
| Falsche Taster-Zuordnung | ✅ Behoben | Bit-Mapping in config.h |
| CD4021 Timing | ✅ Behoben | Längere Load-Pulse (5µs) |
| 0-basiert vs 1-basiert | ✅ Behoben | Durchgängig 1-basiert |
