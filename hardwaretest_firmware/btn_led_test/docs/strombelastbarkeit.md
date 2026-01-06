## Strombelastbarkeit

### 74HC595 (aus Datenblatt)

| Parameter | Wert | Bemerkung |
|-----------|------|-----------|
| **Absolute Maximum** | | |
| IO pro Pin (QA-QH) | ±35 mA | Zerstörungsgrenze |
| IO pro Pin (QH') | ±25 mA | Zerstörungsgrenze |
| ICC/GND gesamt | ±70 mA | Gesamtstrom |
| **Garantierte Treiberleistung** | | |
| IOH/IOL (QA-QH) | 6 mA | Spannungspegel stabil |
| IOH/IOL (QH') | 4 mA | Spannungspegel stabil |

**Empfehlung:** ≤6 mA pro LED für stabilen Betrieb.

Mit 3 kΩ Vorwiderstand (aktuell): ~0.3 mA → sehr konservativ
Mit 330 Ω Vorwiderstand: ~3-5 mA → gute Helligkeit, sicher

---

### ESP32-S3 (XIAO)

| Parameter | Wert | Bemerkung |
|-----------|------|-----------|
| GPIO Output (Default) | ~20 mA | Drive Strength 2 |
| GPIO Output (Max) | ~40 mA | Drive Strength 3 |
| Gesamt alle GPIOs | ~1200 mA | Summenstrom |
| 3.3V Regulator (XIAO) | 800 mA | ME6217C33M5G |

**Für dein Projekt relevant:**

Die ESP32-GPIOs treiben nur die Schieberegister-Eingänge (SI, SCK, RCK) – das sind µA, kein Problem.

Die **LEDs werden vom 74HC595 versorgt**, nicht vom ESP32. Der kritische Pfad ist:

```
USB 5V → XIAO 3.3V Regulator (800mA) → 74HC595 VCC → LED-Ausgänge
```

---

### Berechnung für 100 LEDs

| Szenario | Strom/LED | 100 LEDs | Status |
|----------|-----------|----------|--------|
| 3 kΩ (aktuell) | 0.3 mA | 30 mA | ✓ OK |
| 330 Ω (hell) | 5 mA | 500 mA | ✓ OK |
| 220 Ω (sehr hell) | 7 mA | 700 mA | ⚠️ Grenzwertig |

**Mit PWM 50%** halbiert sich der Durchschnittsstrom nochmals.

**Empfehlung für 100 LEDs:** 470 Ω Vorwiderstand → ~3 mA/LED → 300 mA gesamt (sicher).
