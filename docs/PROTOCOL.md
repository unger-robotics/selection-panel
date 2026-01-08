# Selection Panel: Protokoll-Referenz

Version 2.5.2 | Serial + WebSocket API

## Übersicht

Das Selection Panel verwendet zwei Protokolle:

1. **Serial-Protokoll** (ESP32 ↔ Pi): Zeilenbasiert, ASCII, LF-terminiert
2. **WebSocket-Protokoll** (Server ↔ Browser): JSON-basiert

```
┌─────────────┐                          ┌─────────────┐                          ┌─────────────┐
│             │   PRESS 001              │             │ {"type":"play","id":1}   │             │
│   ESP32-S3  │ ──────────────────────▶  │  server.py  │ ──────────────────────▶  │   Browser   │
│             │                          │             │                          │             │
│             │   LEDCLR                 │             │ {"type":"ended","id":1}  │             │
│             │ ◀──────────────────────  │             │ ◀──────────────────────  │             │
└─────────────┘                          └─────────────┘                          └─────────────┘
     Serial                                                    WebSocket
 USB-CDC @ 115200                                           ws://host:8080/ws
```

## Verbindungsaufbau

### Serial (ESP32 ↔ Pi)

Nach dem Start sendet der ESP32 automatisch:

```
READY
FW SelectionPanel v2.5.2
```

Der Server sollte auf `READY` warten, bevor er Befehle sendet. Empfohlener Timeout: 5 Sekunden.

### WebSocket (Server ↔ Browser)

Der Browser verbindet sich zu `ws://host:8080/ws`. Nach Verbindung ist der Client sofort empfangsbereit.

---

# Teil 1: Serial-Protokoll (ESP32 ↔ Pi)

IDs sind 1-basiert und 3-stellig formatiert (001-100). Physikalisch: USB-CDC @ 115200 Baud, 8N1.

## Nachrichten ESP32 → Pi

### READY

Signalisiert, dass der ESP32 betriebsbereit ist.

```
READY
```

### FW (Firmware-Version)

Meldet die installierte Firmware-Version.

```
FW SelectionPanel v2.5.2
```

### PRESS / RELEASE

Taster-Events. Die ID ist 1-basiert (001-100).

```
PRESS 001
RELEASE 001
```

Die Events werden erst nach erfolgreicher Entprellung (30 ms) gesendet.

### PONG

Antwort auf `PING`.

```
← PING
→ PONG
```

### OK

Bestätigung nach erfolgreich ausgeführtem Befehl.

```
← LEDSET 005
→ OK
```

### ERROR

Fehlermeldung bei ungültigem Befehl oder Parameter.

```
← FOOBAR
→ ERROR unknown command

← LEDSET 999
→ ERROR invalid id
```

## Befehle Pi → ESP32

### PING

Prüft die Verbindung. Antwort: `PONG`.

```
← PING
→ PONG
```

### STATUS

Fragt den aktuellen Zustand ab. Antwort: Aktuelle Auswahl und LED-Zustand.

```
← STATUS
→ STATUS active=5 leds=0x10
```

### VERSION

Fragt die Firmware-Version ab.

```
← VERSION
→ FW SelectionPanel v2.5.2
```

### HELP

Listet verfügbare Befehle auf.

```
← HELP
→ HELP: PING STATUS VERSION LEDSET LEDON LEDOFF LEDCLR LEDALL
```

### LEDSET

One-Hot-Modus: Schaltet nur die angegebene LED an, alle anderen aus.

```
← LEDSET 003
→ OK

# Ergebnis: Nur LED 3 leuchtet
```

### LEDON

Schaltet eine LED additiv ein (andere bleiben unverändert).

```
← LEDON 003
→ OK

← LEDON 005
→ OK

# Ergebnis: LED 3 und 5 leuchten
```

### LEDOFF

Schaltet eine LED aus (andere bleiben unverändert).

```
← LEDOFF 003
→ OK

# Ergebnis: LED 3 aus, LED 5 bleibt an
```

### LEDCLR

Schaltet alle LEDs aus.

```
← LEDCLR
→ OK

# Ergebnis: Alle LEDs aus
```

### LEDALL

Schaltet alle LEDs an.

```
← LEDALL
→ OK

# Ergebnis: Alle 10 LEDs leuchten
```

## Fehlerbehandlung

| Fehlermeldung | Ursache | Lösung |
|---------------|---------|--------|
| `ERROR unknown command` | Unbekannter Befehl | Befehlsname prüfen |
| `ERROR invalid id` | ID außerhalb 1-100 | ID-Bereich prüfen |
| `ERROR missing id` | ID fehlt | `LEDSET <id>` mit ID |

## Timing

| Parameter | Wert | Beschreibung |
|-----------|------|--------------|
| Baudrate | 115200 | 8N1 |
| Event-Latenz | < 35 ms | Abtastung + Entprellung |
| Befehl-Antwort | < 5 ms | Typische Antwortzeit |

Die Event-Latenz setzt sich zusammen aus:
- IO-Zyklus: 5 ms (worst case: gerade verpasst)
- Debounce: 30 ms
- Serial: < 1 ms

## Protokoll-Muster

### Echo-Modus (LED folgt Taster)

```python
while True:
    line = ser.readline().decode().strip()
    if line.startswith("PRESS"):
        btn_id = line.split()[1]
        ser.write(f"LEDSET {btn_id}\n".encode())
```

### Toggle-Modus

```python
led_state = [False] * 10

while True:
    line = ser.readline().decode().strip()
    if line.startswith("PRESS"):
        btn_id = int(line.split()[1])
        led_state[btn_id - 1] = not led_state[btn_id - 1]
        
        if led_state[btn_id - 1]:
            ser.write(f"LEDON {btn_id:03d}\n".encode())
        else:
            ser.write(f"LEDOFF {btn_id:03d}\n".encode())
```

### Sequenz-Modus

```python
sequence = []
MAX_LEN = 5

while True:
    line = ser.readline().decode().strip()
    if line.startswith("PRESS"):
        btn_id = int(line.split()[1])
        sequence.append(btn_id)
        
        if len(sequence) > MAX_LEN:
            sequence.pop(0)
        
        # Alle LEDs der Sequenz anzeigen
        ser.write(b"LEDCLR\n")
        for led_id in sequence:
            ser.write(f"LEDON {led_id:03d}\n".encode())
```

## Erweiterung für 100 Buttons

Das Protokoll skaliert automatisch auf 100 Taster/LEDs. Die ID-Formatierung (3-stellig) unterstützt Werte von 001 bis 100.

```
PRESS 042
LEDSET 099
```

## Debug-Modus

Mit `SERIAL_PROTOCOL_ONLY = false` in config.h werden zusätzliche Debug-Informationen ausgegeben. Diese sollten vom Parser ignoriert werden:

```
[IO] Raw: 0xFF 0xFF
[IO] Debounced: 0xFE 0xFF
PRESS 001
[IO] LED: 0x01 0x00
```

Für Produktivbetrieb: `SERIAL_PROTOCOL_ONLY = true` verwenden.

## Beispiel-Session

Vollständige Beispiel-Kommunikation:

```
# ESP32 startet
→ READY
→ FW SelectionPanel v2.5.2

# Pi prüft Verbindung
← PING
→ PONG

# Pi fragt Status ab
← STATUS
→ STATUS active=0 leds=0x00

# Benutzer drückt Taster 3
→ PRESS 003

# Pi schaltet LED 3 an
← LEDSET 003
→ OK

# Benutzer lässt Taster 3 los
→ RELEASE 003

# Benutzer drückt Taster 7
→ PRESS 007

# Pi schaltet auf LED 7 um
← LEDSET 007
→ OK

# Pi schaltet zusätzlich LED 1 an
← LEDON 001
→ OK

# Pi schaltet alle LEDs aus
← LEDCLR
→ OK

# Pi schaltet alle LEDs an
← LEDALL
→ OK

# Ungültiger Befehl
← FOOBAR
→ ERROR unknown command
```

---

# Teil 2: WebSocket-Protokoll (Server ↔ Browser)

Der Server kommuniziert mit dem Browser-Dashboard über WebSocket. Die Verbindung erfolgt zu `ws://host:8080/ws` (oder `wss://` bei HTTPS).

## Server → Browser

### play

Startet die Medien-Wiedergabe für eine Button-ID.

```json
{"type": "play", "id": 3}
```

Der Browser zeigt das entsprechende Bild (`/media/003.jpg`) und spielt das Audio (`/media/003.mp3`) ab.

### stop

Stoppt die aktuelle Wiedergabe.

```json
{"type": "stop"}
```

## Browser → Server

### ended

Signalisiert, dass das Audio für eine ID beendet ist.

```json
{"type": "ended", "id": 3}
```

Der Server sendet daraufhin `LEDCLR` an den ESP32.

### ping

Heartbeat zur Verbindungsprüfung (optional).

```json
{"type": "ping"}
```

## WebSocket-Ablauf

```
Browser                      Server                       ESP32
   │                            │                            │
   │◀── WebSocket Connect ──────│                            │
   │                            │                            │
   │                            │◀──── PRESS 003 ────────────│
   │                            │                            │
   │◀── {"type":"play","id":3} ─│                            │
   │                            │                            │
   │    [Audio spielt...]       │                            │
   │                            │                            │
   │── {"type":"ended","id":3} ▶│                            │
   │                            │                            │
   │                            │───── LEDCLR ──────────────▶│
   │                            │                            │
```

## HTTP-Endpoints

Der Server bietet zusätzliche HTTP-Endpoints:

| Endpoint | Beschreibung | Beispiel-Antwort |
|----------|--------------|------------------|
| `GET /` | Dashboard (index.html) | HTML |
| `GET /status` | Server-Status | `{"version":"2.5.2","serial_connected":true,...}` |
| `GET /health` | Health-Check | `{"status":"healthy"}` |
| `GET /test/play/{id}` | Simuliert Tastendruck | `OK: play 3` |
| `GET /test/stop` | Stoppt Wiedergabe | `OK: stopped` |
| `GET /static/` | Statische Dateien | JS, CSS, Favicon |
| `GET /media/` | Medien-Dateien | 001.jpg, 001.mp3 |

### Status-Endpoint

```bash
curl http://rover.local:8080/status | jq
```

```json
{
  "version": "2.5.2",
  "mode": "prototype",
  "num_media": 10,
  "current_button": 3,
  "ws_clients": 2,
  "serial_connected": true,
  "serial_port": "/dev/serial/by-id/...",
  "media_missing": 0,
  "esp32_local_led": true
}
```

## Besonderheit: ESP32 setzt LED lokal

Mit `ESP32_SETS_LED_LOCALLY = true` (Standard) setzt der ESP32 bei Tastendruck die LED selbst. Der Server muss dann nicht `LEDSET` senden – er sendet nur `LEDCLR` wenn das Audio beendet ist.

Dies reduziert die Latenz zwischen Tastendruck und LED-Reaktion auf unter 5 ms.
