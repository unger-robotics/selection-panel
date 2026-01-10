# Wiring / Pinout / Mapping

## Pins (XIAO ESP32-S3)

| Signal | XIAO Pin | Rolle |
|---|---:|---|
| SCK (shared) | D8 | SPI-Takt für CD4021 + 74HC595 |
| BTN P/S | D1 | CD4021: Parallel/Shift (P/S) |
| BTN MISO | D9 | CD4021: Q8 (Serial-Out zum ESP32) |
| LED MOSI | D10 | 74HC595: SER (Serial-In vom ESP32) |
| LED RCK | D0 | 74HC595: Latch (RCLK/STCP) |
| LED OE | D2 | 74HC595: Output Enable (active-low, optional PWM) |

Versorgung:

- Alle ICs an **3V3** und **GND**
- Pro IC **100 nF** (VCC↔GND, nah am Pin), zusätzlich **10 µF** nahe am 3V3-Rail empfohlen.

## 74HC595 (LEDs) – Grundverdrahtung

- `SER`  ← `LED MOSI (D10)`
- `SRCLK` ← `SCK (D8)`
- `RCLK` ← `LED RCK (D0)`
- `OE`   ← `LED OE (D2)` (oder fest auf GND, wenn immer aktiv)
- `MR`   → 3V3 (Reset inaktiv halten)

Daisy-Chain (mehrere 595):

- `QH'` von Chip 0 → `SER` von Chip 1 → …
- In der Firmware werden Bytes so gesendet, dass Daisy-Chain korrekt gefüllt wird.

## CD4021B (Buttons) – Grundverdrahtung

- `CP/CLK` ← `SCK (D8)`
- `P/S`    ← `BTN P/S (D1)`
- `Q8`     → `BTN MISO (D9)`
- `INH/CE` → GND (Clock Enable aktiv)
- `DS`     → für Daisy-Chain (siehe unten)

Daisy-Chain (2× CD4021, 16 Eingänge, davon 10 genutzt):

- `DS` des **ersten** Chips an **GND** (definierter Serien-Input)
- `Q8` Chip 0 → `DS` Chip 1
- `Q8` Chip 1 → `BTN MISO (D9)`

Hinweis: Die **Bit-Reihenfolge** ist hardwarebedingt. Die Firmware abstrahiert das über `bitops.h` (IDs `1..N` bleiben stabil, auch wenn intern Bits gedreht/verschoben werden).
