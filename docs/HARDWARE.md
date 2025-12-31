# HARDWARE — Stückliste und Aufbau

> Komponenten und Verdrahtung. Pinbelegung und Policy: siehe [SPEC.md](SPEC.md)

| Version | 2.4.1 |
|---------|-------|
| Stand | 2025-12-30 |
| Status | Prototyp funktionsfähig |

---

## 1 Stückliste (100×)

### Recheneinheit

| Anzahl | Komponente | Anmerkung |
|--------|------------|-----------|
| 1× | Raspberry Pi 5 | Gateway + Media-Server |
| 1× | Netzteil USB-C 5V/5A | |
| 1× | HDMI-Monitor | Dashboard |
| 1× | Aktivlautsprecher | Klinke/HDMI/USB |

### IO-Controller

| Anzahl | Komponente | Anmerkung |
|--------|------------|-----------|
| 1× | Seeed XIAO ESP32-S3 | Dual-Core, USB-C |
| 1× | USB-C Kabel | XIAO ↔ Pi |

### Eingänge (CD4021BE)

| Anzahl | Komponente | Anmerkung |
|--------|------------|-----------|
| 100× | Taster 6×6 mm | Tact-Switch |
| 13× | CD4021BE DIP-16 | 8 Inputs pro IC |
| 100× | Widerstand 10 kΩ | Pull-Up |
| 13× | Kondensator 100 nF | Abblockkondensator |

### Ausgänge (74HC595)

| Anzahl | Komponente | Anmerkung |
|--------|------------|-----------|
| 100× | LED 5 mm | Farbe nach Wahl |
| 13× | 74HC595 DIP-16 | 8 Outputs pro IC |
| 100× | Widerstand 330 Ω | Vorwiderstand @ 5V |
| 13× | Kondensator 100 nF | Abblockkondensator |

### Verdrahtung

| Anzahl | Komponente |
|--------|------------|
| 1× | Lochrasterplatine 160×100 mm |
| 26× | DIP-16 Sockel (optional) |
| 1× | Dupont-Jumper Set |
| 1× | Litze 0,14–0,25 mm² |

---

## 2 Prototyp-Stückliste (10×) ✅

| Komponente | Anzahl | Status |
|------------|--------|--------|
| ESP32-S3 XIAO | 1× | ✅ |
| 74HC595 | 2× | ✅ |
| CD4021BE | 2× | ✅ |
| LED 5 mm | 10× | ✅ |
| Taster 6×6 mm | 10× | ✅ |
| Widerstand 330 Ω | 10× | ✅ |
| Widerstand 10 kΩ | 10× | ✅ |
| Kondensator 100 nF | 4× | ✅ |

**Schaltplan:** `kicad/selection-panel-prototyp.kicad_sch` (Rev 1.1)

---

## 3 Pinbelegung ESP32-S3 XIAO

| Pin | Signal | Funktion | IC |
|-----|--------|----------|-----|
| D0 | LED_DATA | Serial Data | 74HC595 Pin 14 |
| D1 | LED_CLK | Shift Clock | 74HC595 Pin 11 |
| D2 | LED_LATCH | Latch/Store | 74HC595 Pin 12 |
| D3 | BTN_DATA | Serial Data | CD4021 Pin 3 |
| D4 | BTN_CLK | Clock | CD4021 Pin 10 |
| D5 | BTN_LOAD | Parallel Load | CD4021 Pin 9 |

---

## 4 Kaskadierung

**Kaskadierung** bedeutet: ICs in Reihe schalten. Der serielle Ausgang eines ICs geht in den seriellen Eingang des nächsten.

### 74HC595 (LED-Ausgabe)

```
ESP32 D0 ──► #0 Pin 14 (SER)
             #0 Pin 9 (QH') ──► #1 Pin 14 (SER)
                                #1 Pin 9 (QH') ──► ... ──► #12

D1 ──┬──► #0 Pin 11 (SRCLK)
     └──► #1 Pin 11 (SRCLK)  [Bus: alle parallel]

D2 ──┬──► #0 Pin 12 (RCLK)
     └──► #1 Pin 12 (RCLK)   [Bus: alle parallel]
```

### CD4021BE (Taster-Eingabe)

```
                                #1 Pin 3 (Q8) ──► #0 Pin 11 (SER)
ESP32 D3 ◄── #0 Pin 3 (Q8)

D4 ──┬──► #0 Pin 10 (CLK)
     └──► #1 Pin 10 (CLK)    [Bus: alle parallel]

D5 ──┬──► #0 Pin 9 (P/S)
     └──► #1 Pin 9 (P/S)     [Bus: alle parallel]

#1 Pin 11 (SER) ──► +3,3V (VCC)  [wichtig!]
```

---

## 5 IC-Pinbelegung

### 74HC595 (DIP-16)

```
        ┌──────┐
   QB ─┤1   16├─ VCC (+3,3V)
   QC ─┤2   15├─ QA (Output 0)
   QD ─┤3   14├─ SER (Data In)
   QE ─┤4   13├─ OE (→ GND)
   QF ─┤5   12├─ RCLK (Latch)
   QG ─┤6   11├─ SRCLK (Clock)
   QH ─┤7   10├─ SRCLR (→ VCC)
  GND ─┤8    9├─ QH' (Serial Out)
        └──────┘
```

| Pin | Verbindung |
|-----|------------|
| 14 (SER) | ← D0 oder vorheriger QH' |
| 11 (SRCLK) | ← D1 (Bus) |
| 12 (RCLK) | ← D2 (Bus) |
| 9 (QH') | → nächster SER oder offen |
| 10 (SRCLR) | → VCC |
| 13 (OE) | → GND |

### CD4021BE (DIP-16)

```
        ┌──────┐
   P1 ─┤1   16├─ VDD (+3,3V)
   Q6 ─┤2   15├─ P8
   Q8 ─┤3   14├─ P3
   P4 ─┤4   13├─ P2
   P5 ─┤5   12├─ Q7
   P6 ─┤6   11├─ SER (Data In)
   P7 ─┤7   10├─ CLK
  VSS ─┤8    9├─ P/S (Load)
        └──────┘
```

| Pin | Verbindung |
|-----|------------|
| 3 (Q8) | → D3 oder vorheriger SER |
| 10 (CLK) | ← D4 (Bus) |
| 9 (P/S) | ← D5 (Bus) |
| 11 (SER) | ← nächster Q8 oder **→ VCC** |

**P/S = HIGH für Load** — invertiert zum 74HC165!

---

## 6 Prototyp-Verdrahtung (Bit-Mapping)

Der Prototyp verwendet nicht-lineare Verdrahtung auf dem Breadboard.
Das Bit-Mapping in `config.h` kompensiert dies:

```cpp
// config.h
static const uint8_t BUTTON_BIT_MAP[10] = {
    7,   // Taster 1 → Bit 7
    6,   // Taster 2 → Bit 6
    5,   // Taster 3 → Bit 5
    4,   // Taster 4 → Bit 4
    3,   // Taster 5 → Bit 3
    2,   // Taster 6 → Bit 2
    1,   // Taster 7 → Bit 1
    0,   // Taster 8 → Bit 0
    15,  // Taster 9 → Bit 15 (2. IC)
    14   // Taster 10 → Bit 14 (2. IC)
};
```

**Produktion:** Lineare Verdrahtung (Taster 1 = Bit 0, etc.)

---

## 7 Wichtige Regeln

| Regel | Grund |
|-------|-------|
| CD4021 letzter IC: Pin 11 → VCC | CMOS-Eingänge nie floaten |
| 74HC595 letzter IC: Pin 9 offen OK | Ausgang treibt aktiv |
| 100 nF an jedem VCC/VDD | Spannungsspitzen filtern |
| Pull-Up 10 kΩ an jedem Taster | Definierter Pegel wenn offen |
| Unbenutzte CD4021-Inputs → VCC | Keine zufälligen Trigger |

---

## 8 Stromversorgung

| Betriebsart | Stromaufnahme |
|-------------|---------------|
| One-hot (1 LED) | max. 20 mA |
| Alle LEDs (Test) | max. 2 A |

**Empfehlung:** 5V vom Pi-Netzteil abzweigen. Bei 3,3V-Betrieb direkt vom ESP32 3V3-Pin.

---

## 9 Bekannte Hardware-Einschränkungen

| Problem | Lösung |
|---------|--------|
| ESP32-S3 USB-CDC fragmentiert Serial | Firmware: `Serial.flush()` |
| CD4021 braucht längere Pulse als 74HC | Firmware: `delayMicroseconds(5)` für Load |
| Prototyp nicht-lineare Verdrahtung | Firmware: `BUTTON_BIT_MAP` |

---

## 10 Nächste Schritte (Hardware)

- [ ] PCB-Design für 100 Buttons (KiCad)
- [ ] Lineare Verdrahtung (kein Bit-Mapping nötig)
- [ ] Gehäuse-Design
- [ ] Stromversorgung dimensionieren (100 LEDs)
