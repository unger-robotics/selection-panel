# Wiring / Pinout / Mapping

## Pins (XIAO ESP32-S3)

| Signal | Pin | Funktion |
|--------|-----|----------|
| SCK (shared) | D8 | SPI-Takt für beide Chip-Typen |
| BTN P/S | D1 | CD4021B Parallel/Serial Control |
| BTN MISO | D9 | CD4021B Q8 (Daten vom ersten Chip) |
| LED MOSI | D10 | 74HC595 SER (Daten zum ersten Chip) |
| LED RCK | D0 | 74HC595 Latch (STCP) |
| LED OE | D2 | 74HC595 Output Enable (PWM, active-low) |

## CD4021B (Buttons) – Bit-Zuordnung

- Active-Low: gedrückt = `0`, losgelassen = `1`
- CD4021B gibt MSB-first aus: PI-1 erscheint zuerst (Bit 7), PI-8 zuletzt (Bit 0)
- **Hardware-Verdrahtung:** BTN 1 → PI-8, BTN 8 → PI-1

| Taster | Pin-Name | Pin-Nr | Byte | Bit |
|--------|----------|--------|------|-----|
| BTN 1 | PI-8 | 1 | 0 | 0 |
| BTN 2 | PI-7 | 15 | 0 | 1 |
| BTN 3 | PI-6 | 14 | 0 | 2 |
| BTN 4 | PI-5 | 13 | 0 | 3 |
| BTN 5 | PI-4 | 4 | 0 | 4 |
| BTN 6 | PI-3 | 5 | 0 | 5 |
| BTN 7 | PI-2 | 6 | 0 | 6 |
| BTN 8 | PI-1 | 7 | 0 | 7 |
| BTN 9 | PI-8 | 1 | 1 | 0 |
| BTN 10 | PI-7 | 15 | 1 | 1 |

**Firmware-Abstraktion:** `bitops.h` verwendet `btn_bit(id) = 7 - ((id - 1) % 8)` um die ID auf das korrekte Bit zu mappen.

**Wichtig:** Nach Parallel-Load liegt das **erste Bit sofort** an Q8 an („first bit problem"). Die Firmware löst dies durch `digitalRead(MISO)` vor dem SPI-Transfer.

## 74HC595 (LEDs) – Bit-Zuordnung

- Active-High: an = `1`, aus = `0`
- **Bit0 = LED1, Bit1 = LED2, …** (LSB-first)

| LED | Byte | Bit |
|-----|------|-----|
| LED 1 | 0 | 0 |
| LED 2 | 0 | 1 |
| LED 3 | 0 | 2 |
| LED 4 | 0 | 3 |
| LED 5 | 0 | 4 |
| LED 6 | 0 | 5 |
| LED 7 | 0 | 6 |
| LED 8 | 0 | 7 |
| LED 9 | 1 | 0 |
| LED 10 | 1 | 1 |

**Firmware-Abstraktion:** `bitops.h` verwendet `led_bit(id) = (id - 1) % 8`.

## Daisy-Chain Verbindungen

```
74HC595: ESP32 D10 → SER #0 → QH' #0 → SER #1
CD4021B: 3V3 → DS #1 → Q8 #1 → DS #0 → Q8 #0 → ESP32 D9
```

## Stützkondensatoren

Jeder IC benötigt 100 nF zwischen VCC und GND, möglichst nah am Chip.
