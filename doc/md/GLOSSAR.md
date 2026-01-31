# Glossar

Version 2.5.2 | Stand: 2026-01-08

* **JSON** = *Datenformat* (Text-Notation für Schlüssel/Wert und Listen)
* **WebSocket / UART / SPI** = *Transportwege* (Kommunikationsprotokolle für Datenübertragung)
* **Server / Raspberry Pi / ESP32-S3** = *Rollen/Computer* (wer koordiniert vs. wer I/O macht)
* **Schieberegister** = *Hardware-Trick* (Serial ↔ Parallel zur I/O-Erweiterung)

---

## WebSocket

**Regel:** WebSocket ist eine **dauerhafte TCP-Verbindung** zwischen Browser und Server. Der Server kann **sofort pushen**, ohne dass der Browser ständig "pollt" (**polling** = zyklisches Nachfragen per HTTP).

**Beispiel:** Browser verbindet sich auf `/ws`. Server sendet:

```json
{"type":"play","id":42}
```

**Anwendung:** Der Pi-Server broadcastet (**broadcast** = an alle verbundenen Clients gleichzeitig senden) das Event an alle verbundenen Browser → Bild/Audio startet ohne neue HTTP-Anfrage pro Tastendruck.

---

## JSON

**Regel:** JSON ist **Text**, der Daten als Schlüssel/Wert-Paare und Listen darstellt. Leicht im Browser (JS) und in Python zu parsen (**parsen** = Text in eine Datenstruktur umwandeln).

**Beispiel:**

```json
{"type":"PRESS","id":42,"t_ms":123456}
```

**Anwendung:** Einheitliches Format für Events (**Event** = Zustandsänderung wie "Taste gedrückt/losgelassen") (`PRESS`, `RELEASE`, `play`, `stop`) zwischen Pi ↔ Browser (WebSocket) und ggf. Pi-intern.

---

## Server

**Regel:** Ein Server ist ein Programm, das **Anfragen annimmt** (HTTP/WebSocket) und **Antworten/Ereignisse liefert**.

**Beispiel:** `server.py` macht typischerweise:

* `GET /` → liefert `index.html`
* `GET /media/...` → liefert Datei
* `WS /ws` → hält Verbindung offen und sendet Events

**Anwendung:** Der Raspberry Pi ist der Koordinator: nimmt UART-Events vom ESP an, verwaltet Medien und verteilt "play/stop" an Browser.

---

## UART (Serial)

**Regel:** UART ist eine **asynchrone serielle** Punkt-zu-Punkt-Verbindung (TX/RX + GND). Üblich: **115200 Baud**, 8N1 (**Baud** = Symbole pro Sekunde, hier praktisch Bitrate; **8N1** = 8 Datenbits, kein Paritätsbit, 1 Stopbit).

**Zahl:** Bei 115200 Baud und 8N1 sind es grob:

$$
\frac{115200\,\mathrm{bit/s}}{10\,\mathrm{bit/Byte}} \approx 11520\,\mathrm{Byte/s} \approx 11{,}5\,\mathrm{kB/s}
$$

**Anwendung:** ESP32-S3 → Pi sendet kurze Textzeilen wie `"PRESS 042\n"`. Das ist schnell genug, weil pro Event nur wenige Bytes übertragen werden.

---

## SPI (und "SPI-ähnlich")

**Regel:** SPI ist eine **synchrone** serielle Verbindung: Master erzeugt **Clock**, Daten werden pro Takt geschoben (**synchron** = beide Seiten teilen sich den Takt).

**Beispiel (klassisch):**

* SCLK (Clock)
* MOSI (Daten zum Slave)
* ggf. MISO (Daten zurück)
* CS/Latch (Rahmen/Übernahme)

**Anwendung:** Der ESP32 taktet Bits in/aus die Schieberegister:

* **74HC595**: seriell rein → parallel raus (LEDs)
* **CD4021B**: parallel rein → seriell raus (Taster)

Das ist "SPI-ähnlich", weil du Clock + Data nutzt, aber zusätzlich **Latch/Load**-Signale brauchst (**Latch/Load** = Signal zum "Übernehmen/Einlesen" eines kompletten Bitpakets).

---

## Schieberegister (74HC595 / CD4021B)

**Regel:** Schieberegister wandeln **Serial ↔ Parallel**. Mehrere lassen sich **kaskadieren** (QH' → nächstes SER) (**kaskadieren** = hintereinanderschalten, sodass die Bitkette verlängert wird).

**Beispiel:**

* 13× 74HC595 → (13·8 = 104) LED-Ausgänge
* 13× CD4021B → (13·8 = 104) Taster-Eingänge

**Anwendung:**

* **74HC595 (Output):** ESP schiebt 100 Bits → dann **Latch** (**Latch** = Speicher-Übernahme: Ausgänge wechseln gleichzeitig) → alle LEDs aktualisieren "gleichzeitig".
* **CD4021B (Input):** ESP löst **Parallel-Load** aus (**Parallel-Load** = momentanes Einfrieren der 8 Eingänge in interne Speicherbits) → dann schiebt er 100 Bits raus und liest sie ein.

---

## Raspberry Pi

**Regel:** Ein Raspberry Pi ist ein **Linux-Single-Board-Computer**: Dateisystem, Netzwerk, HDMI, Audio, Python-Server.

**Anwendung:**

* hostet (**hosten** = Dateien/Services bereitstellen) Web-App (HTML/JS/CSS) + Medien-Dateien
* läuft `server.py` (aiohttp): WebSocket + Datei-Serving (**Serving** = Ausliefern von Dateien über HTTP)
* liest USB-Serial vom ESP32-S3
* verteilt Events an Browser und triggert Medienwiedergabe (im Browser)

---

## ESP32-S3 (Seeed XIAO)

**Regel:** ESP32-S3 ist ein **Mikrocontroller**: sehr schnell für I/O, deterministisch (**deterministisch** = Timing ist bei gleicher Last gut reproduzierbar), echtzeit-nah. Das Seeed XIAO ist das Board-Layout mit USB, Pins, Regler usw.

**Anwendung:**

* scannt Taster über CD4021B (schnell, periodisch)
* setzt LEDs über 74HC595 (schnell, latched)
* entprellt und erzeugt Events
* sendet Events über UART an den Pi
* **FreeRTOS Dual-Core:** io_task auf Core 1, serial_task auf Core 1

---

## 74HC595 (DIP-16) – LED-Ausgabe

```
        ┌──────┐
   QB ─┤1   16├─ VCC (+3,3V)
   QC ─┤2   15├─ QA (Output 0)
   QD ─┤3   14├─ SER (Data In)     ← D10 (MOSI)
   QE ─┤4   13├─ OE (→ GND/PWM)    ← D2
   QF ─┤5   12├─ RCLK (Latch)      ← D0
   QG ─┤6   11├─ SRCLK (Clock)     ← D8 (SCK)
   QH ─┤7   10├─ SRCLR (→ VCC)
  GND ─┤8    9├─ QH' (Serial Out)
        └──────┘
```

| Pin | Name | Verbindung |
|-----|------|------------|
| 14 | SER | ← D10 (MOSI) oder vorheriger QH' |
| 11 | SRCLK | ← D8 (SCK, shared) |
| 12 | RCLK | ← D0 (Latch) |
| 9 | QH' | → nächster SER oder offen |
| 10 | SRCLR | → VCC (nicht löschen) |
| 13 | OE | ← D2 (PWM für Helligkeit) oder → GND |

**SPI-Mode:** MODE0 (CPOL=0, CPHA=0) @ 1 MHz

---

## CD4021B (DIP-16) – Taster-Eingabe

```
         ┌──────┐
   PI-8 ─┤1   16├─ VDD (+3,3V)
   Q6   ─┤2   15├─ PI-7
   Q8   ─┤3   14├─ PI-6            Q8 → D9 (MISO)
   PI-4 ─┤4   13├─ PI-5
   PI-3 ─┤5   12├─ Q7
   PI-2 ─┤6   11├─ DS (Serial In)  ← VCC (letzter) oder vorheriger Q8
   PI-1 ─┤7   10├─ CLK             ← D8 (SCK, shared)
  VSS   ─┤8    9├─ P/S (Load)      ← D1
         └──────┘
```

| Pin | Name | Verbindung |
|-----|------|------------|
| 3 | Q8 | → D9 (MISO) oder nächster DS |
| 10 | CLK | ← D8 (SCK, shared) |
| 9 | P/S | ← D1 (Load-Signal) |
| 11 | DS | ← **VCC** (letzter IC!) oder vorheriger Q8 |

**P/S = HIGH für Load** – invertiert zum 74HC165!

**SPI-Mode:** MODE1 (CPOL=0, CPHA=1) @ 500 kHz

---

## Hardware-Verdrahtung und Bit-Mapping

### Button-Verdrahtung (CD4021B)

Die Taster sind so verdrahtet, dass BTN 1 an PI-8 (Pin 1) liegt:

| Taster | CD4021B Pin | PI-Eingang | Bit im Datenstrom |
|--------|-------------|------------|-------------------|
| BTN 1 | Pin 1 | PI-8 | Bit 0 |
| BTN 2 | Pin 15 | PI-7 | Bit 1 |
| BTN 3 | Pin 14 | PI-6 | Bit 2 |
| BTN 4 | Pin 13 | PI-5 | Bit 3 |
| BTN 5 | Pin 4 | PI-4 | Bit 4 |
| BTN 6 | Pin 5 | PI-3 | Bit 5 |
| BTN 7 | Pin 6 | PI-2 | Bit 6 |
| BTN 8 | Pin 7 | PI-1 | Bit 7 |

**Formel:** `btn_bit(id) = (id - 1) % 8`

### LED-Verdrahtung (74HC595)

| LED | 74HC595 Pin | Ausgang | Bit im Datenstrom |
|-----|-------------|---------|-------------------|
| LED 1 | Pin 15 | QA | Bit 0 |
| LED 2 | Pin 1 | QB | Bit 1 |
| LED 3 | Pin 2 | QC | Bit 2 |
| LED 4 | Pin 3 | QD | Bit 3 |
| LED 5 | Pin 4 | QE | Bit 4 |
| LED 6 | Pin 5 | QF | Bit 5 |
| LED 7 | Pin 6 | QG | Bit 6 |
| LED 8 | Pin 7 | QH | Bit 7 |

**Formel:** `led_bit(id) = (id - 1) % 8`

---

## Taster-Input Decoder (CD4021B, Active-Low)

**Active-Low:** gedrückt ⇒ Bit = `0` (Pull-up Widerstand 10kΩ)

**Beispiel: BTN 5 gedrückt**

```text
Bit:    7    6    5    4    3    2    1    0
BTN:    8    7    6    5    4    3    2    1
Wert:   1    1    1    0    1    1    1    1
                     ↑
               BTN 5 gedrückt (Bit 4 = 0)
```

**Hex:** `0xEF` (1110 1111)

**Code:**

```cpp
bool isPressed = !((stream >> btn_bit(5)) & 1);  // btn_bit(5) = 4
```

---

## LED-Output Decoder (74HC595, Active-High)

**Active-High:** an ⇒ Bit = `1`

**Beispiel: LED 5 an**

```text
Bit:    7    6    5    4    3    2    1    0
LED:    8    7    6    5    4    3    2    1
Wert:   0    0    0    1    0    0    0    0
                     ↑
                LED 5 an (Bit 4 = 1)
```

**Hex:** `0x10` (0001 0000)

**Code:**

```cpp
led_set(ledBytes, 5, true);  // setzt Bit 4
```

---

## LED100-Beispiel (Skalierung)

100-LED-Config: `NUM_LEDS = 100` → `NUM_BYTES = 13`

**Ziel:** *nur LED100 an*

### Berechnung

* `idx0 = 100 - 1 = 99`
* `ic = 99 / 8 = 12` → **IC #12** (letzter in der Kette)
* `bit = 99 % 8 = 3` → **Bit 3**
* `mask = 1 << 3 = 0x08`

### Array-Zustand

```cpp
ledBytes[12] = 0x08;  // LED100 an
ledBytes[0..11] = 0x00;
```

### SPI-Transfer (Reihenfolge!)

```cpp
// Hinten zuerst, vorne zuletzt
for (int i = NUM_BYTES - 1; i >= 0; i--) {
    SPI.transfer(ledBytes[i]);
}
digitalWrite(PIN_LED_RCK, HIGH);  // Latch
digitalWrite(PIN_LED_RCK, LOW);
```

---

## Binär-Hex-Tabelle

| Binär | Hex | Dezimal |
|-------|-----|---------|
| 0000 | 0 | 0 |
| 0001 | 1 | 1 |
| 0010 | 2 | 2 |
| 0011 | 3 | 3 |
| 0100 | 4 | 4 |
| 0101 | 5 | 5 |
| 0110 | 6 | 6 |
| 0111 | 7 | 7 |
| 1000 | 8 | 8 |
| 1001 | 9 | 9 |
| 1010 | A | 10 |
| 1011 | B | 11 |
| 1100 | C | 12 |
| 1101 | D | 13 |
| 1110 | E | 14 |
| 1111 | F | 15 |

---

## Firmware-Architektur (FreeRTOS)

Die Firmware verwendet **FreeRTOS Dual-Core** auf dem ESP32-S3:

| Task | Core | Priorität | Periode | Funktion |
|------|------|-----------|---------|----------|
| io_task | 1 | 5 | 5 ms | Hardware-I/O (Taster, LEDs) |
| serial_task | 1 | 2 | Event-driven | Protokoll-Handler |

**Design-Prinzipien:**

* **Queue-basiert:** io_task sendet LogEvents über FreeRTOS-Queue an serial_task
* **Mutex-geschützt:** SPI-Bus wird durch Mutex vor gleichzeitigem Zugriff geschützt
* **RAII:** SpiGuard für automatisches Cleanup
* **Zeitbasiertes Debouncing:** Jeder Taster hat eigenen Timer (30 ms)
* **Bitfeld-basiert:** LEDs/Taster als Byte-Arrays mit Maskenoperationen

Core 0 bleibt für WiFi/BLE reserviert (falls später benötigt).

---

*Stand: 2026-01-08 | Version 2.5.2*
