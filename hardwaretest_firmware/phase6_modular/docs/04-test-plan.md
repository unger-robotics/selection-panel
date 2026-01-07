# Testplan

## Phase A: LED-Test (74HC595, HW-SPI)

**DoD**

- Lauflicht LED1..LED10 korrekt
- Log zeigt Bytes passend zum Mapping (LED1 = bit0)

## Phase B: Button-Test (CD4021, HW-SPI)

**DoD**

- T1..T10 einzeln korrekt erkannt
- RAW/DEB plausibel (DEB stabil nach 30 ms)
- Kein “T1 fehlt” (first-bit rescue bestätigt)

## Phase C: Integration (Buttons -> LEDs, One-Hot)

**DoD**

- Taster n setzt LED n
- One-Hot: nie mehr als eine LED = 1
- Auswahl-Policy dokumentiert (Latch oder nicht)

## Phase D: FreeRTOS Stabilität

**DoD**

- IO-Periodik stabil (z. B. 5 ms)
- Serial Task kann lang loggen ohne IO-Jitter
- Keine Reboots/Watchdog/Stack issues (Stacks dokumentiert)
