# Debug Playbook

## Grundregel

IO darf niemals durch Logging blockieren. Debug-Ausgaben nur bei Zustandsänderungen.

---

## Symptom: Taster 1 fehlt / alles um 1 verschoben

**Ursache:** CD4021B liefert erstes Bit direkt nach Parallel-Load; reines HW-SPI verliert es → Shift um 1.

**Fix:** "First-Bit Rescue": erstes Bit per `digitalRead(MISO)` vor SPI lesen, danach Bytes rekonstruieren.

```cpp
// Beispiel aus cd4021.cpp
uint8_t firstBit = digitalRead(PIN_BTN_MISO);
SPI.transfer(buffer, BTN_BYTES);
// firstBit in buffer[0] Bit 7 einfügen
```

---

## Symptom: Bits flackern / inkonsistent

**Ursache:** Falscher SPI-Mode oder zu hohe Frequenz auf Lochraster.

**Fix:**
- CD4021B: `SPI_MODE1`, 500 kHz
- 74HC595: `SPI_MODE0`, 1 MHz

Bei anhaltenden Problemen: Frequenzen halbieren.

---

## Symptom: PWM invertiert (mehr Duty = dunkler)

**Ursache:** OE ist active-low.

**Fix:** Duty invertieren in der Firmware:
```cpp
uint8_t invertedDuty = 255 - duty;  // 0% Duty → volle Helligkeit
```

---

## Symptom: LEDs "merken" alten Zustand nach Reset

**Ursache:** 74HC595 Latch hält Zustand, CLR ist nicht angeschlossen.

**Fix:**
1. CLR (Pin 10) auf 3V3 legen (nicht floaten!)
2. Im Init: definierte 0-Bytes schreiben + latch
3. OE kurz HIGH setzen während Init

---

## Symptom: Alle Taster zeigen "gedrückt" beim Start

**Ursache:** DS-Pin des letzten CD4021B auf GND statt 3V3.

**Fix:** DS (Pin 11) des letzten CD4021B auf **+3V3** legen.

---

## Symptom: Phantom-Tastendrücke auf unbenutzten Eingängen

**Ursache:** Floating Inputs auf CD4021B #1.

**Fix:** Ungenutzte PI-x Pins auf +3V3 legen (nicht offen lassen).

---

## Symptom: Serial-Output zeigt Zahlen statt PRESS/RELEASE

**Ursache:** `SERIAL_PROTOCOL_ONLY = false` in config.h.

**Fix:** Für Pi-Betrieb: `SERIAL_PROTOCOL_ONLY = true` setzen.

---

## Debug-Logging aktivieren

In `config.h`:
```cpp
constexpr bool LOG_VERBOSE_PER_ID = true;   // Detailliert pro Taster
constexpr bool LOG_ON_RAW_CHANGE = true;    // Auch unentprellte Änderungen
```

**Warnung:** Nur zum Debugging! Kann IO-Timing beeinflussen.
