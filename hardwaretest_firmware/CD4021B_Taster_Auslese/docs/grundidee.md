# Selection Panel – Firmware-Dokumentation

## Ziel und Grundidee

Der Code bedient **zwei Schieberegister-Ketten über denselben Hardware-SPI-Bus**:

* **74HC595** (Ausgänge) → LEDs schalten
  * nutzt **MOSI + SCK**
  * übernimmt neue LED-Zustände erst bei **LATCH_595** (RCLK-Flanke)

* **CD4021** (Eingänge) → Taster einlesen
  * nutzt **MISO + SCK**
  * lädt Tasterzustände parallel bei **LOAD_4021** (P/S)

Warum das zusammen auf einem SCK funktioniert:

* Beim **Tasterlesen** takten wir zwar auch die 74HC595-Schieberegister, aber **ohne LATCH-Flanke** ändern sich deren Ausgänge nicht.
* Beim **LED-Schreiben** takten wir zwar auch die CD4021, aber vor jedem echten Lesen erfolgt wieder ein **Parallel-Load** – damit stimmt der Zustand.

---

## Hardware-Konfiguration

### Versorgungsspannung

* **Empfehlung:** Beide IC-Typen mit **3,3 V** betreiben
* Begründung: Der ESP32-S3 arbeitet mit 3,3 V Logikpegeln
* Bei 5 V Betrieb: $V_{IH}$ des CD4021B (3,5 V min) wird vom ESP32 (3,3 V max) nicht sicher erreicht
* Beide ICs sind laut Datenblatt für 3,3 V spezifiziert:
  * 74HC595: $V_{CC} = 2{-}6\,\text{V}$
  * CD4021B: $V_{DD} = 3{-}18\,\text{V}$

### Pinbelegung ESP32-S3 XIAO

| Signal      | XIAO Pin | Richtung | 74HC595         | CD4021                   |
|-------------|----------|----------|-----------------|--------------------------|
| SPI SCK     | D8       | Out      | SRCLK (Pin 11)  | CLK (Pin 10)             |
| SPI MOSI    | D10      | Out      | SER (Pin 14)    | –                        |
| SPI MISO    | D9       | In       | –               | Q8 (Pin 3) von **IC#0**  |
| LATCH_595   | D0       | Out      | RCLK (Pin 12)   | –                        |
| LOAD_4021   | D1       | Out      | –               | P/S (Pin 9)              |
| 3V3         | 3V3      | –        | VCC (Pin 16)    | VDD (Pin 16)             |
| GND         | GND      | –        | GND (Pin 8)     | VSS (Pin 8)              |

### 74HC595: Fest verdrahtete Steuerpins

Diese Pins werden im Code **nicht** angesteuert und müssen **fest beschaltet** sein:

| Pin | Signal           | Beschaltung | Begründung                              |
|----:|------------------|-------------|-----------------------------------------|
|  10 | $\overline{SCLR}$ (Clear) | → VCC (3V3) | Kein Reset nötig, Schieberegister bleibt aktiv |
|  13 | $\overline{G}$ (OE)       | → GND       | Ausgänge permanent enabled              |

**Achtung:** Ohne diese Beschaltung können die Ausgänge hochohmig sein oder sich zufällig löschen.

### Kaskadierung

* **74HC595:** QH′ (Pin 9) von IC#0 → SER (Pin 14) von IC#1 → …
* **CD4021:** Q8 (Pin 3) von IC#1 → SERIAL IN (Pin 11) von IC#0 → …
* **Wichtig:** SERIAL IN des letzten CD4021 fest auf **VDD** legen (nicht floaten)

### SPI-Konfiguration

| Parameter   | 74HC595 (Schreiben) | CD4021 (Lesen)  |
|-------------|---------------------|-----------------|
| Modus       | `SPI_MODE0`         | `SPI_MODE0`     |
| Bit-Order   | `MSBFIRST`          | `LSBFIRST`      |
| Taktrate    | 1 MHz (konservativ) | 1 MHz           |

* **SPI_MODE0** (CPOL=0, CPHA=0): Daten werden bei **steigender SCK-Flanke** übernommen – das passt zu beiden ICs laut Datenblatt.
* **MSBFIRST beim 595:** Das IC am Ende der Kette wird zuerst gesendet, damit die Bits korrekt „durchrutschen".
* **LSBFIRST beim 4021:** Damit landet Bit0 im Byte beim ersten Taster des jeweiligen ICs.

---

## Verdrahtungsregel CD4021 (entscheidend für lineare Adressierung)

Damit `Bit0 == BTN(8k+1)` gilt, muss pro CD4021 IC #k folgende Verdrahtung eingehalten werden:

| Button      | → PI-Eingang | Pin  |
|------------:|--------------|------|
| BTN(8k+1)   | PI-8         | Pin 1  |
| BTN(8k+2)   | PI-7         | Pin 15 |
| BTN(8k+3)   | PI-6         | Pin 14 |
| BTN(8k+4)   | PI-5         | Pin 13 |
| BTN(8k+5)   | PI-4         | Pin 4  |
| BTN(8k+6)   | PI-3         | Pin 5  |
| BTN(8k+7)   | PI-2         | Pin 6  |
| BTN(8k+8)   | PI-1         | Pin 7  |

**Beispiel:** BTN6 liegt an PI-3 (Pin 5), ergibt mit LSBFIRST → Bit5 im Byte (Maske `0x20`).

---

## Skalierung (10 → 100)

* `NUM_LEDS` und `NUM_BTNS` sind die einzigen „Projektgrößen".
* Die Byte-Anzahl berechnet sich automatisch:
  * `LED_BYTES = (NUM_LEDS + 7) / 8` → 10 LEDs = 2 Bytes, 100 LEDs = 13 Bytes
  * `BTN_BYTES = (NUM_BTNS + 7) / 8` → 10 Taster = 2 Bytes, 100 Taster = 13 Bytes
* `static_assert` begrenzt sinnvoll auf max. **13 ICs** (= 104 Bits).

---

## Datenmodell

### LED-Puffer

| Variable     | Typ                  | Bedeutung                                      |
|--------------|----------------------|------------------------------------------------|
| `ledBytes[]` | `uint8_t[LED_BYTES]` | Gewünschtes LED-Bitmuster (noch nicht ausgegeben) |
| `ledsDirty`  | `bool`               | `true` = Puffer hat sich geändert, muss rausgeschoben werden |

### Taster-Puffer

| Variable       | Typ                  | Bedeutung                                    |
|----------------|----------------------|----------------------------------------------|
| `btnRaw[]`     | `uint8_t[BTN_BYTES]` | Roher gelesener Zustand, logisch normiert: `1 = gedrückt` |
| `btnStable[]`  | `uint8_t[BTN_BYTES]` | Entprellter Zustand: `1 = gedrückt`          |
| `debounceCnt[]`| `uint8_t[NUM_BTNS]`  | Entprell-Zähler pro Taste                    |

**Warum „1 = gedrückt" obwohl Pull-ups active-low sind:**

* `BTN_ACTIVE_LOW = true`
* Beim Einlesen: `btnRaw[ic] = ~raw` → aus `0` (physisch gedrückt) wird logisch `1`

---

## Bit-Adressierung (1-basiert, ohne Lookup)

Wir arbeiten bewusst mit **1-basierten IDs** (`idx1 = 1…NUM`):

```cpp
getBit1(array, idx1)      // Holt Bit für Taste/LED idx1
setBit1(array, idx1, v)   // Setzt Bit für Taste/LED idx1
```

Das ist der Kern für die skalierbare, lineare Behandlung von 100 Tastern/LEDs.

Zusätzlich: `maskUnused()` löscht in der letzten Byte-Gruppe alle Bits oberhalb von `NUM_*`, damit keine „Geisterbits" entstehen.

---

## LED-Seite (74HC595)

### Prinzip

Wir ändern nur den **Puffer** (`ledBytes[]`). Erst `ledsShowIfDirty()` schiebt die Bytes über SPI raus und toggelt **LATCH**, damit die Ausgänge umspringen.

### Implementierung

**`ledsSet(ledIndex1, on)`:**

1. Berechnet Byte-Index: `ic = (idx1 - 1) / 8`
2. Berechnet Bit-Position: `bit = (idx1 - 1) % 8`
3. Setzt oder löscht das Bit (active-high üblich)
4. Setzt `ledsDirty = true`

**`ledsShowIfDirty()`:**

1. `LATCH` auf LOW (Vorbereitung)
2. SPI-Transfer **von hinten nach vorn**: `for (i = LED_BYTES; i-- > 0;)`
   * Grund: Das Byte für den IC am ESP muss **zuletzt** gesendet werden, weil es zuerst „durch die Kette" wandert.
3. `LATCH` auf HIGH → Ausgänge übernehmen gleichzeitig
4. `ledsDirty = false`

---

## Taster-Seite (CD4021)

### Schritt 1: Parallel-Load (Tasterzustand „einfrieren")

`buttonsReadRaw()` führt folgende Sequenz aus:

1. `SCK` auf LOW (definierter Ausgangszustand)
2. `LOAD_4021` auf HIGH (Parallel-Modus aktivieren)
3. Setup-Delay (`LOAD_SETUP_US`)
4. **`sckCommitPulse()`** – ein SCK-Puls außerhalb der SPI-Transaction
   * **Funktional notwendig:** Bei P/S = HIGH lädt die steigende Clock-Flanke die Parallel-Eingänge ins Schieberegister (siehe Truth Table im Datenblatt)
   * Außerhalb der SPI-Transaction, damit der Load-Zeitpunkt definiert ist
5. `LOAD_4021` auf LOW (Shift-Modus aktivieren)
6. Setup-Delay

### Schritt 2: Seriell auslesen

```cpp
SPI.beginTransaction(SPISettings(SPI_HZ, LSBFIRST, SPI_MODE0));
for (size_t ic = 0; ic < BTN_BYTES; ++ic) {
    uint8_t raw = SPI.transfer(0x00);  // Dummy MOSI, wir lesen MISO
    btnRaw[ic] = BTN_ACTIVE_LOW ? ~raw : raw;  // Normierung: 1 = gedrückt
}
SPI.endTransaction();
maskUnused(btnRaw, BTN_BYTES, NUM_BTNS);
```

* **LSBFIRST:** Das erste serielle Bit landet als Bit0 im Byte
* **Normierung:** `~raw` invertiert (active-low → logisch 1 = gedrückt)
* **maskUnused():** Entfernt undefinierte Bits oberhalb von `NUM_BTNS`

---

## Entprellen (Debounce)

### Parameter

| Konstante      | Wert   | Bedeutung                          |
|----------------|--------|------------------------------------|
| `SCAN_MS`      | 5 ms   | Abtastintervall                    |
| `DEBOUNCE_MS`  | 50 ms  | Mindest-Stabilzeit für Zustandswechsel |
| `DEBOUNCE_TICKS` | 10   | $= \lceil 50/5 \rceil$ (Integer-Ceiling via `(50+5-1)/5`) |

### Regel

Ein Zustandswechsel gilt erst als „echt", wenn `raw` über **10 aufeinanderfolgende Scans** stabil von `stable` abweicht.

### Ablauf in `buttonsDebounceAndEmit()`

Für jede Taste `i = 1…NUM_BTNS`:

1. Vergleiche `raw` mit `stable`
2. **Wenn gleich:** Zähler auf 0 zurücksetzen
3. **Wenn ungleich:** Zähler inkrementieren (bis `DEBOUNCE_TICKS`)
4. **Sobald Schwelle erreicht:**
   * `btnStable` auf `raw` setzen
   * Zähler zurücksetzen
   * `onButtonEvent(i, raw)` auslösen

Das erzeugt genau zwei Event-Typen:

* **PRESS i** (wenn stabil auf 1 wechselt)
* **RELEASE i** (wenn stabil auf 0 wechselt)

---

## Ereignislogik (Demo-Verhalten)

`onButtonEvent(id1, pressed)` implementiert aktuell:

```cpp
Serial.print(pressed ? "PRESS " : "RELEASE ");
Serial.println(id1);

if (pressed && id1 <= NUM_LEDS) {
    ledsSetOnly(id1);  // Exklusiv: nur diese LED an
}
```

**Wichtig:** Das ist **kein Toggle**.
Ein erneuter Druck auf denselben Taster lässt die LED **an** (sie war ohnehin exklusiv an). Erst ein anderer Taster schaltet um.

---

## Hauptzyklus (nicht-blockierend)

`updateIO()` ist der zentrale „Scheduler":

```cpp
inline void updateIO() {
    static uint32_t t0 = 0;
    uint32_t now = millis();

    if (now - t0 >= SCAN_MS) {
        t0 = now;
        buttonsReadRaw();
        buttonsDebounceAndEmit();
    }

    ledsShowIfDirty();
}
```

* **Alle `SCAN_MS`:** Taster lesen und entprellen
* **Jederzeit:** LEDs aktualisieren (nur wenn `ledsDirty`)

**Vorteil:** Keine blockierenden `delay()` im Loop – nur sehr kurze Mikrosekunden-Delays um Load/Latch/SCK-Puls.

---

## Setup (Initialisierungsreihenfolge)

1. **Pins konfigurieren:**
   * `LATCH_595` als Output, initial HIGH
   * `LOAD_4021` als Output, initial LOW

2. **SPI initialisieren:**

   ```cpp
   SPI.begin(PIN_SPI_SCK, PIN_SPI_MISO, PIN_SPI_MOSI);
   ```

3. **SCK als GPIO Output** (zusätzlich zu SPI):
   * Ermöglicht den manuellen „Commit-Puls" für den Parallel-Load

4. **LEDs initialisieren:**
   * `ledsClearAll()` – alle LEDs aus
   * `ledsShowIfDirty()` – einmal ausgeben

5. **Taster initialisieren:**
   * `buttonsReadRaw()` – aktuellen Zustand lesen
   * In `btnStable[]` kopieren – verhindert „Startup-Spam"
   * Der Anfangszustand gilt damit als stabil (keine Events beim Boot)

---

## Timing-Parameter (Datenblatt-Abgleich)

Alle gewählten Werte sind **konservativ** für lange Leitungen (bis ca. 1 m):

| Parameter        | Datenblatt (typ/max)    | Code-Wert | Bewertung |
|------------------|-------------------------|-----------|-----------|
| SPI-Takt         | 50 MHz (595) / 6 MHz (4021) | 1 MHz | ✅ Sehr sicher |
| SCK Pulsbreite   | 6–90 ns                 | 500 ns    | ✅ OK |
| Setup SI→SCK     | 5–10 ns                 | ~10 ns    | ✅ OK |
| Hold Time        | 0 ns                    | 0 ns      | ✅ OK |
| P/S Pulsbreite   | 80 ns typ               | 2 µs      | ✅ OK |

---

## Anpassungspunkte

| Was                | Wo                  | Typische Änderung              |
|--------------------|---------------------|--------------------------------|
| Projektgröße       | `NUM_LEDS`, `NUM_BTNS` | 10 → 100                    |
| SPI-Geschwindigkeit| `SPI_HZ`            | 0,5–4 MHz je nach Leitungslänge |
| Scan-Intervall     | `SCAN_MS`           | 2–10 ms                        |
| Entprellzeit       | `DEBOUNCE_MS`       | 20–80 ms                       |
| Event-Logik        | `onButtonEvent()`   | One-hot, Toggle, Mehrfachbelegung |

---

## Beispiel: BTN6 gedrückt → LED6 an

1. **Taster lesen:**
   * `LOAD_4021 = HIGH` → Parallel-Load vorbereiten
   * `sckCommitPulse()` → Parallel-Eingänge übernehmen
   * `LOAD_4021 = LOW` → Shift-Modus
   * SPI-Read: BTN6 (an PI-3) erscheint als **Bit5 im Byte0** (Maske `0x20`)

2. **Entprellen:**
   * 10 Scans lang stabil „gedrückt" → `onButtonEvent(6, true)`

3. **LED setzen:**
   * `ledsSetOnly(6)` → Byte0 Bit5 setzen (`0x20`), alle anderen löschen
   * `ledsDirty = true`

4. **LED ausgeben:**
   * `ledsShowIfDirty()`: `LATCH = LOW`, SPI-Write, `LATCH = HIGH`
   * LED6 leuchtet exklusiv
