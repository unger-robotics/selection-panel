# Glossar

* **JSON** = *Datenformat* (Text-Notation für Schlüssel/Wert und Listen)
* **WebSocket / UART / SPI** = *Transportwege* (Kommunikationsprotokolle für Datenübertragung)
* **Server / Raspberry Pi / ESP32-S3** = *Rollen/Computer* (wer koordiniert vs. wer I/O macht)
* **Schieberegister** = *Hardware-Trick* (Serial ↔ Parallel zur I/O-Erweiterung)

---

## WebSocket

**Regel:** WebSocket ist eine **dauerhafte TCP-Verbindung** zwischen Browser und Server. Der Server kann **sofort pushen**, ohne dass der Browser ständig “pollt” (**polling** = zyklisches Nachfragen per HTTP).

**Beispiel:** Browser verbindet sich auf `/ws`. Server sendet:

```json
{"type":"play","id":42}
```

**Anwendung bei dir:** Der Pi-Server broadcastet (**broadcast** = an alle verbundenen Clients gleichzeitig senden) das Event an alle verbundenen Browser → Bild/Audio startet ohne neue HTTP-Anfrage pro Tastendruck.

---

## JSON

**Regel:** JSON ist **Text**, der Daten als Schlüssel/Wert-Paare und Listen darstellt. Leicht im Browser (JS) und in Python zu parsen (**parsen** = Text in eine Datenstruktur umwandeln).

**Beispiel:**

```json
{"type":"PRESS","id":42,"t_ms":123456}
```

**Anwendung bei dir:** Einheitliches Format für Events (**Event** = Zustandsänderung wie „Taste gedrückt/losgelassen“) (`PRESS`, `RELEASE`, `play`, `stop`) zwischen Pi ↔ Browser (WebSocket) und ggf. Pi-intern.

---

## Server

**Regel:** Ein Server ist ein Programm, das **Anfragen annimmt** (HTTP/WebSocket) und **Antworten/Ereignisse liefert**.

**Beispiel:** `server.py` macht typischerweise:

* `GET /` → liefert `index.html`
* `GET /media/...` → liefert Datei
* `WS /ws` → hält Verbindung offen und sendet Events

**Anwendung bei dir:** Der Raspberry Pi ist der Koordinator: nimmt UART-Events vom ESP an, verwaltet Medien und verteilt “play/stop” an Browser.

---

## UART (Serial)

**Regel:** UART ist eine **asynchrone serielle** Punkt-zu-Punkt-Verbindung (TX/RX + GND). Üblich: **115200 Baud**, 8N1 (**Baud** = Symbole pro Sekunde, hier praktisch Bitrate; **8N1** = 8 Datenbits, kein Paritätsbit, 1 Stopbit).

**Zahl:** Bei 115200 Baud und 8N1 sind es grob
$$
\frac{115200,\mathrm{bit/s}}{10,\mathrm{bit/Byte}} \approx 11520,\mathrm{Byte/s}\approx 11{,}5,\mathrm{kB/s}
$$

**Anwendung bei dir:** ESP32-S3 → Pi sendet kurze Textzeilen wie `"PRESS 042\n"`. Das ist schnell genug, weil pro Event nur wenige Bytes übertragen werden.

---

## SPI (und „SPI-ähnlich“)

**Regel:** SPI ist eine **synchrone** serielle Verbindung: Master erzeugt **Clock**, Daten werden pro Takt geschoben (**synchron** = beide Seiten teilen sich den Takt).

**Beispiel (klassisch):**

* SCLK (Clock)
* MOSI (Daten zum Slave)
* ggf. MISO (Daten zurück)
* CS/Latch (Rahmen/Übernahme)

**Anwendung bei dir:** Der ESP32 taktet Bits in/aus die Schieberegister:

* **74HC595**: seriell rein → parallel raus (LEDs)
* **CD4021BE**: parallel rein → seriell raus (Taster)

Das ist „SPI-ähnlich“, weil du Clock + Data nutzt, aber zusätzlich **Latch/Load**-Signale brauchst (**Latch/Load** = Signal zum „Übernehmen/Einlesen“ eines kompletten Bitpakets).

---

## Schieberegister (74HC595 / CD4021BE)

**Regel:** Schieberegister wandeln **Serial ↔ Parallel**. Mehrere lassen sich **kaskadieren** (Q7’ → nächstes SER) (**kaskadieren** = hintereinanderschalten, sodass die Bitkette verlängert wird).

**Beispiel:**

* 13× 74HC595 → ($13\cdot 8 = 104$) LED-Ausgänge
* 13× CD4021 → ($13\cdot 8 = 104$) Taster-Eingänge

**Anwendung bei dir:**

* **74HC595 (Output):** ESP schiebt 100 Bits → dann **Latch** (**Latch** = Speicher-Übernahme: Ausgänge wechseln gleichzeitig) → alle LEDs aktualisieren “gleichzeitig”.
* **CD4021BE (Input):** ESP löst **Parallel-Load** aus (**Parallel-Load** = momentanes Einfrieren der 8 Eingänge in interne Speicherbits) → dann schiebt er 100 Bits raus und liest sie ein.

---

## Raspberry Pi

**Regel:** Ein Raspberry Pi ist ein **Linux-Single-Board-Computer**: Dateisystem, Netzwerk, HDMI, Audio, Python-Server.

**Anwendung bei dir:**

* hostet (**hosten** = Dateien/Services bereitstellen) Web-App (HTML/JS/CSS) + Medien-Dateien
* läuft `server.py` (aiohttp): WebSocket + Datei-Serving (**Serving** = Ausliefern von Dateien über HTTP)
* liest USB-Serial vom ESP32-S3
* verteilt Events an Browser und triggert Medienwiedergabe (im Browser)

---

## ESP32-S3 (Seeed XIAO)

**Regel:** ESP32-S3 ist ein **Mikrocontroller**: sehr schnell für I/O, deterministisch (**deterministisch** = Timing ist bei gleicher Last gut reproduzierbar), echtzeit-nah. Das Seeed XIAO ist das Board-Layout mit USB, Pins, Regler usw.

**Anwendung bei dir:**

* scannt Taster über CD4021 (schnell, periodisch)
* setzt LEDs über 74HC595 (schnell, latched)
* entprellt und erzeugt Events
* sendet Events über UART an den Pi

---

### 74HC595 (DIP-16)

```
        ┌──────┐
   QB ─┤1   16├─ VCC (+3,3V)
   QC ─┤2   15├─ QA (Output 0)
   QD ─┤3   14├─ SER (Data In)
   QE ─┤4   13├─ OE (→ GND)
   QF ─┤5   12├─ RCLK (Latch)
   QG ─┤6   11├─ SRCLK (Clock)
   QH ─┤7   10├─ SRCLR (→ VCC)
  GND ─┤8    9├─ QH' (Serial Out)
        └──────┘
```

| Pin | Verbindung |
|-----|------------|
| 14 (SER) | ← D0 oder vorheriger QH' |
| 11 (SRCLK) | ← D1 (Bus) |
| 12 (RCLK) | ← D2 (Bus) |
| 9 (QH') | → nächster SER oder offen |
| 10 (SRCLR) | → VCC |
| 13 (OE) | → GND |

### CD4021BE (DIP-16)

```
# alt
        ┌──────┐
   P1 ─┤1   16├─ VDD (+3,3V)
   Q6 ─┤2   15├─ P8
   Q8 ─┤3   14├─ P3
   P4 ─┤4   13├─ P2
   P5 ─┤5   12├─ Q7
   P6 ─┤6   11├─ SER (Data In)
   P7 ─┤7   10├─ CLK
  VSS ─┤8    9├─ P/S (Load)
        └──────┘
```

```
# neu
         ┌──────┐
   PI-8 ─┤1   16├─ VDD (+3,3V)
   Q6   ─┤2   15├─ PI-7
   Q8   ─┤3   14├─ PI-6
   PI-4 ─┤4   13├─ PI-5
   PI-3 ─┤5   12├─ Q7
   PI-2 ─┤6   11├─ SER (Data In)
   PI-1 ─┤7   10├─ CLK
  VSS   ─┤8    9├─ P/S (Load)
         └──────┘
```

| Pin | Verbindung |
|-----|------------|
| 3 (Q8) | → D3 oder vorheriger SER |
| 10 (CLK) | ← D4 (Bus) |
| 9 (P/S) | ← D5 (Bus) |
| 11 (SER) | ← nächster Q8 oder **→ VCC** |

**P/S = HIGH für Load** – invertiert zum 74HC165!

---

Umlöten ist sinnvoll. Der Kern ist: **CD4021 gibt seriell zuerst die Stufe Q8 aus** – die entspricht **PI-8**. Wenn du willst, dass „erstes Bit = BTN1“ ist, musst du die Taster **pro IC in der Reihenfolge PI-8, PI-7, …, PI-1** belegen.

### IC #0 (BTN1…BTN8) – pin alt → pin neu

**Ziel (neu):** BTN1 startet auf **PI-8**, dann absteigend.

| BTN | Alt (PI, Pin) | Neu (PI, Pin) |
| --: | ------------- | ------------- |
|   1 | PI-1 (7)      | **PI-8 (1)**  |
|   2 | PI-2 (6)      | **PI-7 (15)** |
|   3 | PI-3 (5)      | **PI-6 (14)** |
|   4 | PI-4 (4)      | **PI-5 (13)** |
|   5 | PI-5 (13)     | **PI-4 (4)**  |
|   6 | PI-6 (14)     | **PI-3 (5)**  |
|   7 | PI-7 (15)     | **PI-2 (6)**  |
|   8 | PI-8 (1)      | **PI-1 (7)**  |

---

### IC #1 (BTN9…BTN16) – pin alt → pin neu

(du nutzt im Prototyp nur BTN9/BTN10; der Rest ist Reserve)

| BTN | Alt (PI, Pin) | Neu (PI, Pin) |
| --: | ------------- | ------------- |
|   9 | PI-1 (7)      | **PI-8 (1)**  |
|  10 | PI-2 (6)      | **PI-7 (15)** |
|  11 | PI-3 (5)      | **PI-6 (14)** |
|  12 | PI-4 (4)      | **PI-5 (13)** |
|  13 | PI-5 (13)     | **PI-4 (4)**  |
|  14 | PI-6 (14)     | **PI-3 (5)**  |
|  15 | PI-7 (15)     | **PI-2 (6)**  |
|  16 | PI-8 (1)      | **PI-1 (7)**  |

---

## Taster-Input Mini-Decoder (CD4021, low-active)

* **BTN5 → `stream` Bit 10**
* **low-active:** gedrückt ⇒ Bit = `0`

Bei „BTN5 gedrückt“ muss **Bit 10 = 0** sein, alle anderen = 1.

**Passender `stream`-Wert (nur BTN5 gedrückt):**

* Maske für Bit 10: `0x0400`
* stream = `0xFFFF & ~0x0400 = 0xFBFF`
* Binär: `1111 1011 1111 1111` (Bit 10 ist 0)

Decoder:

```text
Bit:    15 14 13 12 11 10  9  8  7  6  5  4  3  2  1  0
Zuordn: B1 B8 B3 B2 B4 B5 B6 B7 B9  -  - B10  -  -  -  -
Wert:    1  1  1  1  1  0  1  1  1  1  1  1  1  1  1  1
=> gedrückt: BTN5
```

## LED5-Beispiel als Bytebild - 74HC595 LED-Chain (kaskadiert)

Prototyp: `NUM_LEDS = 10` → `NUM_BYTES = 2`

**Ziel:** *nur LED5 an* (High-Active: `1 = an`)

### 1) Mapping (wie im Code)

* `ledBytes[0]` = **IC #0 am ESP** = LED1..LED8  (Bits 0..7)
* `ledBytes[1]` = **IC #1**        = LED9..LED16 (Bits 0..7)

**Senden:** hinten → vorne: **erst `ledBytes[1]`, dann `ledBytes[0]`**

---

## Komplettes Beispiel „nur LED5 an“

### A) Berechnung

* LED5 → `ledIndex1 = 5`
* `idx0 = 5 - 1 = 4`
* `ic = 4 / 8 = 0`
* `bit = 4 % 8 = 4`
* `mask = 1 << 4 = 0x10`

### B) Array-Zustand

* `ledBytes[0] = 0x10`
* `ledBytes[1] = 0x00`

### C) Bytebild (binär)

```text
ledBytes[1] (IC #1, LED9..LED16):  0000 0000  (0x00)
ledBytes[0] (IC #0, LED1..LED8) :  0001 0000  (0x10)
                                   ^ Bit4 => LED5 an
```

### D) Als 16-bit Debug-Wert (nur zum Verständnis)

```text
value = (ledBytes[1] << 8) | ledBytes[0]
      = (0x00       << 8) | 0x10
      = 0x0010

0x0010 = 0000 0000 0001 0000  (b15..b0)
                     ^ b4 = 1 => LED5
```

### E) Decoder (b15..b0) für Prototyp-LED1..LED10

```text
Bit:    15 14 13 12 11 10  9   8   7   6   5   4   3   2   1   0
Zuordn:  -  -  -  -  -  - L10  L9  L8  L7  L6  L5  L4  L3  L2  L1
Wert:    0  0  0  0  0  0  0   0   0   0   0   1   0   0   0   0
=> LED an: LED5
```

### F) Was tatsächlich über SPI rausgeht (Reihenfolge!)

```text
SPI.transfer(ledBytes[1]);  // 0x00 -> wandert nach hinten (IC #1)
SPI.transfer(ledBytes[0]);  // 0x10 -> bleibt vorne (IC #0 am ESP)
LATCH HIGH -> alle Ausgänge übernehmen gleichzeitig
```

## LED100-Beispiel

100-LED-Config: `NUM_LEDS = 100` → `NUM_BYTES = 13`

**Ziel:** *nur LED100 an* (High-Active: `1 = an`)

### 0) Voraussetzung im Code

```cpp
constexpr uint16_t NUM_LEDS  = 100;
constexpr size_t   NUM_BYTES = (NUM_LEDS + 7) / 8; // 13
```

---

## 1) Mapping (wie im Code / dein Standard)

* `ledBytes[0]`  = **IC #0 am ESP** = LED1..LED8 (Bits 0..7)
* …
* `ledBytes[12]` = **IC #12 letzter IC** = LED97..LED104 (Bits 0..7)

**Senden:** hinten → vorne: **erst `ledBytes[12]`, …, zuletzt `ledBytes[0]`**

---

## 2) Berechnung für LED100

* `ledIndex1 = 100`
* `idx0 = 100 - 1 = 99`
* `ic  = 99 / 8 = 12`  → **IC #12**
* `bit = 99 % 8 = 3`   → **Bit3**
* `mask = 1 << 3 = 0x08`

---

## 3) Array-Zustand (13 Bytes)

Alle Bytes sind `0x00`, außer IC #12:

* `ledBytes[12] = 0x08`
* `ledBytes[0..11] = 0x00`

### Byte-Übersicht (komplett)

| IC/Byte-Index | LEDs auf diesem IC |            Byte (hex) |
| ------------: | ------------------ | --------------------: |
|             0 | LED1..LED8         |                `0x00` |
|             1 | LED9..LED16        |                `0x00` |
|             2 | LED17..LED24       |                `0x00` |
|             3 | LED25..LED32       |                `0x00` |
|             4 | LED33..LED40       |                `0x00` |
|             5 | LED41..LED48       |                `0x00` |
|             6 | LED49..LED56       |                `0x00` |
|             7 | LED57..LED64       |                `0x00` |
|             8 | LED65..LED72       |                `0x00` |
|             9 | LED73..LED80       |                `0x00` |
|            10 | LED81..LED88       |                `0x00` |
|            11 | LED89..LED96       |                `0x00` |
|            12 | **LED97..LED104**  | **`0x08`** *(LED100)* |

---

## 4) Bytebild (binär) für IC #12

`ledBytes[12] = 0x08` → `0000 1000` (Bit3 = 1)

```text
ledBytes[12] (IC #12, LED97..LED104): 0000 1000  (0x08)
                                     ^ Bit3 => LED100 an
```

Mini-Decoder für **IC #12**:

```text
Bit (IC#12):  7    6    5    4    3    2    1    0
LED:        L104 L103 L102 L101 L100 L99  L98  L97
Wert:         0    0    0    0    1    0    0    0
=> LED an: LED100
```

---

## 5) SPI-Transfer (tatsächlich gesendet, komplett)

```text
SPI.transfer(ledBytes[12]);  // 0x08 -> landet ganz hinten (IC #12)
SPI.transfer(ledBytes[11]);  // 0x00
...
SPI.transfer(ledBytes[1]);   // 0x00
SPI.transfer(ledBytes[0]);   // 0x00 -> bleibt vorne (IC #0 am ESP)
LATCH HIGH -> alle Ausgänge übernehmen gleichzeitig
```

---

## Tabelle: Umrechnung von 4-Bit-Binärzahlen zu Hexadezimal

| **Binär** | **Hexadezimal** | **Dezimal** |
| --------- | --------------- | ----------- |
| 0000      | 0               | 0           |
| 0001      | 1               | 1           |
| 0010      | 2               | 2           |
| 0011      | 3               | 3           |
| 0100      | 4               | 4           |
| 0101      | 5               | 5           |
| 0110      | 6               | 6           |
| 0111      | 7               | 7           |
| 1000      | 8               | 8           |
| 1001      | 9               | 9           |
| 1010      | A               | 10          |
| 1011      | B               | 11          |
| 1100      | C               | 12          |
| 1101      | D               | 13          |
| 1110      | E               | 14          |
| 1111      | F               | 15          |

## imperativ-prozedurales Paradigma

* **Imperativ/prozedural:** Zustände werden explizit verändert (`ledBytes[]`), Funktionen führen konkrete Schritte aus (`setLed()`, `showLeds()`).
* **Superloop (Run-to-completion):** `loop()` läuft endlos und erledigt pro Durchlauf nur kleine, schnelle Arbeitseinheiten; Timing über `millis()` statt `delay()`.
* **Datenorientiert/bitbasiert:** LEDs als Bitfeld (Byte-Array) + Maskenoperationen (`|`, `&`, `~`) statt vieler Einzelvariablen.
* **Kooperatives Scheduling:** Kein RTOS/Threads; alle Aufgaben teilen sich die CPU durch kurze, wiederholte Checks.

Es ist **nicht** objektorientiert im Sinne von Klassen/Objekten (auch wenn C++ genutzt wird); die Struktur ist bewusst minimal und nah an der Hardware.
