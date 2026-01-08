# Contributing

## Dogma

- Kein dynamischer Speicher im Dauerbetrieb
- Kein Serial im IO-Pfad
- Keine blockierenden Delays im Taskpfad (`vTaskDelayUntil`)

## Code Style

- Mapping/Bitlogik zentral (kein Copy&Paste)
- Kommentare erklären **warum** (Timing, Edge-Cases, Hardware)
- Hardware-Details nur in `drivers/`
- Logik (Debounce/Selection) ohne Pins/SPI

## Pull Requests

- Jede Änderung hat: (1) Grund, (2) DoD-Test, (3) Doku-Update falls Mapping/Policy betroffen
