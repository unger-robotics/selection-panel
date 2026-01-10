# Systemübersicht

## Ziel / Invarianten

- **Taster n → LED n (1:1)**, IDs: `1..BTN_COUNT`
- **One-Hot**: maximal **eine** LED aktiv (oder `0 = alle aus`)
- **Preempt**: neuer Tastendruck überschreibt sofort die vorherige Auswahl
- **Nicht-blockierend**: IO-Zyklus bleibt stabil (200 Hz), Serial/Logging ist entkoppelt

## Hardware (Prototyp)

| Bauteil | Rolle | Logik |
|---|---|---|
| CD4021B | Button-Input (Shift-Register) | Active-Low (`0 = gedrückt`) |
| 74HC595 | LED-Output (Shift-Register) | Active-High (`1 = an`) |

**SPI (shared, ohne CS):**

- Gemeinsamer SCK (`PIN_SCK`)
- CD4021 nutzt **MISO** (`PIN_BTN_MISO`) + **P/S** (`PIN_BTN_PS`)
- 74HC595 nutzt **MOSI** (`PIN_LED_MOSI`) + **Latch** (`PIN_LED_RCK`) + optional **OE/PWM** (`PIN_LED_OE`)
- Wichtig: **CPOL muss 0 bleiben** (SPI Mode 0/1 ok), weil keine CS-Leitungen existieren.

## Firmware: Tasks & Datenfluss

### IO-Task (Priorität `PRIO_IO`, 200 Hz)

Zyklus (`IO_PERIOD_MS = 5 ms`):

1) Button-Rohdaten lesen (CD4021)
2) Entprellen (`DEBOUNCE_MS`)
3) Auswahl/One-Hot aktualisieren (Selection)
4) LED-Bytes schreiben (74HC595)
5) Log-Event non-blocking in `logQueue` senden

Eigenschaft: darf **niemals** blockieren (Timing ist die Referenz).

### Serial-Task (Priorität `PRIO_SERIAL`)

- Liest `logQueue` (Events) und sendet **PRESS/RELEASE** an den Pi
- Nimmt Kommandos vom Pi an (z. B. `LEDSET n`) und gibt sie an die IO-Task weiter
- Einzige Stelle mit `Serial.*`

## Boot / Handshake

- Die Firmware sendet `READY`, sobald sie **Kommandos sicher annehmen** kann.
- Host-Regel: **erst nach `READY`** Kommandos senden.

## Serial-Protokoll (kompakt)

**ESP32 → Pi**

- `READY`
- `PRESS %03u`
- `RELEASE %03u`
- `OK`
- `ERROR <reason>`

**Pi → ESP32 (minimal, One-Hot-konform)**

- `PING`
- `STATUS`
- `VERSION`
- `HELP`
- `LEDSET n`  (setzt One-Hot: genau LED `n` an)

Hinweis: Debug-Kommandos wie `LEDON/LEDOFF/LEDALL` sind **nicht One-Hot-konform** und sollten in produktivem Protokollbetrieb deaktiviert sein.

## SPI-Sicherheit

SPI-Zugriffe laufen über `SpiBus` (Mutex + Guard):

- `beginTransaction()` beim Eintritt
- `endTransaction()` + Unlock beim Verlassen (RAII)
- Keine “freilaufenden” SCK-Flanken außerhalb von SPI-Transaktionen
