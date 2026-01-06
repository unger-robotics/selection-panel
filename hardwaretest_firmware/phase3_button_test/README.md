# Phase 3: Taster-Test (CD4021B)

Taster-Test über Schieberegister CD4021B (Active-Low).

## Hex-Referenz (Bit7 = Taster 1, Hardware-bedingt)

| Taster | Hex  | Binär      |
|--------|------|------------|
| 1      | 0x7F | 0111 1111  |
| 2      | 0xBF | 1011 1111  |
| 3      | 0xDF | 1101 1111  |
| 4      | 0xEF | 1110 1111  |
| 5      | 0xF7 | 1111 0111  |
| 6      | 0xFB | 1111 1011  |
| 7      | 0xFD | 1111 1101  |
| 8      | 0xFE | 1111 1110  |
| 9-10   | IC1  | wie IC0    |
| keiner | 0xFF | 1111 1111  |

## Bit-Zuordnung

Die Zuordnung ist Hardware-bedingt unterschiedlich:

| ID | Taster (CD4021B) | LED (74HC595) |
|----|------------------|---------------|
| 1  | Bit 7            | Bit 0         |
| 2  | Bit 6            | Bit 1         |
| 3  | Bit 5            | Bit 2         |
| 4  | Bit 4            | Bit 3         |
| 5  | Bit 3            | Bit 4         |
| 6  | Bit 2            | Bit 5         |
| 7  | Bit 1            | Bit 6         |
| 8  | Bit 0            | Bit 7         |

Die Software abstrahiert das: `isPressed(5)` und `setLED(5, true)` funktionieren korrekt.

## Build & Flash

```bash
cd phase3_button_test
pio run -t upload
pio device monitor
```

## Erwartete Ausgabe

```
========================================
Taster-Test - CD4021B (Active-Low)
========================================

Hex-Referenz (Bit7=T1, Hardware-bedingt):
T1=0x7F T2=0xBF T3=0xDF T4=0xEF
T5=0xF7 T6=0xFB T7=0xFD T8=0xFE
T9/T10 auf IC1 (gleiche Werte)

Warte auf Tastendruck...

IC0: 01111111 (0x7F) | IC1: 11111111 (0xFF) | Taster: 1
IC0: 10111111 (0xBF) | IC1: 11111111 (0xFF) | Taster: 2
IC0: 11111111 (0xFF) | IC1: 01111111 (0x7F) | Taster: 9
IC0: 11111111 (0xFF) | IC1: 10111111 (0xBF) | Taster: 10
```

## Pinbelegung

| Signal | Pin | CD4021B | Funktion |
|--------|-----|---------|----------|
| SCK    | D8  | Pin 10  | Clock    |
| P/S    | D1  | Pin 9   | Load     |
| MISO   | D9  | Pin 3   | Data     |
