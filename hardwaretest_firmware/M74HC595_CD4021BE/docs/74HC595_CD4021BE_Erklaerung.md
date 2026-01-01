# 74HC595 + CD4021BE: Technische Dokumentation

*Eine Erklärung im Bloch-Standard – technische Exzellenz gepaart mit didaktischer Klarheit*

---

## Einführung: Warum Schieberegister?

Stellen wir uns ein konkretes Problem vor: Ein ESP32-S3 soll 100 LEDs steuern und 100 Taster einlesen. Direkt verdrahtet bräuchten wir 200 GPIO-Pins – der ESP32-S3 hat aber nur etwa 20 nutzbare. Die Lösung liegt in der **seriellen Kommunikation**: Wir wandeln parallele Signale in serielle um und umgekehrt.

Die Analogie dazu: Ein Güterzug. Statt 100 LKWs gleichzeitig durch eine enge Straße zu schicken (100 Spuren nötig), reihen wir die Container hintereinander auf einem Gleis auf. Am Ziel werden sie wieder parallel verteilt.

Zwei Bausteine lösen dieses Problem:

| Baustein | Richtung | Funktion |
|----------|----------|----------|
| **74HC595** | MCU → Außenwelt | Serial-In, Parallel-Out (SIPO) |
| **CD4021B** | Außenwelt → MCU | Parallel-In, Serial-Out (PISO) |

---

## Teil 1: Der 74HC595 – Daten hinausschreiben

### Architektur

Wenn wir das Blockdiagramm im Datenblatt betrachten, erkennen wir drei Stufen:

```
                    ┌─────────────────────────────────────────────┐
                    │              74HC595                        │
                    │                                             │
    SI ────────────►│  8-Bit          8-Bit           Tri-State  │
   (Pin 14)         │  Schieberegister → Speicherregister → Buffer │──►  QA-QH
                    │       ▲               ▲              ▲      │    (Pins 15,1-7)
    SCK ───────────►│───────┘               │              │      │
   (Pin 11)         │                       │              │      │
    RCK ───────────►│───────────────────────┘              │      │
   (Pin 12)         │                                      │      │
    G̅ ─────────────►│──────────────────────────────────────┘      │
   (Pin 13)         │                                             │
    SCLR̅ ──────────►│  (Clear für Schieberegister)                │
   (Pin 10)         │                                             │
                    │                          QH' ───────────────┼──► zum nächsten IC
                    │                         (Pin 9)             │
                    └─────────────────────────────────────────────┘
```

**Das Prinzip:** Daten werden Bit für Bit in das Schieberegister getaktet. Erst wenn alle 8 Bits drin sind, kopiert ein Latch-Impuls den gesamten Inhalt ins Speicherregister – und die Ausgänge ändern sich gleichzeitig.

### Die Signale im Detail

| Pin | Symbol | Aktiv | Funktion |
|-----|--------|-------|----------|
| 14 | SI | – | Serial Input – hier schieben wir Bit für Bit hinein |
| 11 | SCK | ↑ | Shift Clock – positive Flanke übernimmt SI |
| 12 | RCK | ↑ | Register Clock – positive Flanke kopiert zum Speicher |
| 10 | SCLR̅ | LOW | Shift Clear – LOW löscht das Schieberegister |
| 13 | G̅ | LOW | Output Enable – LOW aktiviert Ausgänge |
| 9 | QH' | – | Serial Out – für Kaskadierung |

### Timing-Sequenz: Wie schreiben wir ein Byte?

Nehmen wir an, wir wollen das Muster `0b10110001` ausgeben:

```
Zeit ──────────────────────────────────────────────────────────────────────►

SI      ─┐   ┌───┐   ┌───────┐   ┌───────────────┐   ┌───┐
         └───┘   └───┘       └───┘               └───┘   └─────────────
          1   0   1   1   0   0   0   1
              ▲       ▲       ▲       ▲       ▲       ▲       ▲       ▲
SCK     ────┐ │ ┌───┐ │ ┌───┐ │ ┌───┐ │ ┌───┐ │ ┌───┐ │ ┌───┐ │ ┌───┐ │
            └─┼─┘   └─┼─┘   └─┼─┘   └─┼─┘   └─┼─┘   └─┼─┘   └─┼─┘   └─┘
              │       │       │       │       │       │       │
              Bit7    Bit6    Bit5    Bit4    Bit3    Bit2    Bit1    Bit0
              (MSB)                                           (LSB)

RCK     ────────────────────────────────────────────────────────────┐   ┌──
                                                                    └───┘
                                                                      ▲
                                                               Alle 8 Bits
                                                               erscheinen
                                                               gleichzeitig
                                                               an QA-QH
```

**Kritisch:** Die Ausgänge ändern sich erst bei der RCK-Flanke – nicht während des Schiebens. Das verhindert "Geisterbilder" auf den LEDs.

### Kaskadierung: Mehr als 8 Ausgänge

Der QH'-Ausgang (Pin 9) liefert das "herausgeschobene" Bit. Verbinden wir QH' des ersten ICs mit SI des zweiten, entsteht eine 16-Bit-Kette:

```
ESP32                  74HC595 #1 (vorne)         74HC595 #2 (hinten)
┌─────┐               ┌─────────────┐             ┌─────────────┐
│     │──── SI ──────►│ SI      QH' │────────────►│ SI      QH' │──► (offen)
│     │──── SCK ─────►│ SCK         │─────────────┤ SCK         │
│     │──── RCK ─────►│ RCK         │─────────────┤ RCK         │
└─────┘               │ QA..QH      │             │ QA..QH      │
                      └──┬──────────┘             └──┬──────────┘
                         │                           │
                      LED 1-8                     LED 9-16
```

**Wichtig:** Wir schieben zuerst die Bits für das hintere IC, dann für das vordere. Das erste Bit "rutscht" durch bis zum Ende der Kette.

---

## Teil 2: Der CD4021B – Daten hereinlesen

### Architektur

Der CD4021B arbeitet spiegelverkehrt: Er liest 8 parallele Eingänge gleichzeitig ein und schiebt sie seriell heraus.

```
                    ┌─────────────────────────────────────────────┐
                    │              CD4021B                        │
                    │                                             │
 P1-P8 ────────────►│  8× Parallel     8-Bit                     │
(Pins 7,6,5,4,      │  Latches    →    Schieberegister           │──► Q8
 13,14,15,1)        │     ▲                  ▲         ▲         │   (Pin 3)
                    │     │                  │         │         │
  P/S ─────────────►│─────┴──────────────────┤         │         │
 (Pin 9)            │                        │         │         │
  CLK ─────────────►│────────────────────────┴─────────┘         │
 (Pin 10)           │                                             │
   SI ─────────────►│  (Serial Input für Kaskadierung)           │
 (Pin 11)           │                                             │
                    └─────────────────────────────────────────────┘
```

### Die zwei Betriebsmodi

Der P/S-Pin (Parallel/Serial) steuert das Verhalten:

| P/S | Modus | Was passiert bei CLK ↑ |
|-----|-------|------------------------|
| HIGH | Parallel Load | Alle 8 Eingänge werden in die Latches übernommen |
| LOW | Serial Shift | Daten werden um 1 Position geschoben |

### Timing-Sequenz: Wie lesen wir 8 Taster?

```
Zeit ──────────────────────────────────────────────────────────────────────►

P/S     ────┐                                                          ┌────
            └──────────────────────────────────────────────────────────┘
            HIGH (Load)           LOW (Shift)

CLK     ──────┐   ┌───────┐   ┌───────┐   ┌───────┐   ┌───────┐   ┌───────┐
              └───┘       └───┘       └───┘       └───┘       └───┘       └──
                ▲           ▲           ▲           ▲           ▲
                │           │           │           │           │
              Load!      Shift       Shift       Shift       Shift
                          Bit7        Bit6        Bit5        ...

Q8      ──────────┐       ┌───┐       ┌───────────────────────────────────
        (vorher)  └───────┘   └───────┘
                    Bit8      Bit7      Bit6 ...
                    ▲         ▲         ▲
                    │         │         │
                  Lesen     Lesen     Lesen
```

**Kritisch:** Nach dem Load-Puls liegt Q8 **sofort** am Ausgang – noch vor dem ersten Clock. Die korrekte Sequenz ist:

1. P/S = HIGH → CLK ↑ (Load)
2. P/S = LOW
3. **Q8 lesen** (erstes Bit!)
4. CLK ↑ (Shift)
5. Q8 lesen (zweites Bit)
6. ... wiederholen

---

## Teil 3: Der Code im Detail

### Konfiguration und Pin-Definitionen

```cpp
// =============================================================================
// Konfiguration (ein Ort für Skalierung / Anpassungen)
// =============================================================================
constexpr uint8_t NUM_LEDS = 10;
constexpr uint8_t NUM_BTNS = 10;
constexpr uint8_t SHIFT_BITS = 16;   // 2× 8-bit Shiftregister in Kaskade
constexpr uint32_t DEBOUNCE_MS = 30; // Entprellen (stabiler Zustand)
constexpr uint32_t POLL_MS = 1;      // Begrenze Poll-Rate (CPU-Last/Jitter)
```

**Warum `constexpr`?** Diese Werte werden zur Compile-Zeit ausgewertet – kein RAM-Verbrauch, kein Laufzeit-Overhead. Bei Skalierung auf 100× ändern wir nur diese Zeilen.

### LED-Ausgabe: Der 74HC595-Treiber

```cpp
// Einziger HW-Zugriff für LEDs: schreibt 2 Bytes (Kaskade)
inline void write595(uint8_t u3, uint8_t u2) {
    digitalWrite(LED_LATCH_PIN, LOW);                    // Latch sperren
    shiftOut(LED_DATA_PIN, LED_CLOCK_PIN, MSBFIRST, u3); // IC hinten
    shiftOut(LED_DATA_PIN, LED_CLOCK_PIN, MSBFIRST, u2); // IC vorne
    digitalWrite(LED_LATCH_PIN, HIGH);                   // Latch freigeben
}
```

**Reihenfolge erklärt:** Das zuerst gesendete Byte (u3) "rutscht" durch das vordere IC hindurch ins hintere. Das zweite Byte (u2) bleibt im vorderen IC. Deshalb: hinten zuerst, vorne zuletzt.

```
Schiebe-Reihenfolge:

         u3 (Byte für IC#2)        u2 (Byte für IC#1)
         ┌───────────────┐         ┌───────────────┐
ESP32 ──►│ b7 b6 ... b0  │────────►│ b7 b6 ... b0  │
         └───────────────┘         └───────────────┘
         "rutscht durch"           "bleibt hier"
              │                          │
              ▼                          ▼
         LED 9-16                    LED 1-8
```

### Taster-Einlesen: Der CD4021B-Treiber

```cpp
inline uint16_t read4021Stream16() {
    // 1) Parallel Load: P/S HIGH übernimmt alle Eingänge in die Latches
    digitalWrite(BTN_LOAD_PIN, HIGH);
    delayMicroseconds(2);  // t_WH: Load-Puls (min 160 ns @ 5V)
    digitalWrite(BTN_LOAD_PIN, LOW);
    delayMicroseconds(2);  // t_REM: Warten bis Q8 stabil

    // 2) Serial Shift: MSB-first (erstes Bit = Bit 15)
    uint16_t v = 0;
    for (uint8_t i = 0; i < SHIFT_BITS; i++) {
        // ERST lesen (Q8 enthält aktuelles Bit)
        v <<= 1;
        v |= (digitalRead(BTN_DATA_PIN) ? 1u : 0u);

        // DANN clocken (schiebt nächstes Bit nach Q8)
        if (i < (SHIFT_BITS - 1)) {
            digitalWrite(BTN_CLOCK_PIN, HIGH);
            delayMicroseconds(1);
            digitalWrite(BTN_CLOCK_PIN, LOW);
            delayMicroseconds(1);
        }
    }
    return v;
}
```

**Die kritische Stelle:** Wir lesen **vor** dem Clock-Puls, nicht danach. Nach dem Parallel Load liegt das erste Bit bereits an Q8. Der Clock schiebt das nächste Bit herbei.

```
Schleifenablauf (vereinfacht):

i=0:  Lesen → v = 0b0000000000000001 (Bit von P8)
      Clock → P7 kommt nach Q8

i=1:  v <<= 1 → v = 0b0000000000000010
      Lesen → v |= bit → v = 0b0000000000000011 (falls P7=1)
      Clock → P6 kommt nach Q8

...und so weiter
```

### Das Bit-Mapping: Hardware-Abstraktion

```cpp
constexpr uint8_t BTN_BIT_MAP[NUM_BTNS] = {
    15, // BTN1  -> Bit 15 (IC#1, Q8)
    12, // BTN2  -> Bit 12 (IC#1, Q5)
    13, // BTN3  -> Bit 13 (IC#1, Q6)
    // ...
};
```

**Warum ein Mapping?** Die physische Verdrahtung entspricht selten der logischen Nummerierung. Statt im Code zu rechnen, definieren wir eine Lookup-Tabelle. Bei Verdrahtungsänderungen passen wir nur diese Tabelle an – der Rest des Codes bleibt unberührt.

```cpp
inline uint16_t readButtonsPressedMask() {
    const uint16_t stream = read4021Stream16();

    uint16_t pressed = 0;
    for (uint8_t i = 0; i < NUM_BTNS; i++) {
        const uint8_t sb = BTN_BIT_MAP[i];              // Physische Position
        const bool level = ((stream >> sb) & 1u) != 0;  // Bit extrahieren
        const bool isPressed = BTN_ACTIVE_LOW ? (!level) : level;
        if (isPressed)
            pressed |= (static_cast<uint16_t>(1u) << i); // Logische Position
    }
    return pressed;
}
```

**Das Ergebnis:** Eine saubere Maske, wo Bit 0 = BTN1, Bit 1 = BTN2, usw. Die Hardware-Komplexität ist gekapselt.

### Debouncing: Prellen eliminieren

Mechanische Taster prellen – sie schalten beim Drücken mehrfach ein und aus:

```
Reales Signal beim Drücken:

Taster  ────┐ ┌─┐ ┌─┐ ┌───────────────────────────────────
            └─┘ └─┘ └─┘
            │←──────────►│
             Prellphase
             (~5-20 ms)
```

Die Lösung: Wir warten, bis der Zustand **stabil** bleibt.

```cpp
inline void buttonsTask(uint32_t nowMs) {
    const uint16_t raw = readButtonsPressedMask();

    // Rohänderung: Zeitpunkt merken
    if (raw != g_btnRaw) {
        g_btnRaw = raw;
        g_btnChangedAt = nowMs;  // Timer neu starten
    }

    // Wenn lange genug stabil -> übernehmen
    if ((nowMs - g_btnChangedAt) >= DEBOUNCE_MS && g_btnStable != g_btnRaw) {
        const uint16_t prev = g_btnStable;
        g_btnStable = g_btnRaw;

        // Rising Edge: 0 → 1 (Taste wurde gedrückt)
        const uint16_t rising = (g_btnStable & ~prev);
        // ...
    }
}
```

**Flankenerkennung:** Wir reagieren nur auf den Übergang 0→1, nicht auf den dauerhaften Zustand. Sonst würde die LED bei gehaltenem Taster ständig toggeln.

---

## Teil 4: Timing-Analyse

### Sind unsere Delays ausreichend?

Aus dem Datenblatt (CD4021B bei 5 V):

| Parameter | Symbol | Min (ns) | Unser Delay |
|-----------|--------|----------|-------------|
| P/S Pulse Width | t_WH | 80 | 2000 ns (2 µs) ✓ |
| P/S Removal Time | t_REM | 140 | 2000 ns ✓ |
| Clock Pulse Width | t_W | 40 | 1000 ns ✓ |

Bei 3,3 V (statt 5 V) verlängern sich alle Zeiten um etwa Faktor 2. Unsere Delays haben einen Sicherheitsfaktor von ~25× – mehr als ausreichend.

### Gesamtzeit für einen Durchlauf

```
Parallel Load:     2 µs + 2 µs = 4 µs
16× Lesen+Clock:   16 × (digitalRead + 2 µs) ≈ 16 × 3 µs = 48 µs
─────────────────────────────────────────────────────
Summe:             ~52 µs pro Scan
```

Bei POLL_MS = 1 ms haben wir ~950 µs "Luft" – der ESP32 langweilt sich.

---

## Teil 5: Skalierung auf 100×

Für 100 LEDs und 100 Taster brauchen wir 13 ICs je Typ (13 × 8 = 104 ≥ 100).

### Änderungen im Code

```cpp
// Vorher (10×)
constexpr uint8_t NUM_LEDS = 10;
constexpr uint8_t SHIFT_BITS = 16;
static uint16_t g_ledState = 0;

// Nachher (100×)
constexpr uint8_t NUM_LEDS = 100;
constexpr uint8_t NUM_SHIFT_BYTES = 13;  // Aufrunden: (100+7)/8
static uint8_t g_ledState[NUM_SHIFT_BYTES] = {0};
```

### Optimierung: Hardware-SPI

Bei 100× wird Bit-Banging zum Flaschenhals. Der ESP32-S3 hat Hardware-SPI:

```cpp
#include <SPI.h>

void applyLeds() {
    SPI.beginTransaction(SPISettings(4000000, MSBFIRST, SPI_MODE0));
    digitalWrite(LED_LATCH_PIN, LOW);
    for (int8_t i = NUM_SHIFT_BYTES - 1; i >= 0; i--) {
        SPI.transfer(g_ledState[i]);
    }
    digitalWrite(LED_LATCH_PIN, HIGH);
    SPI.endTransaction();
}
```

**Geschwindigkeitsvergleich:**

| Methode | 16 Bits | 104 Bits |
|---------|---------|----------|
| shiftOut() | ~64 µs | ~416 µs |
| Hardware-SPI @ 4 MHz | ~4 µs | ~26 µs |

Faktor 16× schneller – relevant für flimmerfreie LED-Ansteuerung.

---

## Zusammenfassung

Die Kombination aus 74HC595 (Ausgänge) und CD4021B (Eingänge) löst das Pin-Multiplex-Problem elegant:

| Aspekt | 10× (aktuell) | 100× (Ziel) |
|--------|---------------|-------------|
| GPIO-Pins | 6 | 6 |
| ICs | 4 | 26 |
| Scan-Zeit | ~100 µs | ~500 µs |
| Code-Änderungen | – | Konstanten + Array-Typen |

Das Grundprinzip bleibt identisch – nur die Skalierung ändert sich. Genau das macht gutes Hardware-Abstraktions-Design aus.

---

## Referenzen

- SGS-Thomson: *M54/74HC595 Datasheet* – 8-Bit Shift Register with Output Latches
- Texas Instruments: *CD4021B Datasheet* – CMOS 8-Stage Static Shift Register
- Seeed Studio: *XIAO ESP32S3 Wiki* – Pinout und Spezifikationen
