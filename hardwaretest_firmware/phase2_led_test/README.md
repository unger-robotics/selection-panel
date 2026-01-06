# Phase 2: LED-Test (74HC595)

LED-Test über Schieberegister 74HC595 (Active-High).

## Hex-Referenz

| LED | Hex  | Binär      |
|-----|------|------------|
| 1   | 0x01 | 0000 0001  |
| 2   | 0x02 | 0000 0010  |
| 3   | 0x04 | 0000 0100  |
| 4   | 0x08 | 0000 1000  |
| 5   | 0x10 | 0001 0000  |
| 6   | 0x20 | 0010 0000  |
| 7   | 0x40 | 0100 0000  |
| 8   | 0x80 | 1000 0000  |
| 9-10| IC1  | wie IC0    |
| alle| 0xFF | 1111 1111  |

## Build & Flash

```bash
cd phase2_led_test
pio run -t upload
pio device monitor
```

## Erwartete Ausgabe

```
========================================
LED-Test - 74HC595 (Active-High)
========================================

Hex-Referenz:
L1=0x01 L2=0x02 L3=0x04 L4=0x08
L5=0x10 L6=0x20 L7=0x40 L8=0x80
L9/L10 auf IC1 (gleiche Werte)

PWM Helligkeit: 50%

Starte Lauflicht...

IC0: 00000001 (0x01) | IC1: 00000000 (0x00) | LED: 1
IC0: 00000010 (0x02) | IC1: 00000000 (0x00) | LED: 2
IC0: 00000100 (0x04) | IC1: 00000000 (0x00) | LED: 3
IC0: 00001000 (0x08) | IC1: 00000000 (0x00) | LED: 4
IC0: 00010000 (0x10) | IC1: 00000000 (0x00) | LED: 5
IC0: 00100000 (0x20) | IC1: 00000000 (0x00) | LED: 6
IC0: 01000000 (0x40) | IC1: 00000000 (0x00) | LED: 7
IC0: 10000000 (0x80) | IC1: 00000000 (0x00) | LED: 8
IC0: 00000000 (0x00) | IC1: 00000001 (0x01) | LED: 9
IC0: 00000000 (0x00) | IC1: 00000010 (0x02) | LED: 10
```

## Pinbelegung

| Signal | Pin | 74HC595 | Funktion |
|--------|-----|---------|----------|
| SCK    | D8  | Pin 11  | Clock    |
| MOSI   | D10 | Pin 14  | Data     |
| RCK    | D0  | Pin 12  | Latch    |
| OE     | D2  | Pin 13  | PWM      |
