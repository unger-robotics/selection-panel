# Wiring / Pinout / Mapping

## Pins (XIAO ESP32-S3)

| Signal | Pin |
|---|---|
| SCK (shared) | D8 |
| BTN P/S | D1 |
| BTN MISO (Q8 vom letzten CD4021) | D9 |
| LED MOSI (SER in erstes 74HC595) | D10 |
| LED RCK (Latch) | D0 |
| LED OE (PWM, active-low) | D2 |

## CD4021 (Buttons) – Bit-Zuordnung

- Active-Low: gedrückt = `0`
- **Bit7 = T1, Bit6 = T2, … Bit0 = T8** (MSB-first)

| Taster-ID | Byte | Bit |
|---:|---:|---:|
| 1..8 | 0 | 7..0 |
| 9..10 | 1 | 7..6 |

**Wichtig:** Nach Parallel-Load liegt das **erste Bit sofort** an Q8 an („first bit“).

## 74HC595 (LEDs) – Bit-Zuordnung

- Active-High: an = `1`
- **Bit0 = LED1, Bit1 = LED2, …** (LSB-first)

| LED-ID | Byte | Bit |
|---:|---:|---:|
| 1..8 | 0 | 0..7 |
| 9..10 | 1 | 0..1 |
