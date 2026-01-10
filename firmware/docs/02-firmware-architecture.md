# Firmware-Architektur

## Schichten (Abhängigkeit nur nach unten)

```

┌─────────────┐
│  main.cpp   │  Entry Point, Queue-Erstellung, Task-Start
├─────────────┤
│    app/     │  FreeRTOS Tasks: io_task, serial_task
├─────────────┤
│   logic/    │  debounce (Entprellung), selection (One-Hot/Preempt)
├─────────────┤
│  drivers/   │  cd4021 (readRaw), hc595 (write/latch/OE-PWM)
├─────────────┤
│    hal/     │  spi_bus (Mutex + Transactions)
└─────────────┘

```

## Task-Modell

### IO-Task (`PRIO_IO`, `CORE_APP`, 200 Hz)

- harte Periode via `vTaskDelayUntil()`
- Verantwortlich für:
  - Rohdaten lesen → entprellen → Auswahl berechnen → LEDs schreiben
  - Host-LED-Kommandos aus Queue verarbeiten (non-blocking)
  - LogEvent-Snapshots in `logQueue` senden

### Serial-Task (`PRIO_SERIAL`, `CORE_APP`)

- Verantwortlich für:
  - Protokollausgabe (PRESS/RELEASE/STATUS)
  - Kommandos annehmen und an IO-Task weiterreichen
- Darf langsamer sein, darf IO **nicht** stören.

## Queues / Entkopplung

- `logQueue`: IO → Serial (Snapshots/Events)
- `ledCmdQueue`: Serial → IO (Host-Kommandos)

Ziel: kein direkter Hardwarezugriff aus der Serial-Task.

## One-Hot: wo wird es garantiert?

- lokal: `selection` + `buildOneHotLED(activeId)` → LED-Bytes werden One-Hot erzeugt
- remote: **nur `LEDSET n`** ist One-Hot-konform
  (`LEDON/LEDOFF/LEDALL` sind Debug und nicht One-Hot-konform)

## SPI (shared bus ohne CS)

- SPI-Zugriffe sind über `SpiBus` serialisiert (Mutex + `beginTransaction/endTransaction`)
- Erlaubte Modi: **Mode 0/1** (CPOL=0). Kein Mode 2/3.
- Keine SCK-Flanken außerhalb von SPI-Transaktionen.

## Konfiguration (Auszug)

| Parameter | Wert | Bedeutung |
|---|---:|---|
| `IO_PERIOD_MS` | 5 | `200 Hz` IO-Zyklus |
| `DEBOUNCE_MS` | 30 | Entprellen (zeitbasiert) |
| `SPI_HZ_BTN` | 500000 | CD4021 Takt |
| `SPI_HZ_LED` | 1000000 | 74HC595 Takt |
| `SPI_MODE_BTN` | MODE1 | CD4021 Mode |
| `SPI_MODE_LED` | MODE0 | 74HC595 Mode |
