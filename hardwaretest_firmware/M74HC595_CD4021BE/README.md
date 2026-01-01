---
title: "Hardwaretest: 74HC595 + CD4021BE (10 Taster / 10 LEDs) – XIAO ESP32-S3"
repo: "M74HC595_CD4021BE"
path: "selection-panel/hardwaretest_firmware/M74HC595_CD4021BE"
framework: "PlatformIO + Arduino"
board: "Seeed Studio XIAO ESP32S3"
version: "1.1.1"
status: "working"
---

# Hardwaretest: 74HC595 + CD4021BE (10×)

Dieses Projekt ist ein **Hardware-/Firmware-Test** für ein Selection-Panel:

- **10 Taster** werden über **2× CD4021BE** (Parallel-In / Serial-Out) eingelesen.
- **10 LEDs** werden über **2× 74HC595** (Serial-In / Parallel-Out) angesteuert.
- Zuordnung: **BTN1 ↔ LED1 … BTN10 ↔ LED10** (Toggle pro Tastendruck).

Repo-Struktur:

```text
.
├── platformio.ini
├── src/
│   └── main.cpp
└── test/
    └── main.cpp
```

Arbeitsverzeichnis (lokal):

```text
/Users/jan/daten/start/IoT/AMR/selection-panel-esp32s3-raspi5-100x/selection-panel/hardwaretest_firmware/M74HC595_CD4021BE
```

---

## 1) Hardware-Konzept

### LEDs (Ausgänge) – 2× 74HC595

- Kaskade aus zwei 8-bit Schieberegistern ⇒ **16 Ausgänge**, davon **10 genutzt**.
- Daten werden seriell geschoben und mit **Latch** gleichzeitig übernommen.

Signale:

| Pin | Symbol | Funktion |
|-----|--------|----------|
| 14  | SER    | Serial Data Input |
| 11  | SRCLK  | Shift Register Clock (positive Flanke) |
| 12  | RCLK   | Storage Register Clock / Latch |
| 13  | OE     | Output Enable (active-low, extern auf GND) |
| 10  | SCLR   | Shift Clear (active-low, extern auf VCC) |

### Taster (Eingänge) – 2× CD4021BE

- Kaskade aus zwei 8-bit Parallel-In / Serial-Out ⇒ **16 Eingänge**, davon **10 genutzt**.
- Taster sind **active-low**: Pull-Up nach 3V3, Taster nach GND ⇒ gedrückt = `0`.

Signale:

| Pin | Symbol | Funktion |
|-----|--------|----------|
| 3   | Q8     | Serial Data Output (letztes Bit der Kette) |
| 10  | CLK    | Clock (positive Flanke schiebt Daten) |
| 9   | P/S    | Parallel/Serial Control (HIGH=Load, LOW=Shift) |
| 11  | SI     | Serial Input (für Kaskadierung) |

---

## 2) Pinbelegung (XIAO ESP32-S3)

### 74HC595 (LEDs)

| Signal              | XIAO Pin |
| ------------------- | -------- |
| `LED_DATA`  (SER)   | `D0`     |
| `LED_CLOCK` (SRCLK) | `D1`     |
| `LED_LATCH` (RCLK)  | `D2`     |

### CD4021 (Taster)

| Signal            | XIAO Pin |
| ----------------- | -------- |
| `BTN_DATA`  (Q8)  | `D3`     |
| `BTN_CLOCK` (CLK) | `D4`     |
| `BTN_LOAD`  (P/S) | `D5`     |

---

## 3) LED-Strom (Hinweis)

Mit **3 kΩ** Vorwiderstand an **3,3 V** ergibt sich typ. nur **~0,1–0,5 mA** (abhängig von LED-Flussspannung $V_f$).
Die LEDs sind daher **sehr dimm**, als Funktionstest aber geeignet.

Für höhere Helligkeit: Vorwiderstand auf 330 Ω reduzieren → ~3–5 mA (innerhalb der 6 mA Spezifikation des 74HC595).

---

## 4) Firmware-Design (Engineering-Praxis)

### Ziele

- **minimal, konsistent, wartbar**
- **nicht-blockierend** (kein `delay()`, nur kurze `delayMicroseconds()` fürs Timing)
- **skalierbar** über zentrale Konstanten (`NUM_BTNS`, `NUM_LEDS`, `SHIFT_BITS`)
- Verdrahtungsänderungen nur in **Mapping-Tabellen**

### CD4021 Lese-Sequenz (v1.1 korrigiert)

Nach dem Parallel Load liegt das erste Bit (Q8) **sofort** am Ausgang. Die korrekte Sequenz:

```
Load → Lesen → Clock → Lesen → Clock → ... → Lesen (kein Clock am Ende)
```

**Timing-Parameter** (CD4021B Datenblatt, bei 5 V):

| Parameter | Symbol | Min | Typ | Einheit |
|-----------|--------|-----|-----|---------|
| P/S Pulse Width | $t_{WH}$ | – | 80 | ns |
| P/S Removal Time | $t_{REM}$ | – | 70 | ns |
| Clock Pulse Width | $t_W$ | – | 40 | ns |
| Setup Time (Parallel) | $t_s$ | – | 25 | ns |

Bei 3,3 V verlängern sich die Zeiten. Der Code verwendet 2 µs Delays für Robustheit (Faktor ~25× Sicherheit).

**Warum kein Hardware-SPI für CD4021B?**

SPI sendet immer zuerst einen Clock, dann wird gelesen. Der CD4021B liefert das erste Bit aber **vor** dem ersten Clock. Hardware-SPI würde das erste Bit verpassen.

### Debouncing

- Stabiler Zustand über `DEBOUNCE_MS` (default: 30 ms)
- Flankenerkennung: nur 0→1 Übergang löst Toggle aus
- Polling-Rate über `POLL_MS` begrenzt (default: 1 ms)

---

## 5) Button-Bitmapping (gemessen)

Die reale Bitposition je Taster wurde empirisch ermittelt:

```cpp
// BTN1..BTN10 -> Bitposition im 16-bit Stream (MSB-first)
constexpr uint8_t BTN_BIT_MAP[10] = { 15, 12, 13, 11, 10, 9, 8, 14, 7, 4 };
```

Der Stream wird **MSB-first** aufgebaut: erstes gelesenes Bit = Bit 15.

Wenn sich Verdrahtung/Mechanik ändert: **nur `BTN_BIT_MAP[]` anpassen**.

---

## 6) Build / Flash / Monitor (PlatformIO)

### VS Code

1. PlatformIO öffnen
2. `Upload` ausführen
3. Serial Monitor: `115200`

### CLI

```bash
pio run
pio run -t upload
pio device monitor -b 115200
```

---

## 7) Erwartete Ausgabe

Beim Start:

```text
=====================================
74HC595 + CD4021BE Hardwaretest v1.1.1
BTN1..BTN10 toggles LED1..LED10
=====================================
```

Beim Drücken von BTN1..BTN10:

```text
BTN1 pressed -> toggle LED1
BTN2 pressed -> toggle LED2
...
BTN10 pressed -> toggle LED10
```

---

## 8) Parameter (Tuning)

| Parameter | Default | Bereich | Wirkung |
|-----------|---------|---------|---------|
| `DEBOUNCE_MS` | 30 | 20–80 | Höher = stabiler bei stark prellenden Tastern |
| `POLL_MS` | 1 | 1–10 | Höher = geringere CPU-Last, langsamere Reaktion |

---

## 9) Skalierung auf 100×

Für 100 Taster / 100 LEDs sind folgende Änderungen nötig:

### Hardware

- 13× 74HC595 + 13× CD4021 kaskadieren (= 104 I/Os)

### Firmware

| Änderung | Aktuell (10×) | Für 100× |
|----------|---------------|----------|
| `NUM_LEDS` | 10 | 100 |
| `NUM_BTNS` | 10 | 100 |
| `SHIFT_BITS` | 16 | 104 |
| `g_ledState` | `uint16_t` | `uint8_t[13]` |
| `g_btnRaw` | `uint16_t` | `uint8_t[13]` |
| `LED_BIT_MAP` | `[10]` | `[100]` |
| `BTN_BIT_MAP` | `[10]` | `[100]` |

### Optional: Hardware-SPI für 74HC595

Bei 100× wird Bit-Banging zum Flaschenhals (~416 µs vs. ~26 µs mit SPI).

| Signal | Aktuell | Hardware-SPI |
|--------|---------|--------------|
| SER    | D0      | D10 (MOSI)   |
| SRCLK  | D1      | D8 (SCK)     |
| RCLK   | D2      | D2 (bleibt)  |

**CD4021B bleibt bei Bit-Banging** – SPI verpässt das erste Bit nach Parallel Load.

---

## 10) Changelog

### v1.1.1

- Skalierungs-Dokumentation für 100× im Code-Header ergänzt
- SPI-Hinweis: Warum CD4021B kein Hardware-SPI nutzen kann
- README erweitert mit detaillierter Skalierungs-Tabelle

### v1.1.0

- **Fix:** CD4021 Lese-Sequenz korrigiert (erst lesen, dann clocken)
- **Fix:** Timing-Delays an Datenblatt-Spezifikation angepasst
- Dokumentation mit Timing-Parametern erweitert
- Startup-Banner hinzugefügt

### v1.0.0

- Initiale Version mit funktionierendem Toggle-Mechanismus

---

## 11) Referenzen

- [74HC595 Datenblatt (SGS-Thomson)](https://www.st.com/resource/en/datasheet/m74hc595.pdf)
- [CD4021B Datenblatt (Texas Instruments)](https://www.ti.com/lit/ds/symlink/cd4021b.pdf)
- [XIAO ESP32S3 Wiki (Seeed Studio)](https://wiki.seeedstudio.com/xiao_esp32s3_getting_started/)
