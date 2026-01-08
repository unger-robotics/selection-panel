# Systemübersicht

## Ziel

- **Taster n → LED n (1:1)**
- **One-Hot**: maximal eine LED aktiv
- Nicht-blockierende Firmware (IO stabil, Logging entkoppelt)

## Hardware

| Komponente | Funktion | Logik |
|------------|----------|-------|
| CD4021B | Taster-Schieberegister | Active-Low (gedrückt = 0) |
| 74HC595 | LED-Schieberegister | Active-High (an = 1) |

**SPI-Bus:**
- Shared SCK (D8)
- Separate Datenleitungen: MISO (D9) für Buttons, MOSI (D10) für LEDs
- Separate Steuerung: P/S (D1) für CD4021B, RCK (D0) für 74HC595
- Optional: OE-PWM (D2) für LED-Helligkeit (active-low)

## Firmware-Pipeline

```
1) Read RAW      → HW-SPI, CD4021B (first-bit rescue)
2) Debounce      → Zeitbasiert (30 ms)
3) Selection     → Pressed-Edge Detection → activeId
4) Build One-Hot → LED-Bytes aus activeId
5) Write LEDs    → HW-SPI, 74HC595 + Latch
6) LogEvent      → Queue → Serial-Task (non-blocking)
```

## Architektur-Schichten

```
┌─────────────────────────────────────┐
│  main.cpp (Entry Point)            │
├─────────────────────────────────────┤
│  app/ (FreeRTOS Tasks)             │
│  • io_task: 200 Hz Zyklus          │
│  • serial_task: Event-driven       │
├─────────────────────────────────────┤
│  logic/ (Geschäftslogik)           │
│  • debounce: Entprellung           │
│  • selection: One-Hot Policy       │
├─────────────────────────────────────┤
│  drivers/ (Hardware-Treiber)       │
│  • cd4021: Button-Input            │
│  • hc595: LED-Output               │
├─────────────────────────────────────┤
│  hal/ (Hardware Abstraction)       │
│  • spi_bus: Mutex + Guard          │
└─────────────────────────────────────┘
```
