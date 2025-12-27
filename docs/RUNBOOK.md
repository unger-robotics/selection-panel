# RUNBOOK — Betrieb und Troubleshooting

> Debugging-Checklisten und Testprozeduren. Befehle: siehe [COMMANDS.md](COMMANDS.md)

| Version | 2.2.5 |
|---------|-------|

---

## 1 Schnellstart

```bash
open http://rover:8080/           # Dashboard
curl http://rover:8080/status     # Status
curl http://rover:8080/test/play/5  # Test
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

### 74HC595 (LEDs)

| Symptom | Ursache | Lösung |
|---------|---------|--------|
| Keine LEDs | OE nicht auf GND | Pin 13 → GND |
| Alle LEDs an | SRCLR auf LOW | Pin 10 → VCC |
| Falsche LED | Kaskadierung | QH' → SER prüfen |

### ESP32

| Symptom | Ursache | Lösung |
|---------|---------|--------|
| Keine Antwort | USB-Port belegt | Server stoppen |
| Kein Upload | Falscher Port | `ls /dev/serial/by-id/` |
| Reboot-Schleife | Watchdog | Firmware prüfen |

---

## 3 Browser-Debugging

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

### Shortcuts

| Taste | Funktion |
|-------|----------|
| `Space` | Play/Pause |
| `Ctrl+D` | Debug-Panel |

---

## 4 Server-Debugging

```bash
# Logs live
journalctl -u selection-panel -f

# Letzte 10 Minuten
journalctl -u selection-panel --since "10 min ago"

# Serial-Port prüfen
ssh pi@rover 'ls -l /dev/serial/by-id/'

# Benutzerrechte
ssh pi@rover 'groups pi'  # dialout muss enthalten sein
```

---

## 5 End-to-End Tests

### Preempt-Test

1. Button 5 drücken → LED 5 an, Audio läuft
2. Nach 1 s: Button 3 drücken
3. **Erwartet:** Audio 5 stoppt sofort, LED 3 an, Audio 3 läuft

### One-hot Test

1. `curl .../test/play/5`
2. `curl .../test/play/3`
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

### Stresstest

```bash
for i in {1..200}; do
    curl -s "http://rover:8080/test/play/$((RANDOM % 10))"
    sleep 0.2
done
```

---

## 6 Deployment-Checkliste

- [ ] Firmware geflasht
- [ ] server.py auf Pi kopiert
- [ ] venv mit Paketen erstellt
- [ ] Serial-Port eingetragen
- [ ] systemd-Service aktiviert
- [ ] Medien vorhanden (000–099)
- [ ] Reboot-Test bestanden
- [ ] Preempt-Test bestanden

---

## 7 Nützliche Einzeiler

```bash
# Medien zählen
ls ~/selection-panel/media/*.jpg | wc -l

# Service-Status
systemctl is-active selection-panel

# Firewall öffnen
sudo ufw allow 8080/tcp

# USB-Regeln neu laden
sudo udevadm control --reload-rules && sudo udevadm trigger
```
