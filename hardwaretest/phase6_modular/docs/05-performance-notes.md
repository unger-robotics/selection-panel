# Performance Notes

## IO Periodik

- IO läuft alle `IO_PERIOD_MS`
- Debounce: `DEBOUNCE_MS` sollte ein Vielfaches/mehrere Ticks abdecken

## SPI

- Lochraster: 500 kHz (CD4021) / 1 MHz (74HC595) ist robust
- Bei Fehlern zuerst Frequenzen halbieren

## FreeRTOS Stack

- Serial Task braucht mehr Stack (Format/Print)
- IO Task bleibt klein und deterministisch

## Queue Policy

- IO darf nicht blockieren
- Bei voller Queue: Log droppen (best effort)
