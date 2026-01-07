# Firmware-Architektur

## Schichten (Abhängigkeit nur nach unten)

- `hal/`      : SPI-Bus + Mutex + Guard (thread-safe)
- `drivers/`  : CD4021 readRaw(), 74HC595 write()/latch()/PWM
- `logic/`    : Debounce (zeitbasiert), Selection (One-Hot Policy)
- `app/`      : FreeRTOS Tasks + Queue
- `main.cpp`  : nur init + Tasks starten

## Warum diese Aufteilung?

- Hardware-Spezialfälle (CD4021 first-bit) sind an genau einer Stelle.
- Logik ist testbar ohne Hardware.
- IO bleibt deterministisch (kein Serial im IO-Pfad).

## FreeRTOS Taskmodell (empfohlen)

- **IO-Task**: periodisch (`vTaskDelayUntil`)
  - read → debounce → select → write
- **Serial-Task**: event-driven (`xQueueReceive`)
  - format → Serial.print

## SPI-Sicherheit

SPI-Zugriffe werden über Mutex serialisiert (Guard macht endTransaction+unlock sicher).
