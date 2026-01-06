---
title: "btn-led-test: 74HC595 + CD4021B (10 Taster / 10 LEDs) – XIAO ESP32-S3"
repo: "selection-panel"
path: "firmware/btn-led-test"
framework: "PlatformIO + Arduino"
board: "Seeed Studio XIAO ESP32S3"
version: "2.0.0"
status: "working"
---

# btn-led-test

Hardware-Test für Selection-Panel: **Taster drücken → LED leuchtet**.

- **10 Taster** über **2× CD4021B** (Parallel-In / Serial-Out)
- **10 LEDs** über **2× 74HC595** (Serial-In / Parallel-Out)
- **Hardware-PWM** für Helligkeitssteuerung (50%)
- **Skalierbar** auf 100 Taster/LEDs durch eine Konstante

---

## 1) Hardware-Übersicht

### 74HC595 (LEDs)

| Pin | Symbol | Funktion |
|-----|--------|----------|
| 14  | SI     | Serial Data Input |
| 11  | SCK    | Shift Register Clock |
| 12  | RCK    | Storage Register Clock (Latch) |
| 13  | G̅ (OE) | Output Enable (PWM) |
| 10  | SCLR   | Shift Clear (→ VCC) |

### CD4021B (Taster)

| Pin | Symbol | Funktion |
|-----|--------|----------|
| 3   | Q8     | Serial Data Output |
| 10  | CLK    | Clock |
| 9   | P/S    | Parallel/Serial Control |
| 11  | SI     | Serial Input (Daisy-Chain) |

---

## 2) Pinbelegung (XIAO ESP32-S3)

| Signal | XIAO Pin | Funktion |
|--------|----------|----------|
| SCK    | D8       | Shared Clock (595 + 4021) |
| MOSI   | D10      | 74HC595 Serial Data |
| RCK    | D0       | 74HC595 Latch |
| OE     | D2       | 74HC595 PWM (Hardware) |
| MISO   | D9       | CD4021B Serial Data |
| P/S    | D1       | CD4021B Load/Shift |

### Verdrahtung OE (PWM)

```
IC#0 Pin 13 (G̅) ──┬── D2
IC#1 Pin 13 (G̅) ──┘
```

---

## 3) Firmware-Design

### Prinzipien

- **Simple is best** – eine Konstante steuert die Skalierung
- **Keine Redundanz** – Bit-Banging für beide IC-Typen
- **Engineering-Praxis** – Timing nach Datenblatt

### Konfiguration

```cpp
constexpr size_t BTN_COUNT = 10;   // 10 für Prototyp, 100 für Produktion
constexpr size_t LED_COUNT = 10;
constexpr uint8_t PWM_DUTY = 50;   // 0-100%
```

### CD4021B Lese-Sequenz

Q8 liegt nach Parallel-Load **sofort** am Ausgang. Hardware-SPI würde das erste Bit verpassen.

```
P/S HIGH → P/S LOW → Lesen → Clock → Lesen → Clock → ...
```

### Bit-Mapping

| IC | Pin | Bit | Taster/LED |
|----|-----|-----|------------|
| 0  | P8  | 7   | 1          |
| 0  | P7  | 6   | 2          |
| …  | …   | …   | …          |
| 0  | P1  | 0   | 8          |
| 1  | P8  | 7   | 9          |
| 1  | P7  | 6   | 10         |

---

## 4) Build / Flash / Monitor

```bash
pio run -t upload
pio device monitor -b 115200
```

---

## 5) Erwartete Ausgabe

```
Selection Panel | PWM: 50% (Hardware)
Taster: 1
Taster: -
Taster: 2
Taster: -
Taster: 1, 5
Taster: -
```

---

## 6) PWM-Konfiguration

| Parameter | Wert | Beschreibung |
|-----------|------|--------------|
| `PWM_DUTY` | 50 | Helligkeit 0-100% |
| `PWM_FREQ` | 1000 | 1 kHz (flimmerfrei) |
| `LEDC_CHANNEL` | 0 | ESP32 LEDC Kanal |

OE ist **Active-LOW**: PWM invertiert → `duty = 255 - (percent * 255 / 100)`

---

## 7) Skalierung auf 100×

### Firmware

```cpp
constexpr size_t BTN_COUNT = 100;
constexpr size_t LED_COUNT = 100;
```

Abgeleitete Werte berechnen sich automatisch:

| Konstante | 10× | 100× |
|-----------|-----|------|
| `BTN_BYTES` | 2 | 13 |
| `LED_BYTES` | 2 | 13 |

### Hardware

- 13× CD4021B kaskadieren
- 13× 74HC595 kaskadieren
- Alle OE-Pins parallel an D2

---

## 8) Changelog

### v2.0.0

- **Neu:** Hardware-PWM über OE-Pin (LEDC)
- **Neu:** Shared Clock für 595 und 4021
- **Neu:** Skalierbare Architektur (eine Konstante)
- **Fix:** CD4021B Bit-Banging (erstes Bit vor Clock lesen)
- **Fix:** Korrekte Bit-Reihenfolge für beide ICs
- Pinbelegung optimiert für Shared SPI-Bus

### v1.1.1

- Skalierungs-Dokumentation ergänzt

### v1.1.0

- CD4021 Lese-Sequenz korrigiert
- Timing-Delays angepasst

### v1.0.0

- Initiale Version

---

## 9) Referenzen

- [74HC595 Datenblatt](https://www.st.com/resource/en/datasheet/m74hc595.pdf)
- [CD4021B Datenblatt](https://www.ti.com/lit/ds/symlink/cd4021b.pdf)
- [XIAO ESP32S3 Wiki](https://wiki.seeedstudio.com/xiao_esp32s3_getting_started/)
