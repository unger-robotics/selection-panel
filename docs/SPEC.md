# SPEC – Spezifikation Auswahlpanel

> **Single Source of Truth** für Protokolle, Pinbelegung und Policy.

| Metadaten | Wert |
|-----------|------|
| Version | 2.4.2 |
| Datum | 2025-01-01 |
| Status | ✅ Prototyp funktionsfähig (10×) |

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
| **USB-CDC** | USB Communications Device Class (virtueller COM-Port) |
| **1-basiert** | Nummerierung beginnt bei 1 (nicht 0) |
| **Preloading** | Medien vorladen bevor sie benötigt werden |

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

Jeder neue Tastendruck unterbricht sofort die aktuelle Wiedergabe.

**Mit ESP32 v2.4.1+ (lokale LED-Steuerung):**

1. ESP32 setzt LED sofort (< 1ms)
2. ESP32 → Pi: `PRESS 005`
3. Pi → Browser: `{"type":"stop"}` + `{"type":"play","id":5}` (parallel)
4. Browser spielt aus Cache (< 50ms)

**Race-Condition-Schutz:** Nach Audio-Ende meldet der Browser `{"type":"ended","id":5}`. Pi sendet `LEDCLR` **nur wenn** `id == current_id`.

---

## 3 Nummerierung (1-basiert!)

**Alle IDs sind 1-basiert** (001-100), nicht 0-basiert (000-099).

| Schicht | Format | Beispiel |
|---------|--------|----------|
| Taster (physisch) | 1-100 | Taster 1, Taster 10 |
| Serial-Protokoll | 001-100 | `PRESS 001`, `LEDSET 010` |
| WebSocket | 1-100 | `{"type":"play","id":1}` |
| Medien-Dateien | 001-100 | `001.jpg`, `010.mp3` |
| Dashboard-Anzeige | 001-100 | "001", "010" |

**Keine Konvertierung nötig** – durchgängig 1-basiert in allen Schichten.

---

## 4 Pinbelegung ESP32-S3 XIAO

| Signal | Pin | Ziel-IC | Funktion |
|--------|-----|---------|----------|
| LED_DATA | D0 | 74HC595 Pin 14 (SER) | Serielle Daten für LEDs |
| LED_CLK | D1 | 74HC595 Pin 11 (SRCLK) | Takt für Schieberegister |
| LED_LATCH | D2 | 74HC595 Pin 12 (RCLK) | Ausgabe freigeben |
| BTN_DATA | D3 | CD4021 Pin 3 (Q8) | Serielle Daten von Tastern |
| BTN_CLK | D4 | CD4021 Pin 10 (CLK) | Takt zum Auslesen |
| BTN_LOAD | D5 | CD4021 Pin 9 (P/S) | Taster-Zustände einlesen |

---

## 5 CD4021BE vs. 74HC165

| Aspekt | 74HC165 | CD4021BE |
|--------|---------|----------|
| Load-Signal | LOW | **HIGH** |
| Shift-Signal | HIGH | LOW |
| DIP-Verfügbarkeit | Schwer | Gut |
| Load-Puls | 1 µs | **2 µs** (CMOS) |
| Clock-Puls | 1 µs | **1 µs** |

> **Wichtig:** Die invertierte Load-Logik und längere Pulse sind in der Firmware berücksichtigt.

---

## 6 Verdrahtungsregeln

| IC | Pin | Regel | Grund |
|----|-----|-------|-------|
| CD4021 (letzter) | Pin 11 (SER) | → VCC | CMOS-Eingänge nie floaten |
| 74HC595 (letzter) | Pin 9 (QH') | offen OK | Ausgang treibt aktiv |
| 74HC595 (alle) | Pin 10 (SRCLR) | → VCC | Clear deaktiviert |
| 74HC595 (alle) | Pin 13 (OE) | → GND | Outputs aktiviert |
| Alle ICs | VCC/VDD | 100 nF → GND | Abblockkondensator |

---

## 7 Serial-Protokoll (ESP32 ↔ Pi)

115200 Baud, ASCII, Newline-terminiert (`\r\n`).

### 7.1 ESP32 → Pi

| Nachricht | Bedeutung |
|-----------|-----------|
| `READY` | Controller bereit |
| `PRESS 001` | Taster 1 gedrückt (001-100) |
| `RELEASE 001` | Taster 1 losgelassen |
| `OK` | Befehl erfolgreich |
| `PONG` | Antwort auf PING |
| `ERROR msg` | Fehlermeldung |
| `FW ...` | Firmware-Version |
| `MODE ...` | Build-Modus |
| `MAPPING ...` | Bit-Mapping Status |

### 7.2 Pi → ESP32

| Befehl | Funktion |
|--------|----------|
| `LEDSET 001` | LED 1 ein (one-hot) |
| `LEDON 001` | LED 1 ein (additiv) |
| `LEDOFF 001` | LED 1 aus |
| `LEDCLR` | Alle LEDs aus |
| `LEDALL` | Alle LEDs ein |
| `PING` | Verbindungstest → PONG |
| `STATUS` | Zustand abfragen |
| `TEST` | LED-Lauflicht starten |
| `STOP` | LED-Test stoppen |
| `VERSION` | Firmware-Version |
| `HELLO` | Startup-Nachricht |
| `QRESET` | Queue-Statistik zurücksetzen |
| `HELP` | Befehlsliste |

### 7.3 STATUS-Ausgabe (v2.4.1+)

```
LEDS 0000100000      # Bit-Vektor (MSB links)
CURLED 5             # Aktuelle LED (1-basiert, 0 = keine)
BTNS 0000000000      # Bit-Vektor
HEAP 372756          # Freier Heap (Bytes)
CORE 1               # Aktueller CPU-Core
QFREE 16             # Freie Queue-Slots
QOVFL 0              # Queue-Overflow-Zähler
MODE PROTOTYPE       # Build-Modus
MAPPING ON           # Bit-Mapping aktiv
```

### 7.4 USB-CDC Besonderheit

ESP32-S3 XIAO nutzt USB-CDC statt UART. **Wichtig:**

```cpp
// Firmware muss Serial.flush() verwenden!
Serial.println(buffer);
Serial.flush();  // Verhindert Fragmentierung
```

---

## 8 WebSocket-Protokoll (Pi ↔ Browser)

Endpoint: `ws://<pi>:8080/ws`

### 8.1 Pi → Browser

| Nachricht | Bedeutung |
|-----------|-----------|
| `{"type":"stop"}` | Wiedergabe stoppen |
| `{"type":"play","id":5}` | Medien 5 abspielen |

### 8.2 Browser → Pi

| Nachricht | Bedeutung |
|-----------|-----------|
| `{"type":"ended","id":5}` | Audio 5 beendet |
| `{"type":"ping"}` | Heartbeat |

**ID ist 1-basiert** (1-100, nicht 0-99).

---

## 9 HTTP-Endpoints

| Pfad | Funktion |
|------|----------|
| `/` | Dashboard (index.html) |
| `/ws` | WebSocket-Verbindung |
| `/static/*` | CSS, JavaScript |
| `/media/*` | Bilder und Audio |
| `/test/play/{id}` | Wiedergabe simulieren (1-basiert) |
| `/test/stop` | Wiedergabe stoppen |
| `/status` | Server-Status (JSON) |
| `/health` | Health-Check (200/503) |

### 9.1 Status-Response (v2.4.2)

```json
{
  "version": "2.4.2",
  "mode": "prototype",
  "num_media": 10,
  "current_button": 5,
  "ws_clients": 1,
  "serial_connected": true,
  "serial_port": "/dev/ttyACM0",
  "media_missing": 0,
  "missing_files": [],
  "esp32_local_led": true
}
```

---

## 10 Medien-Konvention

| ID | Bild | Audio |
|----|------|-------|
| 1 | `media/001.jpg` | `media/001.mp3` |
| 5 | `media/005.jpg` | `media/005.mp3` |
| 10 | `media/010.jpg` | `media/010.mp3` |
| 100 | `media/100.jpg` | `media/100.mp3` |

**IDs: 1-100 (1-basiert), Dateien: 001-100 (zero-padded, 3 Stellen).**

---

## 11 Latenz-Budget

| Komponente | Latenz | Beschreibung |
|------------|--------|--------------|
| ESP32 LED | < 1ms | Lokale Steuerung (v2.4.1+) |
| Serial | ~5ms | USB-CDC Übertragung |
| Server | ~1ms | asyncio.gather (v2.4.2) |
| WebSocket | ~5ms | Netzwerk |
| Dashboard | < 50ms | Aus Cache (v2.3.0) |
| **Gesamt** | **< 70ms** | Tastendruck → Wiedergabe |

---

## 12 Akzeptanztests

| Test | Erwartung | Status |
|------|-----------|--------|
| Preempt | Neuer Taster unterbricht sofort | ✅ |
| One-hot | Nur eine LED leuchtet | ✅ |
| Ende | Nach Audio: alle LEDs aus | ✅ |
| Race | LED bleibt an wenn neue ID aktiv | ✅ |
| Debounce | Nur ein Event pro Tastendruck | ✅ |
| Zuordnung | Taster 1 → Medien 001 | ✅ |
| Alle Taster | 10/10 erkannt (Prototyp) | ✅ |
| LED-Latenz | < 1ms | ✅ |
| Dashboard-Latenz | < 50ms | ✅ |

---

## 13 Versionen

| Komponente | Version | Änderung |
|------------|---------|----------|
| Firmware | 2.4.1 | LED sofort bei Tastendruck, CURLED, QRESET |
| Server | 2.4.2 | asyncio.gather, ESP32_SETS_LED_LOCALLY |
| Dashboard | 2.3.0 | Preloading, Cache für sofortige Wiedergabe |

---

## 14 Bekannte Einschränkungen

| Problem | Status | Lösung |
|---------|--------|--------|
| ESP32-S3 USB-CDC fragmentiert | ✅ Behoben | `Serial.flush()` |
| pyserial funktioniert nicht | ✅ Behoben | `os.open` + `stty` |
| CD4021 braucht längere Pulse | ✅ Behoben | 2 µs Load, 1 µs Clock |
| Prototyp nicht-linear verdrahtet | ✅ Behoben | Bit-Mapping |
| LED-Latenz durch Roundtrip | ✅ Behoben | Lokale LED-Steuerung |
| Dashboard-Latenz | ✅ Behoben | Medien-Preloading |
