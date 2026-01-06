# Phase 4: Kombinierter Test (Taster → LED)

Taster drücken → entsprechende LED leuchtet.

## Funktion

```
Taster 1 gedrückt → LED 1 an
Taster 2 gedrückt → LED 2 an
...
Taster 100 gedrückt → LED 100 an
```

## Dateien

```
phase4_combined_test/
├── platformio.ini
├── README.md
└── src/
    ├── config.h      ← Zentrale Konfiguration
    └── main.cpp
```

## Skalierung

```cpp
// config.h - NUR DIESE ZEILEN ÄNDERN:
constexpr size_t BTN_COUNT = 10;   // → 100 für Produktion
constexpr size_t LED_COUNT = 10;   // → 100 für Produktion
```

| Anzahl | CD4021B ICs | 74HC595 ICs | Bytes |
|--------|-------------|-------------|-------|
| 10     | 2           | 2           | 2     |
| 100    | 13          | 13          | 13    |

## Build & Flash

```bash
cd phase4_combined_test
pio run -t upload
pio device monitor
```

## Erwartete Ausgabe

```
========================================
Phase 4: Kombiniert (Taster -> LED)
========================================

Konfiguration: 10 Taster, 10 LEDs
Hardware: 2x CD4021B, 2x 74HC595
PWM Helligkeit: 50%

Bereit. Druecke Taster...

T 1->L 1 | IC#0 | BTN:Bit7(MSB) LED:Bit0(LSB) | 0x7F->0x01
T 2->L 2 | IC#0 | BTN:Bit6(MSB) LED:Bit1(LSB) | 0xBF->0x02
T 9->L 9 | IC#1 | BTN:Bit7(MSB) LED:Bit0(LSB) | 0x7F->0x01
T10->L10 | IC#1 | BTN:Bit6(MSB) LED:Bit1(LSB) | 0xBF->0x02
```

## MSB / LSB Erklärung

```
MSB = Most Significant Bit  (höchstwertiges Bit, links)
LSB = Least Significant Bit (niederwertigstes Bit, rechts)

Byte: [Bit7] [Bit6] [Bit5] [Bit4] [Bit3] [Bit2] [Bit1] [Bit0]
        MSB ◄──────────────────────────────────────────► LSB
       128    64    32     16     8      4      2      1
```

**CD4021B (Taster):** MSB-basierte Zuordnung
- Q8 (P8) kommt beim Shift zuerst raus → Bit 7
- Bit 7 = Taster 1, Bit 6 = Taster 2, ...

**74HC595 (LED):** LSB-basierte Zuordnung  
- QA ist das erste Ausgangs-Pin → Bit 0
- Bit 0 = LED 1, Bit 1 = LED 2, ...

## Bit-Zuordnung pro IC

| ID in IC | Taster Bit | LED Bit |
|----------|------------|---------|
| 1        | 7          | 0       |
| 2        | 6          | 1       |
| 3        | 5          | 2       |
| 4        | 4          | 3       |
| 5        | 3          | 4       |
| 6        | 2          | 5       |
| 7        | 1          | 6       |
| 8        | 0          | 7       |

## Pinbelegung

| Pin | Funktion | Bei Skalierung |
|-----|----------|----------------|
| D8  | SCK      | Alle ICs       |
| D1  | BTN P/S  | Alle CD4021B   |
| D9  | BTN MISO | Letzter CD4021B Q8 |
| D10 | LED MOSI | Erster 74HC595 SI |
| D0  | LED RCK  | Alle 74HC595   |
| D2  | LED OE   | Alle 74HC595   |
