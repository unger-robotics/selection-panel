# Debug Playbook

## Grundregel

IO darf niemals durch Logging blockieren. Debug-Ausgaben nur bei Zustandsänderungen.

---

## Symptom: Taster 1 fehlt / alles um 1 verschoben

**Ursache:** CD4021 liefert erstes Bit direkt nach Load; reines HW-SPI verliert es → Shift um 1.
**Fix:** “first bit rescue”: erstes Bit per `digitalRead(MISO)` vor SPI lesen, danach Bytes rekonstruieren.

---

## Symptom: Bits flackern / inkonsistent

**Ursache:** falscher SPI-Mode oder zu hohe Frequenz auf Lochraster.
**Fix:** CD4021: `SPI_MODE1`, 500 kHz; 74HC595: `SPI_MODE0`, 1 MHz.

---

## Symptom: PWM invertiert

**Ursache:** OE ist active-low.
**Fix:** Duty invertieren (mehr % -> länger LOW).

---

## Symptom: LEDs “merken” alten Zustand nach Reset

**Ursache:** 74HC595 Latch hält Zustand.
**Fix:** Im Init definierte 0-Bytes schreiben + latch; OE ggf. kurz deaktivieren.
