# SPEC — Spezifikation Auswahlpanel

> **Single Source of Truth** für Protokolle, Pinbelegung und Policy.

| Metadaten | Wert |
|-----------|------|
| Version | 2.2.5 |
| Datum | 2025-12-27 |
| Status | Phase 6 (Integration) |

---

## 1 Glossar

| Begriff | Erklärung |
|---------|-----------|
| **One-hot** | Genau ein Bit ist aktiv, alle anderen sind aus |
| **Preempt** | Neue Aktion unterbricht sofort die laufende |
| **Race-Condition** | Timing-Problem, wenn zwei Ereignisse fast gleichzeitig auftreten |
| **FreeRTOS** | Echtzeit-Betriebssystem für Mikrocontroller |
| **Mutex** | Sperre, die gleichzeitigen Zugriff auf Ressourcen verhindert |
| **CMOS** | Chip-Technologie – Eingänge nie unbeschaltet lassen |
| **Pull-Up** | Widerstand zieht Signal auf HIGH, Taster zieht auf LOW |
| **Kaskadierung** | ICs in Reihe schalten, um mehr Ein-/Ausgänge zu erhalten |
| **DIP-16** | IC-Gehäuse mit 16 Pins in zwei Reihen |

---

## 2 Policy

### 2.1 One-hot LED

Zu jedem Zeitpunkt leuchtet **maximal eine LED**.

| Befehl | Wirkung |
|--------|---------|
| `LEDSET n` | LED *n* an, alle anderen aus |
| `LEDCLR` | Alle LEDs aus |

**Vorteil:** Maximalstrom = 1 × 20 mA statt 100 × 20 mA.

### 2.2 Preempt („Umschalten gewinnt")

Jeder neue Tastendruck unterbricht sofort die aktuelle Wiedergabe:

1. Pi → ESP32: `LEDSET n`
2. Pi → Browser: `{"type":"stop"}`
3. Pi → Browser: `{"type":"play","id":n}`

Nach Audio-Ende meldet der Browser `{"type":"ended","id":n}`.  
Pi sendet `LEDCLR` **nur wenn** `id == current_id` (Race-Condition-Schutz).

---

## 3 Pinbelegung ESP32-S3 XIAO

| Signal | Pin | Ziel-IC | Funktion |
|--------|-----|---------|----------|
| LED Data | D0 | 74HC595 SER | Serielle Daten für LEDs |
| LED Clock | D1 | 74HC595 SRCLK | Takt für Schieberegister |
| LED Latch | D2 | 74HC595 RCLK | Ausgabe freigeben |
| Btn Data | D3 | CD4021 Q8 | Serielle Daten von Tastern |
| Btn Clock | D4 | CD4021 CLK | Takt zum Auslesen |
| Btn Load | D5 | CD4021 P/S | Taster-Zustände einlesen |

---

## 4 CD4021BE vs. 74HC165

| Aspekt | 74HC165 | CD4021BE |
|--------|---------|----------|
| Load-Signal | LOW | **HIGH** |
| Shift-Signal | HIGH | LOW |
| DIP-Verfügbarkeit | Schwer | Gut |

> **Wichtig:** Die invertierte Load-Logik ist in der Firmware berücksichtigt.

---

## 5 Verdrahtungsregeln

| IC | Pin | Regel | Grund |
|----|-----|-------|-------|
| CD4021 (letzter) | Pin 11 (SER) | → VCC | CMOS-Eingänge nie floaten |
| 74HC595 (letzter) | Pin 9 (QH') | offen OK | Ausgang treibt aktiv |
| 74HC595 (alle) | Pin 10 (SRCLR) | → VCC | Clear deaktiviert |
| 74HC595 (alle) | Pin 13 (OE) | → GND | Outputs aktiviert |

---

## 6 Serial-Protokoll (ESP32 ↔ Pi)

115200 Baud, ASCII, Newline-terminiert.

### ESP32 → Pi

| Nachricht | Bedeutung |
|-----------|-----------|
| `READY` | Controller bereit |
| `PRESS nnn` | Taster gedrückt (000–099) |
| `RELEASE nnn` | Taster losgelassen |
| `OK` | Befehl erfolgreich |
| `PONG` | Antwort auf PING |

### Pi → ESP32

| Befehl | Funktion |
|--------|----------|
| `LEDSET nnn` | LED ein (one-hot) |
| `LEDON/LEDOFF nnn` | LED additiv ein/aus |
| `LEDCLR/LEDALL` | Alle aus/ein |
| `PING/STATUS` | Test/Zustand |
| `TEST/STOP` | LED-Lauflicht |
| `VERSION/QRESET` | Info/Reset |

---

## 7 WebSocket-Protokoll (Pi ↔ Browser)

Endpoint: `ws://<pi>:8080/ws`

| Richtung | Nachricht |
|----------|-----------|
| Pi → Browser | `{"type":"stop"}` |
| Pi → Browser | `{"type":"play","id":42}` |
| Browser → Pi | `{"type":"ended","id":42}` |

---

## 8 HTTP-Endpoints

| Pfad | Funktion |
|------|----------|
| `/` | Dashboard |
| `/ws` | WebSocket |
| `/media/*` | Bilder/Audio |
| `/test/play/{id}` | Wiedergabe testen |
| `/status` | Server-Status (JSON) |
| `/health` | Health-Check |

---

## 9 Medien-Konvention

| ID | Bild | Audio |
|----|------|-------|
| 42 | `media/042.jpg` | `media/042.mp3` |

IDs: 0–99 (intern), Protokoll: 000–099 (zero-padded).

---

## 10 Akzeptanztests

| Test | Erwartung |
|------|-----------|
| Preempt | Neuer Taster unterbricht sofort |
| One-hot | Nur eine LED leuchtet |
| Ende | Nach Audio: alle LEDs aus |
| Race | LED bleibt an wenn neue ID aktiv |
| Debounce | Nur ein Event pro Tastendruck |
