# Performance Notes

## IO-Periodik

| Parameter | Wert | Beschreibung |
|-----------|------|--------------|
| `IO_PERIOD_MS` | 5 ms | Abtastrate (200 Hz) |
| `DEBOUNCE_MS` | 30 ms | Entprellzeit (6 Zyklen) |

**Regel:** `DEBOUNCE_MS` sollte mehrere IO-Zyklen abdecken (mindestens 3-5).

## SPI-Timing

| Chip | Frequenz | Mode | Bemerkung |
|------|----------|------|-----------|
| CD4021B | 500 kHz | MODE1 | Lochraster-sicher |
| 74HC595 | 1 MHz | MODE0 | Lochraster-sicher |

**Bei Fehlern:** Frequenzen halbieren (250 kHz / 500 kHz).

### Transfer-Zeiten (10 Buttons/LEDs)

| Operation | Bytes | Zeit @ 500 kHz |
|-----------|-------|----------------|
| Button Read | 2 | ~32 µs |
| LED Write | 2 | ~16 µs |
| **Gesamt** | 4 | **~50 µs** |

**Headroom:** Bei 5 ms Zyklus sind 50 µs nur 1% der verfügbaren Zeit.

## FreeRTOS Stack

| Task | Stack (Words) | Bemerkung |
|------|---------------|-----------|
| IO-Task | 2048 | Minimal, deterministisch |
| Serial-Task | 4096 | Mehr Stack für Format/Print |

**Monitoring:**
```cpp
UBaseType_t highWater = uxTaskGetStackHighWaterMark(NULL);
// Sollte > 200 Words sein
```

## Queue Policy

```
IO-Task ──[LogEvent]──▶ Queue ──▶ Serial-Task
                         │
                         └─ Bei Queue voll: Event droppen (non-blocking)
```

**Wichtig:**
- IO darf **niemals** blockieren
- `xQueueSend` mit `0` Timeout (sofort zurück wenn voll)
- Queue-Größe: 32 Events (in config.h einstellbar)

## Latenz-Analyse

| Pfad | Latenz |
|------|--------|
| Taster → ESP32 erkennt | 5-35 ms (IO-Zyklus + Debounce) |
| ESP32 → Pi empfängt | < 5 ms (USB-CDC) |
| Pi → Browser (WebSocket) | < 10 ms |
| Browser → Audio startet | ~40 ms (preloaded) |
| **Gesamt** | **~60-90 ms** |

## Stromverbrauch

| Zustand | Strom |
|---------|-------|
| Idle (WiFi aus) | ~80 mA |
| Active (LEDs an) | ~120 mA |
| Peak (WiFi aktiv) | ~500 mA |

## Skalierung auf 100 Buttons

| Metrik | 10 Buttons | 100 Buttons |
|--------|------------|-------------|
| SPI-Bytes | 4 | 26 |
| Transfer-Zeit | ~50 µs | ~260 µs |
| IO-Headroom | 99% | 95% |
| ICs benötigt | 4 | 26 |
