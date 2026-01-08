# Firmware-Architektur

## Schichten (Abhängigkeit nur nach unten)

```
┌─────────────┐
│  main.cpp   │  Entry Point, Queue-Erstellung
├─────────────┤
│    app/     │  FreeRTOS Tasks (io_task, serial_task)
├─────────────┤
│   logic/    │  Debounce, Selection (One-Hot Policy)
├─────────────┤
│  drivers/   │  CD4021 readRaw(), HC595 write()/latch()/PWM
├─────────────┤
│    hal/     │  SPI-Bus + Mutex + Guard (thread-safe)
└─────────────┘
```

## Warum diese Aufteilung?

| Vorteil | Erklärung |
|---------|-----------|
| Isolierte Komplexität | Hardware-Spezialfälle (CD4021 first-bit) an genau einer Stelle |
| Testbarkeit | Logik ist testbar ohne Hardware |
| Determinismus | IO bleibt vorhersagbar (kein Serial im IO-Pfad) |
| Wartbarkeit | Klare Verantwortlichkeiten pro Schicht |

## FreeRTOS Taskmodell

### IO-Task (Priorität 5)

- Periodisch via `vTaskDelayUntil` (5 ms = 200 Hz)
- Pipeline: read → debounce → select → write
- Darf **niemals** blockieren

### Serial-Task (Priorität 2)

- Event-driven via `xQueueReceive`
- Format → `Serial.print`
- Kann längere Operationen durchführen ohne IO zu stören

## SPI-Sicherheit

```cpp
// Guard Pattern für automatisches Unlock
{
    SpiGuard guard(spi_bus);  // beginTransaction + Lock
    // ... SPI-Operationen ...
}  // Destruktor: endTransaction + Unlock
```

SPI-Zugriffe werden über Mutex serialisiert. Der Guard garantiert:
- `beginTransaction()` bei Konstruktion
- `endTransaction()` + Unlock bei Destruktion (auch bei Exceptions)

## Konfiguration (config.h)

| Parameter | Wert | Beschreibung |
|-----------|------|--------------|
| `IO_PERIOD_MS` | 5 | Abtastrate (200 Hz) |
| `DEBOUNCE_MS` | 30 | Entprellzeit |
| `SPI_HZ_BTN` | 500000 | CD4021B Taktrate |
| `SPI_HZ_LED` | 1000000 | 74HC595 Taktrate |
| `SPI_MODE_BTN` | MODE1 | CD4021B SPI-Mode |
| `SPI_MODE_LED` | MODE0 | 74HC595 SPI-Mode |
