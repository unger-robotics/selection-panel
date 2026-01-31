# SPEC – Spezifikation Auswahlpanel

> **Single Source of Truth** für Protokolle, Pinbelegung und Policy.

| Metadaten | Wert |
|-----------|------|
| Version | 2.5.2 |
| Datum | 2026-01-08 |
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

### 2.2 Preempt ("Umschalten gewinnt")

Jeder neue Tastendruck unterbricht sofort die aktuelle Wiedergabe.

**Mit ESP32 v2.5.2 (lokale LED-Steuerung):**

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
| MOSI | D10 | 74HC595 Pin 14 (SER) | Serielle Daten für LEDs |
| SCK | D8 | 74HC595 Pin 11 + CD4021B Pin 10 | Gemeinsamer SPI-Takt |
| RCK | D0 | 74HC595 Pin 12 (RCLK) | LED-Latch (Ausgabe freigeben) |
| OE | D2 | 74HC595 Pin 13 (OE) | Output Enable (PWM für Helligkeit) |
| MISO | D9 | CD4021B Pin 3 (Q8) | Serielle Daten von Tastern |
| P/S | D1 | CD4021B Pin 9 (P/S) | Parallel Load (HIGH = Load) |

**SPI-Modi:**
- 74HC595: MODE0 (CPOL=0, CPHA=0) @ 1 MHz
- CD4021B: MODE1 (CPOL=0, CPHA=1) @ 500 kHz

---

## 5 CD4021B vs. 74HC165

| Aspekt | 74HC165 | CD4021B |
|--------|---------|---------|
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
| CD4021B (letzter) | Pin 11 (DS) | → VCC | CMOS-Eingänge nie floaten |
| 74HC595 (letzter) | Pin 9 (QH') | offen OK | Ausgang treibt aktiv |
| 74HC595 (alle) | Pin 10 (SRCLR) | → VCC | Clear deaktiviert |
| 74HC595 (alle) | Pin 13 (OE) | → D2 oder GND | Outputs aktiviert/PWM |
| Alle ICs | VCC/VDD | 100 nF → GND | Abblockkondensator |

### Bit-Mapping (Hardware-Verdrahtung)

**CD4021B:** BTN 1 → PI-8 (Pin 1), BTN 8 → PI-1 (Pin 7)

| Taster | CD4021B PI | Bit im Datenstrom |
|--------|------------|-------------------|
| BTN 1 | PI-8 (Pin 1) | Bit 0 |
| BTN 8 | PI-1 (Pin 7) | Bit 7 |

**Formel:** `btn_bit(id) = (id - 1) % 8`

**74HC595:** LED 1 → QA, LED 8 → QH

| LED | 74HC595 Q | Bit im Datenstrom |
|-----|-----------|-------------------|
| LED 1 | QA (Pin 15) | Bit 0 |
| LED 8 | QH (Pin 7) | Bit 7 |

**Formel:** `led_bit(id) = (id - 1) % 8`

---

## 7 Serial-Protokoll (ESP32 ↔ Pi)

115200 Baud, ASCII, Newline-terminiert (`\n`).

**Stabiler Device-Pfad:** `/dev/serial/by-id/usb-Espressif*` (nicht `/dev/ttyACM0`)

### 7.1 ESP32 → Pi

| Nachricht | Bedeutung |
|-----------|-----------|
| `READY` | Controller bereit |
| `FW SelectionPanel v2.5.2` | Firmware-Version |
| `PRESS 001` | Taster 1 gedrückt (001-100) |
| `RELEASE 001` | Taster 1 losgelassen |
| `OK` | Befehl erfolgreich |
| `PONG` | Antwort auf PING |
| `ERROR msg` | Fehlermeldung |

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
| `VERSION` | Firmware-Version |
| `HELP` | Befehlsliste |

### 7.3 STATUS-Ausgabe (v2.5.2)

```
STATUS active=5 leds=0x10
LEDS 0000100000      # Bit-Vektor (MSB links)
BTNS 1111111111      # Bit-Vektor (Active-Low: 1 = losgelassen)
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

Endpoint: `ws://rover:8080/ws`

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

### 9.1 Status-Response (v2.5.2)

```json
{
  "version": "2.5.2",
  "mode": "prototype",
  "num_media": 10,
  "current_button": 5,
  "ws_clients": 1,
  "serial_connected": true,
  "serial_port": "/dev/serial/by-id/usb-Espressif...",
  "media_missing": 0,
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
| ESP32 LED | < 1ms | Lokale Steuerung (v2.5.2) |
| Serial | ~5ms | USB-CDC Übertragung |
| Server | ~1ms | asyncio.gather |
| WebSocket | ~5ms | Netzwerk |
| Dashboard | < 50ms | Aus Cache (Preloading) |
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
| Firmware | 2.5.2 | FreeRTOS, Hardware-SPI, First-Bit-Rescue |
| Server | 2.5.2 | by-id Pfad, asyncio, ESP32_SETS_LED_LOCALLY |
| Dashboard | 2.5.1 | Preloading, Cache, Audio-Unlock |

---

## 14 Bekannte Einschränkungen

| Problem | Status | Lösung |
|---------|--------|--------|
| ESP32-S3 USB-CDC fragmentiert | ✅ Behoben | `Serial.flush()` |
| pyserial funktioniert nicht | ✅ Behoben | `os.open` + `stty` |
| CD4021B braucht längere Pulse | ✅ Behoben | 2 µs Load, 1 µs Clock |
| CD4021B First-Bit-Problem | ✅ Behoben | digitalRead() vor SPI |
| LED-Latenz durch Roundtrip | ✅ Behoben | Lokale LED-Steuerung |
| Dashboard-Latenz | ✅ Behoben | Medien-Preloading |
| Serial-Pfad instabil | ✅ Behoben | by-id Pfad verwenden |

---

*Stand: 2026-01-08 | Version 2.5.2*
