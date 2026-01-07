# Hardware-Dokumentation: Selection Panel Prototyp v2

10 Taster → 10 LEDs über Schieberegister am XIAO ESP32-S3.

## Übersicht

```
┌─────────────────────────────────────────────────────────────────────────────┐
│                         ACTIVE                                              │
│  ┌─────────────┐    ┌─────────┐ ┌─────────┐    ┌─────────┐ ┌─────────┐      │
│  │ XIAO        │    │ 74HC595 │ │ 74HC595 │    │ CD4021B │ │ CD4021B │      │
│  │ ESP32-S3    │    │   #0    │ │   #1    │    │   #0    │ │   #1    │      │
│  └─────────────┘    │ OUTPUT  │ │ OUTPUT  │    │  INPUT  │ │  INPUT  │      │
│                     └─────────┘ └─────────┘    └─────────┘ └─────────┘      │
│                                                                             │
│                                                                             |
│      3k Widerstände (LED-Vorwiderstände)      10k Widerstände (Pull-Down)   │
│                                                                             │
│      ◉  ◉    ●  ●  ●  ●    ●  ●    ●  ●                                     │
│     klar    blau    rot      gelb    grün                                   │
│      1  2    3  4  5  6    7  8    9  10    ← LED-Nummerierung              │
│                                                                             │
│      ▣  ▣    ▣  ▣  ▣  ▣    ▣  ▣    ▣  ▣                                     │
│      1  2    3  4  5  6    7  8    9  10    ← Taster-Nummerierung           │
└─────────────────────────────────────────────────────────────────────────────┘
```

## Stückliste

| Bauteil | Anzahl | Wert/Typ | Funktion |
|---------|--------|----------|----------|
| XIAO ESP32-S3 | 1 | Seeed Studio | Mikrocontroller |
| 74HC595 | 2 | DIP-16 | LED-Treiber (Serial-In/Parallel-Out) |
| CD4021B | 2 | DIP-16 | Taster-Eingabe (Parallel-In/Serial-Out) |
| LED | 10 | 5mm, diverse Farben | Visuelle Ausgabe |
| Taster | 10 | 6×6mm | Eingabe |
| Widerstand | 10 | 3kΩ | LED-Vorwiderstand |
| Widerstand | 10 | 10kΩ | Taster Pull-Down |

## Pin-Belegung XIAO ESP32-S3

```
                    ┌───────────┐
                5V ─┤ 5V    GND ├─ GND
               GND ─┤ GND  3V3  ├─ 3V3
         (D0)  RCK ─┤ D0    D10 ├─ MOSI (D10)
         (D1)  P/S ─┤ D1     D9 ├─ MISO (D9)
         (D2)  OE  ─┤ D2     D8 ├─ SCK  (D8)
               D3  ─┤ D3     D7 ├─ D7
               D4  ─┤ D4     D6 ├─ D6
               D5  ─┤ D5        ├─
                    └───────────┘
```

| GPIO | Signal | Richtung | Funktion |
|------|--------|----------|----------|
| D0 | RCK | OUT | 74HC595 Latch (STCP) |
| D1 | P/S | OUT | CD4021B Parallel/Serial Select |
| D2 | OE | OUT | 74HC595 Output Enable (PWM) |
| D8 | SCK | OUT | SPI Clock (gemeinsam) |
| D9 | MISO | IN | CD4021B Daten (Q8) |
| D10 | MOSI | OUT | 74HC595 Daten (SER) |

## 74HC595 LED-Kette (Output)

### Pinout 74HC595

```
        ┌────╮────┐
   QB ──┤ 1    16 ├── VCC (+3V3)
   QC ──┤ 2    15 ├── QA        ← LED1 (IC#0), LED9 (IC#1)
   QD ──┤ 3    14 ├── SER       ← MOSI (IC#0), Q7S von IC#0 (IC#1)
   QE ──┤ 4    13 ├── OE        ← D2 (active-low, PWM)
   QF ──┤ 5    12 ├── RCK/STCP  ← D0 (Latch)
   QG ──┤ 6    11 ├── SCK/SHCP  ← D8
   QH ──┤ 7    10 ├── SRCLR     ← +3V3 (nicht löschen)
  GND ──┤ 8     9 ├── Q7S       → SER von IC#1 (Daisy-Chain)
        └─────────┘
```

### Daisy-Chain-Verbindung

```
ESP32-S3                 74HC595 #0              74HC595 #1
┌────────┐              ┌──────────┐            ┌──────────┐
│    D10 ├─── MOSI ────►│ SER      │            │          │
│        │              │      Q7S ├───────────►│ SER      │
│     D8 ├─── SCK ─────►│ SCK      ├───────────►│ SCK      │
│     D0 ├─── RCK ─────►│ RCK      ├───────────►│ RCK      │
│     D2 ├─── OE ──────►│ OE       ├───────────►│ OE       │
└────────┘              └──────────┘            └──────────┘
                             │                       │
                        QA-QH (8 Bit)           QA-QB (2 Bit)
                             │                       │
                             ▼                       ▼
                        LED 1-8                  LED 9-10
```

**Warum 3kΩ?**

- LED-Spannung: ~2V (rot) bis ~3V (blau/weiß)
- Versorgung: 3.3V
- Strom: (3.3V - 2V) / 3kΩ ≈ 0.4 mA (gedimmt, augenschonend)
- Bei 100 LEDs alle ON: 40 mA Gesamtstrom (ESP32S3 XIAO)

### Bit-Zuordnung (LSB-first)

| Ausgang | Bit im Byte | LED-ID |
|---------|-------------|--------|
| IC#0 QA | Bit 0 | LED 1 |
| IC#0 QB | Bit 1 | LED 2 |
| IC#0 QC | Bit 2 | LED 3 |
| IC#0 QD | Bit 3 | LED 4 |
| IC#0 QE | Bit 4 | LED 5 |
| IC#0 QF | Bit 5 | LED 6 |
| IC#0 QG | Bit 6 | LED 7 |
| IC#0 QH | Bit 7 | LED 8 |
| IC#1 QA | Bit 0 | LED 9 |
| IC#1 QB | Bit 1 | LED 10 |

**Code-Entsprechung:**

```cpp
// Bit0 = LED1, Bit1 = LED2, ...
const uint8_t byte = (id - 1) / 8;
const uint8_t bit  = (id - 1) % 8;  // LSB-first
```

## CD4021B Taster-Kette (Input)

### Pinout CD4021B

```
        ┌────╮────┐
   P8 ──┤ 1    16 ├── VCC (+3V3)
   Q6 ──┤ 2    15 ├── P7
   Q8 ──┤ 3    14 ├── P6        ← Taster-Eingänge
  P/S ──┤ 4    13 ├── P5           (Active-Low mit Pull-Down)
  SER ──┤ 5    12 ├── +3V3 (P4 nicht genutzt → HIGH)
   P1 ──┤ 6    11 ├── P3
   P2 ──┤ 7    10 ├── CLK       ← D8 (SCK)
  GND ──┤ 8     9 ├── Q7
        └─────────┘

P/S = Parallel/Serial Select (Pin 4)
  HIGH: Parallel Load (Taster → Register)
  LOW:  Serial Shift (Register → Q8)
```

### Daisy-Chain-Verbindung

```
ESP32-S3                 CD4021B #0              CD4021B #1
┌────────┐              ┌──────────┐            ┌──────────┐
│     D9 │◄─── MISO ────┤ Q8       │            │          │
│        │              │      SER ├◄───────────┤ Q8       │
│     D8 ├─── SCK ─────►│ CLK      ├───────────►│ CLK      │
│     D1 ├─── P/S ─────►│ P/S      ├───────────►│ P/S      │
└────────┘              └──────────┘            └──────────┘
                             ▲                       ▲
                        P1-P8 (8 Bit)           P1-P2 (2 Bit)
                             │                       │
                        Taster 1-8              Taster 9-10
```

### Taster-Beschaltung (Active-Low mit Pull-Down)

```
+3V3 ────────────┐
                 │
                ┌┴┐ (Taster geschlossen = HIGH am Eingang)
                └┬┘
                 │
CD4021B P1 ─────►|
                 │
                ┌┴┐
            10kΩ│ │ Pull-Down
                └┬┘
                 │
GND ─────────────┘

Taster offen:    P1 = LOW  (über Pull-Down)  → Bit = 1 (nicht gedrückt)
Taster gedrückt: P1 = HIGH (über 3V3)        → Bit = 0 (gedrückt)
```

**Warum Active-Low mit Pull-Down?**

- CD4021B hat keine internen Pull-Ups
- Pull-Down hält offene Eingänge definiert auf LOW
- Taster verbindet Eingang direkt mit 3V3
- Konvention: 0 = gedrückt, 1 = losgelassen (wie Keyboard-Matrix)

### Bit-Zuordnung (MSB-first)

| Eingang | Bit im Byte | Taster-ID |
|---------|-------------|-----------|
| IC#0 P1 | Bit 7 | Taster 1 |
| IC#0 P2 | Bit 6 | Taster 2 |
| IC#0 P3 | Bit 5 | Taster 3 |
| IC#0 P4 | Bit 4 | Taster 4 |
| IC#0 P5 | Bit 3 | Taster 5 |
| IC#0 P6 | Bit 2 | Taster 6 |
| IC#0 P7 | Bit 1 | Taster 7 |
| IC#0 P8 | Bit 0 | Taster 8 |
| IC#1 P1 | Bit 7 | Taster 9 |
| IC#1 P2 | Bit 6 | Taster 10 |

**Code-Entsprechung:**

```cpp
// Bit7 = T1, Bit6 = T2, ... (umgekehrte Reihenfolge!)
const uint8_t byte = (id - 1) / 8;
const uint8_t bit  = 7 - ((id - 1) % 8);  // MSB-first
```

## Timing-Diagramme

### LED-Ausgabe (74HC595)

```
         │◄────── 16 Bit Transfer ──────►│
         │                               │
SCK   ───┴─╱╲╱╲╱╲╱╲╱╲╱╲╱╲╱╲╱╲╱╲╱╲╱╲╱╲╱╲─┴───────────

MOSI  ────<D15><D14>...<D1><D0>──────────────────────
          │                   │
          IC#1 Daten          IC#0 Daten
          (LED 9-10)          (LED 1-8)
                                              ┌──┐
RCK   ────────────────────────────────────────┘  └──
                                              │
                                         Latch-Impuls
```

### Taster-Eingabe (CD4021B)

```
      ┌────┐
P/S   ┘    └─────────────────────────────────────────
      │    │
      │Load│
           │◄────── 16 Bit Transfer ──────►│
           │                               │
SCK   ─────┴─╱╲╱╲╱╲╱╲╱╲╱╲╱╲╱╲╱╲╱╲╱╲╱╲╱╲╱╲─┴─────────
           ▲
           │
      First-Bit hier per digitalRead() sichern!
           │
MISO  ─────<T1><T2><T3>...<T15><T16>─────────────────
```

## SPI-Modi

| IC | Modus | CPOL | CPHA | Begründung |
|----|-------|------|------|------------|
| 74HC595 | MODE0 | 0 | 0 | Übernimmt Daten bei steigender Flanke |
| CD4021B | MODE1 | 0 | 1 | Schiebt bei steigender Flanke → Sample bei fallender |

**Im Code:**

```cpp
static const SPISettings spiButtons(500000, MSBFIRST, SPI_MODE1);
static const SPISettings spiLEDs(1000000, MSBFIRST, SPI_MODE0);
```

---

## 74HC595 Latch-Stabilität (RCK/OE/MR) – wichtig bei gemeinsamem SCK (CD4021 + 595)

**Hintergrund:** Wenn CD4021 und 74HC595 den gleichen **SCK** teilen, dann taktet ein Button-Read über SPI auch den 74HC595 mit.
Beim CD4021-Read wird typischerweise `0x00` übertragen → der 74HC595 schiebt dabei **Nullen** in sein Shift-Register.
Solange **RCK (Latch)** stabil LOW bleibt, ist das unkritisch. Ein kurzer RCK-Glitch kann jedoch dazu führen, dass „alles aus“ gelatcht wird.

### Minimal-Hardware (Breadboard-sicher)

- **RCK (Pin 12, gemeinsam für beide 74HC595)**: **1× 10 kΩ Pull-Down nach GND**
- **OE (Pin 13, active-low)**: **1× 10 kΩ Pull-Up nach 3,3 V** (Outputs beim Boot sicher AUS)
- **/MR (Pin 10, active-low)**: **fest an 3,3 V** (oder 10 kΩ Pull-Up)
- **100 nF** direkt an **VCC/GND pro IC** (vorhanden)

### Optional (wenn immer noch Glitches)

- **33–100 Ω** Serienwiderstand in **RCK** (nahe am ESP32-Pin) zur Dämpfung von Überschwingen
