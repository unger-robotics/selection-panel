# 74HC595 + CD4021B: Technische Dokumentation

**Version:** 2.0.0

---

## Einführung: Warum Schieberegister?

Stellen wir uns ein konkretes Problem vor: Ein ESP32-S3 soll 100 LEDs steuern und 100 Taster einlesen. Direkt verdrahtet bräuchten wir 200 GPIO-Pins – der ESP32-S3 hat aber nur etwa 11 nutzbare. Die Lösung liegt in der **seriellen Kommunikation**: Wir wandeln parallele Signale in serielle um und umgekehrt.

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
   (Pin 13)         │              (PWM für Helligkeit)           │
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
| 13 | G̅ | LOW | Output Enable – **PWM für Helligkeitssteuerung** |
| 9 | QH' | – | Serial Out – für Kaskadierung |

### Hardware-PWM über OE (G̅)

Der OE-Pin ermöglicht eine elegante Helligkeitssteuerung:

```
OE = LOW  → Ausgänge aktiv (LEDs an)
OE = HIGH → Ausgänge High-Z (LEDs aus)

PWM-Signal an OE:
    ┌──┐  ┌──┐  ┌──┐  ┌──┐
OE ─┘  └──┘  └──┘  └──┘  └──
    50%    50%    50%    50%
         Duty Cycle

LED-Helligkeit = 100% - Duty Cycle (Active-LOW!)
```

**Vorteile gegenüber Software-PWM:**

- CPU-Last ~0% (LEDC läuft im Hintergrund)
- Flimmerfrei (konstante Frequenz)
- Alle LEDs gleichzeitig gedimmt

### Kaskadierung: Mehr als 8 Ausgänge

Der QH'-Ausgang (Pin 9) liefert das "herausgeschobene" Bit. Verbinden wir QH' des ersten ICs mit SI des zweiten, entsteht eine 16-Bit-Kette:

```
ESP32                  74HC595 #0              74HC595 #1
┌─────┐               ┌─────────────┐          ┌─────────────┐
│ D10 │──── SI ──────►│ SI      QH' │─────────►│ SI      QH' │──► (offen)
│ D8  │──── SCK ─────►│ SCK         │──────────┤ SCK         │
│ D0  │──── RCK ─────►│ RCK         │──────────┤ RCK         │
│ D2  │──── OE ──────►│ OE          │──────────┤ OE          │  (PWM)
└─────┘               │ QA..QH      │          │ QA..QH      │
                      └──┬──────────┘          └──┬──────────┘
                         │                        │
                      LED 1-8                  LED 9-16
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

| P/S | Modus | Was passiert |
|-----|-------|--------------|
| HIGH | Parallel Load | Alle 8 Eingänge werden übernommen |
| LOW | Serial Shift | CLK ↑ schiebt Daten um 1 Position |

### Timing-Sequenz: Die kritische Erkenntnis

Nach dem Parallel Load liegt Q8 **sofort** am Ausgang – noch vor dem ersten Clock:

```
Zeit ──────────────────────────────────────────────────────────────►

P/S     ────┐           ┌──────────────────────────────────────────
            └───────────┘
            HIGH        LOW (Shift-Modus)
              ↓
           Load!

CLK     ────────────────────┐   ┌───┐   ┌───┐   ┌───┐   ┌───┐
                            └───┘   └───┘   └───┘   └───┘   └───
                              ↑       ↑       ↑       ↑
                            Shift   Shift   Shift   Shift

Q8      ────────────────┬───────┬───────┬───────┬───────┬────────
                        │ Bit7  │ Bit6  │ Bit5  │ Bit4  │ ...
                        ↑       ↑       ↑       ↑       ↑
                      LESEN   LESEN   LESEN   LESEN   LESEN
                      (vor    (nach   (nach   (nach   (nach
                      Clock!)  Clock)  Clock)  Clock)  Clock)
```

**Warum kein Hardware-SPI für CD4021B?**

SPI sendet immer zuerst einen Clock, dann wird gelesen:

```
SPI-Sequenz:      Clock → Lesen → Clock → Lesen → ...
CD4021B braucht:  Lesen → Clock → Lesen → Clock → ...
                    ↑
              Erstes Bit liegt SOFORT nach Load an!
```

Hardware-SPI würde das erste Bit verpassen. Deshalb bleibt CD4021B bei **Bit-Banging**.

---

## Teil 3: Shared Clock – Elegante Vereinfachung

In v2.0 teilen sich beide IC-Typen die Clock-Leitung:

```
ESP32-S3 (XIAO)
┌─────────────────┐
│                 │
│  D8 (SCK) ──────┼────────┬──────────────┬─────────────────────
│                 │        │              │
│  D10 (MOSI) ────┼────────│──────────────│──► 74HC595 SI
│                 │        │              │
│  D9 (MISO) ◄────┼────────│──────────────│─── CD4021B Q8
│                 │        │              │
│  D0 (RCK) ──────┼────────│──► 74HC595   │
│                 │        │    Latch     │
│  D1 (P/S) ──────┼────────│──────────────│──► CD4021B
│                 │        │              │    Load/Shift
│  D2 (OE) ───────┼────────│──► 74HC595   │
│                 │        │    PWM       │
└─────────────────┘        │              │
                           ▼              ▼
                      74HC595 SCK    CD4021B CLK
```

**Warum funktioniert das?**

Die Latch-Pins (RCK, P/S) steuern, welcher IC auf die Clock reagiert:

- Beim LED-Schreiben: RCK übernimmt die Daten
- Beim Taster-Lesen: P/S steuert Load/Shift

Die Clock-Pulse "stören" den jeweils anderen IC nicht, solange sein Latch-Pin inaktiv ist.

---

## Teil 4: Der Code im Detail

### Konfiguration – Eine Konstante steuert alles

```cpp
// =============================================================================
// KONFIGURATION – Nur hier anpassen
// =============================================================================
constexpr size_t BTN_COUNT = 10;  // 10 für Prototyp, 100 für Produktion
constexpr size_t LED_COUNT = 10;
constexpr uint8_t PWM_DUTY = 50;  // 0-100%

// Abgeleitete Konstanten (automatisch berechnet)
constexpr size_t BTN_BYTES = (BTN_COUNT + 7) / 8;  // 2 für 10, 13 für 100
constexpr size_t LED_BYTES = (LED_COUNT + 7) / 8;
```

**Das Prinzip:** Ändern wir `BTN_COUNT` auf 100, berechnet der Compiler automatisch `BTN_BYTES = 13`. Keine weiteren Anpassungen nötig.

### CD4021B Lesen (Bit-Banging)

```cpp
static void readButtons() {
    // Parallel-Load
    digitalWrite(PIN_BTN_PS, HIGH);
    delayMicroseconds(5);
    digitalWrite(PIN_BTN_PS, LOW);
    delayMicroseconds(2);

    // Bit-Banging: Q8 liegt bereits am Ausgang!
    for (size_t ic = 0; ic < BTN_BYTES; ++ic) {
        uint8_t byte = 0;
        for (int8_t bit = 7; bit >= 0; --bit) {
            // ERST lesen
            if (digitalRead(PIN_BTN_MISO)) {
                byte |= (1 << bit);
            }
            // DANN clocken
            clockPulse();
        }
        btnState[ic] = byte;
    }
}
```

**Die kritische Reihenfolge:** Lesen → Clock, nicht Clock → Lesen.

### 74HC595 Schreiben (Bit-Banging)

```cpp
static void writeLEDs() {
    // Letztes IC zuerst (Daisy-Chain)
    for (int16_t ic = LED_BYTES - 1; ic >= 0; --ic) {
        uint8_t byte = ledState[ic];
        for (int8_t bit = 7; bit >= 0; --bit) {
            digitalWrite(PIN_LED_MOSI, (byte >> bit) & 1);
            clockPulse();
        }
    }

    // Latch: Daten übernehmen
    digitalWrite(PIN_LED_RCK, HIGH);
    delayMicroseconds(2);
    digitalWrite(PIN_LED_RCK, LOW);
}
```

### Hardware-PWM Setup

```cpp
void setup() {
    // LEDC PWM für OE
    ledcSetup(LEDC_CHANNEL, PWM_FREQ, LEDC_RESOLUTION);
    ledcAttachPin(PIN_LED_OE, LEDC_CHANNEL);
    setLEDBrightness(PWM_DUTY);
}

static void setLEDBrightness(uint8_t percent) {
    // OE ist Active-LOW: duty invertieren
    uint8_t duty = 255 - (percent * 255 / 100);
    ledcWrite(LEDC_CHANNEL, duty);
}
```

### Button-ID Berechnung

```cpp
// Formel: btn_id = 8 * ic + (7 - bit) + 1
// Verdrahtung: P8 = erster Taster pro IC

static bool isPressed(uint8_t id) {
    uint8_t ic  = (id - 1) / 8;
    uint8_t bit = 7 - ((id - 1) % 8);  // P8=Bit7, P7=Bit6, ...
    return !(btnState[ic] & (1 << bit));  // Active-Low
}
```

| ID | IC | Bit | Pin |
|----|-----|-----|-----|
| 1 | 0 | 7 | P8 |
| 2 | 0 | 6 | P7 |
| 8 | 0 | 0 | P1 |
| 9 | 1 | 7 | P8 |
| 10 | 1 | 6 | P7 |

---

## Teil 5: Timing-Analyse

### CD4021B Timing (bei 3.3 V)

| Parameter | Datenblatt | Unser Delay | Faktor |
|-----------|------------|-------------|--------|
| P/S Setup | ~100 ns | 5 µs | 50× |
| P/S Hold | ~100 ns | 2 µs | 20× |
| Clock Width | ~80 ns | 4 µs | 50× |

### Gesamtzeit pro Scan

```
10 Taster (2 Bytes):
  Load:  7 µs
  Read:  16 × 5 µs = 80 µs
  ─────────────────────────
  Summe: ~87 µs

100 Taster (13 Bytes):
  Load:  7 µs
  Read:  104 × 5 µs = 520 µs
  ─────────────────────────
  Summe: ~527 µs
```

Bei 20 ms Loop-Delay haben wir reichlich Reserve.

---

## Teil 6: Skalierung auf 100×

### Firmware-Änderung

```cpp
// Vorher
constexpr size_t BTN_COUNT = 10;

// Nachher
constexpr size_t BTN_COUNT = 100;
```

Der Rest berechnet sich automatisch:

| Konstante | 10× | 100× |
|-----------|-----|------|
| `BTN_COUNT` | 10 | 100 |
| `BTN_BYTES` | 2 | 13 |
| `LED_BYTES` | 2 | 13 |

### Hardware-Erweiterung

```
IC-Anzahl: 13× CD4021B + 13× 74HC595 = 26 ICs

Verdrahtung:
- Alle SCK parallel an D8
- Alle OE parallel an D2 (PWM)
- CD4021B Daisy-Chain: Q8 → SI
- 74HC595 Daisy-Chain: QH' → SI
```

---

## Zusammenfassung

| Aspekt | v1.x | v2.0 |
|--------|------|------|
| Clock | Getrennt | Shared (D8) |
| PWM | Software | Hardware (OE) |
| Modus | Toggle | Direkt |
| Skalierung | Mehrere Arrays | Eine Konstante |
| GPIO-Pins | 6 | 6 |

---

## Referenzen

- SGS-Thomson: *M54/74HC595 Datasheet* – 8-Bit Shift Register with Output Latches
- Texas Instruments: *CD4021B Datasheet* – CMOS 8-Stage Static Shift Register
- Seeed Studio: *XIAO ESP32S3 Wiki* – Pinout und Spezifikationen
