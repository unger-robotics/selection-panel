# Systemübersicht

## Ziel

- **Taster n → LED n (1:1)**
- **One-Hot**: maximal eine LED aktiv
- Nicht-blockierende Firmware (IO stabil, Logging entkoppelt)

## Hardware

- CD4021B: Taster-Shiftregister (Active-Low)
- 74HC595: LED-Shiftregister (Active-High)
- Shared SCK, separate Datenleitungen:
  - Buttons: MISO (Q8)
  - LEDs: MOSI (SER)
- Optional: OE-PWM für Helligkeit (active-low)

## Firmware-Pipeline

1) Read RAW (HW-SPI, CD4021)
2) Debounce (zeitbasiert)
3) Selection (pressed-edge) → activeId
4) Build One-Hot LED bytes
5) Write LEDs (HW-SPI, 74HC595 + Latch)
6) LogEvent → Serial-Task (optional, FreeRTOS)
